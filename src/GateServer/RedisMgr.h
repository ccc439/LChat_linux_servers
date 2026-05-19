#pragma once
#include "const.h"
#include "hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include "Singleton.h"
#include <condition_variable>
#include <thread>
#include <chrono>
class RedisConPool {
public:
    //初始化连接池
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false), pwd_(pwd), counter_(0) {
        try {
            //创建poolSize_个Redis连接
            for (size_t i = 0; i < poolSize_; ++i) {
                auto* context = redisConnect(host, port);
                if (context == nullptr || context->err != 0) {
                    if (context != nullptr) {
                        redisFree(context);
                    }
                    continue;
                }

                //发送AUTH命令进行密码认证
                auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
                if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                    std::cout << "认证失败" << std::endl;
                    if (reply != nullptr) {
                        freeReplyObject(reply);
                    }
                    redisFree(context); // 认证失败时释放连接，防止泄漏
                    continue;
                }

                //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
                freeReplyObject(reply);
                std::cout << "认证成功" << std::endl;
                connections_.push(context);
            }

            // 启动健康检查线程
            check_thread_ = std::thread([this]() {
                while (!b_stop_) {
                    counter_++;
                    if (counter_ >= 60) { // 每60秒执行一次健康检查
                        checkThread();
                        counter_ = 0;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                });
        }
        catch (...) {
            // 异常时清理资源
            Close();
            ClearConnections();
            throw;
        }
    }

    // 健康检查函数：定期检测连接有效性
    void checkThread() {
        std::vector<redisContext*> local_connections;

        // 阶段1: 快速取出所有连接（持锁时间 < 1ms）
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (b_stop_) return;

            int pool_size = connections_.size();
            for (int i = 0; i < pool_size; i++) {
                local_connections.push_back(connections_.front());
                connections_.pop();
            }
        }

        // 阶段2: 在无锁状态下执行网络I/O（不会阻塞业务线程）
        for (auto* context : local_connections) {
            try {
                // 执行 PING 命令探活
                auto reply = (redisReply*)redisCommand(context, "PING");
                if (!reply) {
                    std::cout << "PING failed: reply is null" << std::endl;
                    // 连接失效，重建连接
                    redisFree(context);
                    context = redisConnect(host_, port_);
                    if (context == nullptr || context->err != 0) {
                        if (context != nullptr) {
                            redisFree(context);
                        }
                        context = nullptr; // 标记为无效
                    }
                    else {
                        // 重新认证
                        auto auth_reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
                        if (auth_reply == nullptr || auth_reply->type == REDIS_REPLY_ERROR) {
                            std::cout << "Re-auth failed" << std::endl;
                            if (auth_reply != nullptr) {
                                freeReplyObject(auth_reply);
                            }
                            redisFree(context);
                            context = nullptr;
                        }
                        else {
                            freeReplyObject(auth_reply);
                            std::cout << "Re-auth success" << std::endl;
                        }
                    }
                    continue;
                }

                freeReplyObject(reply);
                // PING 成功，连接有效
            }
            catch (std::exception& exp) {
                std::cout << "Error keeping connection alive: " << exp.what() << std::endl;
                // 异常时重建连接
                redisFree(context);
                context = redisConnect(host_, port_);
                if (context == nullptr || context->err != 0) {
                    if (context != nullptr) {
                        redisFree(context);
                    }
                    context = nullptr;
                }
                else {
                    auto auth_reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
                    if (auth_reply && auth_reply->type != REDIS_REPLY_ERROR) {
                        freeReplyObject(auth_reply);
                    }
                    else {
                        if (auth_reply) freeReplyObject(auth_reply);
                        redisFree(context);
                        context = nullptr;
                    }
                }
            }

            // 将连接放回局部容器（无论是否重建成功）
            if (context != nullptr) {
                // 保持原位置，稍后统一归还
            }
        }

        // 阶段3: 快速归还所有有效连接（持锁时间 < 1ms）
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (b_stop_) {
                // 如果已关闭，释放所有连接
                for (auto* ctx : local_connections) {
                    if (ctx != nullptr) {
                        redisFree(ctx);
                    }
                }
                return;
            }

            for (auto* ctx : local_connections) {
                if (ctx != nullptr) {
                    connections_.push(ctx);
                }
            }
        }
    }

    // 清空所有连接（用于异常清理或显式关闭）
    void ClearConnections() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            auto* context = connections_.front();
            redisFree(context);
            connections_.pop();
        }
    }

    //释放所有池中的 Redis 连接
    ~RedisConPool() {
        Close(); // 先确保后台线程已停止
        ClearConnections(); // 再释放所有连接
    }

    //从连接池获取连接
    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);//加锁，保证线程安全
        //阻塞等待，直到：连接池关闭，或连接池有空闲连接（只在 notify_one/notify_all 唤醒时检查条件）
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connections_.empty();
            });
        //如果连接池关闭则直接返回空指针
        if (b_stop_) {
            return nullptr;
        }
        //如果连接池有空闲连接，获取连接
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    //向连接池归还连接
    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(mutex_);//加锁，保证线程安全
        //如果连接池关闭则直接返回
        if (b_stop_) {
            if (context != nullptr) {
                redisFree(context); // 关闭时归还的连接直接释放
            }
            return;
        }
        //归还连接
        connections_.push(context);
        //唤醒一个等待的线程（连接池有空闲连接可用了）
        cond_.notify_one();
    }

    //关闭连接池
    void Close() {
        b_stop_ = true;
        //唤醒所有等待的线程（连接池要关闭了）
        cond_.notify_all();

        // 等待健康检查线程退出，防止use-after-free
        if (check_thread_.joinable()) {
            check_thread_.join();
        }
    }

private:
    std::atomic<bool> b_stop_;//连接池关闭标志，原子操作保证线程安全
    size_t poolSize_;//连接池大小，预设的连接数量
    const char* host_;//Redis服务器主机地址
    int port_;//Redis服务器端口
    const char* pwd_;// Redis密码，用于重连时认证
    std::queue<redisContext*> connections_;//存储空闲连接的队列
    std::mutex mutex_;//互斥锁，保护连接队列的线程安全
    std::condition_variable cond_;//条件变量，用于线程同步等待
    std::thread check_thread_; // 健康检查线程
    int counter_; // 计数器，控制健康检查频率
};

class RedisMgr : public Singleton<RedisMgr>,
	public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool HDel(const std::string& key, const std::string& field);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);
	void Close() {
		_con_pool->Close();
		_con_pool->ClearConnections();
	}
private:
	RedisMgr();
	unique_ptr<RedisConPool>  _con_pool;
};

