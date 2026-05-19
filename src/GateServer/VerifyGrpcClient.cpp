#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

VerifyGrpcClient::~VerifyGrpcClient(){}
VerifyGrpcClient::VerifyGrpcClient() {
	auto& gCfgMgr = ConfigMgr::getInstance();
	std::string host = gCfgMgr["VerifyServer"]["Host"];
	std::string port = gCfgMgr["VerifyServer"]["Port"];
	pool_.reset(new RpcConnPool(5, host, port));
}

GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email) {
	ClientContext context;//客户端上下文
	GetVerifyRsp response;//响应消息结构体
	GetVerifyReq request;//请求消息结构体
	request.set_email(email);//赋值给请求结构体的email字段
	//调用RPC方法
	auto stub = pool_->getConnection();
	Status status = stub->GetVerifyCode(&context, request, &response);
	//处理响应
	if (status.ok()) {
		//若成功，返回响应
		pool_->returnConnection(std::move(stub));
		return response;
	}
	else {
		pool_->returnConnection(std::move(stub));
		response.set_error(ErrorCodes::RPCFailed);
		return response;
	}
}

