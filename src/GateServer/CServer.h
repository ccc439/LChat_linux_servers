#pragma once
#include "const.h"

//CRTP
class CServer :public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, unsigned short& port);
    void Start();//启动网关服务器
private:
    tcp::acceptor _acceptor;//接收对端连接的连接器
    boost::asio::io_context& _ioc;//上下文（io_context ~ epoll）
};