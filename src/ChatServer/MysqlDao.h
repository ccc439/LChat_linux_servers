#pragma once
#include "const.h"
#include <thread>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>
#include "data.h"
#include <memory>
#include <queue>
#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>

class MySqlPool {
public:
    // 连接包装结构，存储连接和元数据
    struct SqlConnection {
        std::unique_ptr<sql::Connection> _con;
        int64_t _last_oper_time; // 最后操作时间戳

        SqlConnection(sql::Connection* con, int64_t timestamp)
            : _con(con), _last_oper_time(timestamp) {
        }
    };
    //初始化连接池
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
        try {
            //创建poolSize_个Mysql连接
            for (int i = 0; i < poolSize_; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();//获取MySQL驱动实例
                std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));//创建连接
                con->setSchema(schema_);//设置默认数据库

                // 记录连接创建时间，用于后续健康检查
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

                // 使用包装结构存储连接和时间戳，而非直接存Connection
                pool_.push(std::make_unique<SqlConnection>(con.release(), timestamp));
            }

            // 启动健康检查线程（由Close管理生命周期）
            _check_thread = std::thread([this]() {
                while (!b_stop_) {
                    checkConnection();
                    std::this_thread::sleep_for(std::chrono::seconds(60)); // 每60秒检查一次
                }
                });
        }
        catch (sql::SQLException& e) {
            // 处理异常
            std::cout << "mysql pool init failed, error: " << e.what() << std::endl;

            // 异常时清理已创建的连接，防止内存泄漏
            Close(); // 停止检查线程
            std::lock_guard<std::mutex> lock(mutex_);
            while (!pool_.empty()) {
                pool_.pop();
            }
            throw; // 重新抛出异常，让调用者知道初始化失败
        }
    }

    // 健康检查函数：定期检测连接有效性
    void checkConnection() {
        std::vector<std::unique_ptr<SqlConnection>> local_connections;

        // 阶段1: 快速取出所有连接（持锁时间 < 1ms）
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (b_stop_) return; // 如果已关闭则退出

            int poolsize = pool_.size();
            for (int i = 0; i < poolsize; i++) {
                local_connections.push_back(std::move(pool_.front()));
                pool_.pop();
            }
        }

        // 阶段2: 在无锁状态下执行网络I/O（不会阻塞业务线程）
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

        for (auto& conn_wrapper : local_connections) {
            // 惰性检查：5秒内活跃的连接跳过检测，减少不必要的I/O
            if (timestamp - conn_wrapper->_last_oper_time < 5) {
                continue;
            }

            try {
                // 执行轻量级探活查询
                std::unique_ptr<sql::Statement> stmt(conn_wrapper->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                conn_wrapper->_last_oper_time = timestamp; // 更新最后操作时间
            }
            catch (sql::SQLException& e) {
                std::cout << "Connection lost, reconnecting: " << e.what() << std::endl;

                // 连接失效时重建：释放旧连接，创建新连接
                try {
                    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                    sql::Connection* newcon = driver->connect(url_, user_, pass_);
                    newcon->setSchema(schema_);
                    conn_wrapper->_con.reset(newcon); // 替换为健康连接
                    conn_wrapper->_last_oper_time = timestamp;
                }
                catch (...) {
                    std::cout << "Reconnection failed" << std::endl;
                    // 重连失败时保留原连接，下次再试
                }
            }
        }

        // 阶段3: 快速归还所有连接（持锁时间 < 1ms）
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (b_stop_) return; // 如果已关闭则不归还

            for (auto& conn_wrapper : local_connections) {
                pool_.push(std::move(conn_wrapper));
            }
        }
    }

    //从连接池获取连接
    std::unique_ptr<SqlConnection> getConnection() { // 返回包装类型
        std::unique_lock<std::mutex> lock(mutex_);//加锁，保证线程安全
        //阻塞等待，直到：连接池关闭，或连接池有空闲连接（只在 notify_one/notify_all 唤醒时检查条件）
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !pool_.empty(); });
        //如果连接池关闭则直接返回空指针
        if (b_stop_) {
            return nullptr;
        }
        //如果连接池有空闲连接，获取连接
        std::unique_ptr<SqlConnection> conn_wrapper(std::move(pool_.front()));
        pool_.pop();

        // 更新最后使用时间
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        conn_wrapper->_last_oper_time = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

        return conn_wrapper;
    }

    //向连接池返还连接
    void returnConnection(std::unique_ptr<SqlConnection> conn_wrapper) {
        std::unique_lock<std::mutex> lock(mutex_);//加锁，保证线程安全
        //如果连接池关闭则直接返回
        if (b_stop_) {
            return;
        }

        // 更新最后使用时间
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        conn_wrapper->_last_oper_time = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

        //归还连接
        pool_.push(std::move(conn_wrapper));
        //唤醒一个等待的线程（连接池有空闲连接可用了）
        cond_.notify_one();
    }

    //关闭连接池
    void Close() {
        b_stop_ = true;
        //唤醒所有等待的线程（连接池要关闭了）
        cond_.notify_all();

        // 等待健康检查线程退出，防止use-after-free
        if (_check_thread.joinable()) {
            _check_thread.join();
        }
    }

    //释放所有池中的 Mysql 连接
    ~MySqlPool() {
        Close(); // 先确保后台线程已停止

        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();// unique_ptr自动释放Mysql连接
        }
    }

private:
    std::string url_;//MySQL服务器地址（如：tcp://127.0.0.1:3306）
    std::string user_;//数据库用户名
    std::string pass_;//数据库密码
    std::string schema_;//数据库名
    int poolSize_;//连接池大小，预设的连接数量
    std::queue<std::unique_ptr<SqlConnection>> pool_;//存储包装后的连接
    std::mutex mutex_;//互斥锁，保护连接队列的线程安全
    std::condition_variable cond_;//条件变量，用于线程同步等待
    std::atomic<bool> b_stop_;//连接池关闭标志，原子操作保证线程安全
    std::thread _check_thread; // 健康检查线程
};

class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& email, const std::string& newpwd);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
	bool AddFriendApply(const int& from, const int& to);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
	bool AuthFriendApply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);
private:
	std::unique_ptr<MySqlPool> pool_;
};


