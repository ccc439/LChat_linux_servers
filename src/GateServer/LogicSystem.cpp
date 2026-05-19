#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"

LogicSystem::LogicSystem() {
	// 注册一个GET请求处理器，路径是 "/get_test"（访问http://localhost:8080/get_test）
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
		beast::ostream(conn->_response.body()) << "receive get_test req" << std::endl;//写入HTTP响应体
		int i = 0;
		for (auto& elem : conn->_get_params) {
			i++;
			beast::ostream(conn->_response.body()) << "param " << i << " key is " << elem.first << std::endl;
			beast::ostream(conn->_response.body()) << "param " << i << " value is " << elem.second << std::endl;
		}

	});
	// 注册一个POST请求处理器，获取验证码
	RegPost("/get_verifycode", [](std::shared_ptr<HttpConnection> conn) {
		//获取请求体，并转为string
		auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		conn->_response.set(http::field::content_type, "text/json");// 告诉客户端，返回的数据是JSON格式
		Json::Value root;        // 用于构建返回的JSON
		Json::Reader reader;     // JSON解析器
		Json::Value src_root;    // 用于存放解析出的原始JSON数据
		bool parse_success = reader.parse(body_str, src_root);//解析body字符串为JSON对象（json反序列化）
		if (!parse_success) {
			//若解析失败
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();//json序列化
			beast::ostream(conn->_response.body()) << jsonstr;//将jsonstr写入响应体
			return true;
		}

		if (!src_root.isMember("email")) {
			//若email字段不存在
			std::cout << "Failed to parse JSON data" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}
		//正常情况
		auto email = src_root["email"].asString();//获取email字段的值
		//获取验证码，rsp接收返回的响应消息结构体
		GetVerifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVerifyCode(email);

		root["error"] = rsp.error();
		root["email"] = rsp.email();
		root["code"] = rsp.code();
		std::string jsonstr = root.toStyledString();
		beast::ostream(conn->_response.body()) << jsonstr;
		return true;
	});
	// 注册一个POST请求处理器，注册用户
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> conn) {
		auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		conn->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}
		//在QT已经处理了用户输入 密码 与 确认密码 不一致的问题，这里就不用处理了
		
		//先查找redis中email对应的验证码是否合理
		std::string  verify_code;
		bool b_get_verify = RedisMgr::GetInstance()->Get(CODE_PREFIX + src_root["email"].asString(), verify_code);
		if (!b_get_verify) {
			std::cout << " get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		if (verify_code != src_root["verifycode"].asString()) {
			std::cout << " verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		
		auto name = src_root["user"].asString();
		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		auto icon = src_root["icon"].asString();

		//查找数据库判断用户名或邮箱是否存在，若不存在，注册用户信息
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = 0;
		root["email"] = email;
		root["uid"] = uid;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = src_root["confirm"].asString();
		root["icon"] = icon;
		root["verifycode"] = src_root["verifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(conn->_response.body()) << jsonstr;
		return true;
		});

	// 注册一个POST请求处理器，重置密码
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> conn) {
		auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		conn->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		//先查找redis中email对应的验证码是否合理
		std::string  verify_code;
		bool b_get_verify = RedisMgr::GetInstance()->Get(CODE_PREFIX + src_root["email"].asString(), verify_code);
		if (!b_get_verify) {
			std::cout << " get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		if (verify_code != src_root["verifycode"].asString()) {
			std::cout << " verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		auto name = src_root["user"].asString();
		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();

		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name,email);
		if (!email_valid) {
			std::cout << " user email mismatch" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}
		//更新密码
		bool update_flag = MysqlMgr::GetInstance()->UpdatePwd(email,pwd);
		if (!update_flag) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PwdUpdateFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to update password" << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["verifycode"] = src_root["verifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(conn->_response.body()) << jsonstr;
		return true;
		});

	// 注册一个POST请求处理器，用户登录
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> conn) {
		auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		conn->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		//查询数据库判断邮箱和密码是否匹配
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_valid) {
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PwdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		//查询StatusServer找到合适的连接
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(conn->_response.body()) << jsonstr;
		return true;
		});
}
LogicSystem::~LogicSystem() {}
//注册Get请求
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}
//处理Get请求
bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> conn) {
	if (_get_handlers.find(url) == _get_handlers.end()) {
		//若没找到，直接返回
		return false;
	}
	//若找到，进行对应处理
	_get_handlers[url](conn);
	return true;
}
//注册Post请求
void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}
//处理Post请求
bool LogicSystem::HandlePost(std::string url, std::shared_ptr<HttpConnection> conn) {
	if (_post_handlers.find(url) == _post_handlers.end()) {
		//若没找到，直接返回
		return false;
	}
	//若找到，进行对应处理
	_post_handlers[url](conn);
	return true;
}