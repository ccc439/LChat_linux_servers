#pragma once
#include "const.h"

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(boost::asio::io_context &ioc);
	void Start();
	tcp::socket& GetSocket();
private:
	void CheckDeadline();//超时检测
	void WriteResponse();//发送响应（回包）
	void HandleReq();//处理请求
	void PreParseGetParam();//解析get请求的参数
	tcp::socket _socket;
	beast::flat_buffer _buffer{ 8192 };//缓冲区，用于接收数据
	http::request<http::dynamic_body> _request;//用于解析请求
	http::response<http::dynamic_body> _response;//用于回应客户端
	net::steady_timer deadline_{//定时器，用于判断请求是否超时
		_socket.get_executor(),
		std::chrono::seconds(60)
	};
	//用于解析get请求的参数
	std::string _get_url;//url
	std::unordered_map<std::string, std::string> _get_params;//参数
};

