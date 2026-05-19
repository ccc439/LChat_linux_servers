#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc):_socket(ioc) {

}

tcp::socket& HttpConnection::GetSocket() {
	return _socket;
}

void HttpConnection::Start() {
	auto self = shared_from_this();
	//异步读
	http::async_read(_socket,_buffer,_request,[self](beast::error_code ec,std::size_t bytes_transferred){
		try {
			if (ec) {
				//若出错，直接返回
				std::cout << "http read error is " << ec.what() << std::endl;
				return;
			}
			//若成功
			boost::ignore_unused(bytes_transferred);//抑制编译器未使用变量警告
			self->HandleReq();//处理请求
			self->CheckDeadline();//超时检测
		}
		catch(std::exception& exp){
			std::cout << "exception is " << exp.what() << std::endl;
		}
	});
	
}
void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	deadline_.async_wait([self](beast::error_code ec) {//当定时器到达设定时间时会自动执行传入的回调函数（非阻塞）
		if (!ec) {
			//若成功，表示确实超时，关闭套接字
			self->_socket.close(ec);
		}
	});
}
void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	//设置响应体长度（便于对方切包）
	_response.content_length(_response.body().size());
	//发送响应
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transferred) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);//关闭发送方socket，避免TCP半开连接泄漏
		self->deadline_.cancel();//发送响应完，取消定时器
	});
}

//url解析函数
//char从十进制转为16进制的方法
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}
//char从16进制转为十进制的方法
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}
//url编码
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}
//url解码
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}
//解析get请求的参数
void HttpConnection::PreParseGetParam() {
	// 提取 URI  
	auto uri = _request.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); 
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}
void HttpConnection::HandleReq() {
	//HTTP 响应适配客户端协议版本
	_response.version(_request.version());
	//设置为短连接
	_response.keep_alive(false);
	//处理http请求：get请求类型
	if (_request.method() == http::verb::get) {
		//解析get请求的参数
		PreParseGetParam();
		//交给LogicSystem类处理
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url,shared_from_this());
		if (!success) {
			//若失败
			_response.result(http::status::not_found);//将响应状态码设置为 404
			_response.set(http::field::content_type, "text/plain");//定义响应类型为纯文本
			beast::ostream(_response.body()) << "url not found\r\n";//将错误信息写入响应体
			WriteResponse();//发送响应
			return;
		}
		//若成功
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");//告知客户端：服务器为GateServer
		WriteResponse();
		return;
	}
	//处理http请求：post请求类型
	if (_request.method() == http::verb::post) {
		//交给LogicSystem类处理
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			//若失败
			_response.result(http::status::not_found);//将响应状态码设置为 404
			_response.set(http::field::content_type, "text/plain");//定义响应类型为纯文本
			beast::ostream(_response.body()) << "url not found\r\n";//将错误信息写入响应体
			WriteResponse();//发送响应
			return;
		}
		//若成功
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");//告知客户端：服务器为GateServer
		WriteResponse();
		return;
	}
}