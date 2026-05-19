#include <iostream>
#include "CServer.h"
#include "ConfigMgr.h"
#include "const.h"
/*#include "RedisMgr.h"

void TestRedis() {
    //连接redis 需要启动才可以进行连接
//redis默认监听端口为6387 可以再配置文件中修改
    redisContext* c = redisConnect("127.0.0.1", 6380);
    if (c->err)
    {
        printf("Connect to redisServer faile:%s\n", c->errstr);
        redisFree(c);        return;
    }
    printf("Connect to redisServer Success\n");

    std::string redis_password = "123456";
    redisReply* r = (redisReply*)redisCommand(c, "AUTH %s", redis_password.c_str());
    if (r->type == REDIS_REPLY_ERROR) {
        printf("Redis认证失败！\n");
    }
    else {
        printf("Redis认证成功！\n");
    }

    //为redis设置key
    const char* command1 = "set stest1 value1";

    //执行redis命令行
    r = (redisReply*)redisCommand(c, command1);

    //如果返回NULL则说明执行失败
    if (NULL == r)
    {
        printf("Execut command1 failure\n");
        redisFree(c);        return;
    }

    //如果执行失败则释放连接
    if (!(r->type == REDIS_REPLY_STATUS && (strcmp(r->str, "OK") == 0 || strcmp(r->str, "ok") == 0)))
    {
        printf("Failed to execute command[%s]\n", command1);
        freeReplyObject(r);
        redisFree(c);        return;
    }

    //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command1);

    const char* command2 = "strlen stest1";
    r = (redisReply*)redisCommand(c, command2);

    //如果返回类型不是整形 则释放连接
    if (r->type != REDIS_REPLY_INTEGER)
    {
        printf("Failed to execute command[%s]\n", command2);
        freeReplyObject(r);
        redisFree(c);        return;
    }

    //获取字符串长度
    int length = r->integer;
    freeReplyObject(r);
    printf("The length of 'stest1' is %d.\n", length);
    printf("Succeed to execute command[%s]\n", command2);

    //获取redis键值对信息
    const char* command3 = "get stest1";
    r = (redisReply*)redisCommand(c, command3);
    if (r->type != REDIS_REPLY_STRING)
    {
        printf("Failed to execute command[%s]\n", command3);
        freeReplyObject(r);
        redisFree(c);        return;
    }
    printf("The value of 'stest1' is %s\n", r->str);
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command3);

    const char* command4 = "get stest2";
    r = (redisReply*)redisCommand(c, command4);
    if (r->type != REDIS_REPLY_NIL)
    {
        printf("Failed to execute command[%s]\n", command4);
        freeReplyObject(r);
        redisFree(c);        return;
    }
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command4);

    //释放连接资源
    redisFree(c);

}
void TestRedisMgr() {
    assert(RedisMgr::getInstance()->Set("blogwebsite", "llfc.club"));
    std::string value = "";
    assert(RedisMgr::getInstance()->Get("blogwebsite", value));
    assert(RedisMgr::getInstance()->Get("nonekey", value) == false);
    assert(RedisMgr::getInstance()->HSet("bloginfo", "blogwebsite", "llfc.club"));
    assert(RedisMgr::getInstance()->HGet("bloginfo", "blogwebsite") != "");
    assert(RedisMgr::getInstance()->ExistsKey("bloginfo"));
    assert(RedisMgr::getInstance()->Del("bloginfo"));
    assert(RedisMgr::getInstance()->Del("bloginfo"));
    assert(RedisMgr::getInstance()->ExistsKey("bloginfo") == false);
    assert(RedisMgr::getInstance()->LPush("lpushkey1", "lpushvalue1"));
    assert(RedisMgr::getInstance()->LPush("lpushkey1", "lpushvalue2"));
    assert(RedisMgr::getInstance()->LPush("lpushkey1", "lpushvalue3"));
    assert(RedisMgr::getInstance()->RPop("lpushkey1", value));
    assert(RedisMgr::getInstance()->RPop("lpushkey1", value));
    assert(RedisMgr::getInstance()->LPop("lpushkey1", value));
    assert(RedisMgr::getInstance()->LPop("lpushkey2", value) == false);
}*/
int main()
{
    //TestRedis();
    //TestRedisMgr();
    
    //读取配置文件
    ConfigMgr& config = ConfigMgr::getInstance();
    std::string gate_port_str = config["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());
    
    try {
        net::io_context ioc{ 1 };//初始化上下文io_context，将服务器设置为单线程运行
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);//注册SIGINT，SIGTERM信号
        //非阻塞的系统信号监听，监听到注册的信号被触发时，执行回调
        signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
            if (ec) {
                //若出错，直接返回
                return;
            }
            //若成功（代表信号有效）
            ioc.stop();//停止io_context事件循环
        });
        std::make_shared<CServer>(ioc, gate_port)->Start();//创建服务器实例并启动监听
        std::cout << "Gate Server listening at port : " << gate_port << std::endl;
        ioc.run();//启动异步事件循环，开始处理客户端连接和IO操作
    }
    catch (std::exception const&e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

