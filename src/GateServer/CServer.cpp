#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
    :_ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}

void CServer::Start() {
    auto self = shared_from_this();
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<HttpConnection> new_conn = std::make_shared<HttpConnection>(io_context);
    //异步接受新连接
    _acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {
        try {
            //若出错，放弃该连接，继续监听其他连接
            if (ec) {
                self->Start();
                return;
            }
            //若成功，创建新连接，HttpConnection类管理此连接
           new_conn->Start();
            //继续监听其他连接
            self->Start();
        }
        catch (std::exception& exp) {
            std::cerr << "exception is " << exp.what() << std::endl;
            self->Start();
        }
    });

}