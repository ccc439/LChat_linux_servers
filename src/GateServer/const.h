#pragma once
#include <jsoncpp/json/json.h>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Singleton.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "hiredis.h"
#include <cassert>
#include <memory>

//简化作用域声明
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,//JSON解析错误
	RPCFailed = 1002,//RPC请求错误
	VerifyExpired = 1003,//验证码过期
	VerifyCodeErr = 1004,//验证码错误
	UserExist = 1005,//用户已存在
	PwdErr = 1006,//密码错误
	EmailNotMatch = 1007,//用户名与邮箱不匹配
	PwdUpdateFailed = 1008,//密码更新失败
};

#define CODE_PREFIX "code_"//验证码前缀

//Defer类
class Defer {
public:
	Defer(std::function<void()> func):func_(func){}
	//析构时执行传入的函数对象
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};