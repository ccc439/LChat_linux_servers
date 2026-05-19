#pragma once
#include "const.h"

class HttpConnection;//前向声明
typedef std::function<void(std::shared_ptr<HttpConnection>)>HttpHandler;//定义一个函数类型HttpHandler

//CRTP
class LogicSystem :public Singleton<LogicSystem>, public std::enable_shared_from_this<LogicSystem> {
    friend class Singleton<LogicSystem>;
public:
    void RegGet(std::string url, HttpHandler handler);//注册Get请求
    bool HandleGet(std::string url,std::shared_ptr<HttpConnection> conn);//处理Get请求
    void RegPost(std::string url, HttpHandler handler);//注册Post请求
    bool HandlePost(std::string url, std::shared_ptr<HttpConnection> conn);//处理Post请求
private:
    LogicSystem();
    ~LogicSystem();
    std::map<std::string, HttpHandler> _get_handlers;//处理Get请求的集合
    std::map<std::string, HttpHandler> _post_handlers;//处理Post请求的集合
};
