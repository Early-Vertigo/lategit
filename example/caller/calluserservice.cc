#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // calluserservice是主要关于业务，框架由callee提供
    // 提供方提供proto文件
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);


    // 演示调用远程发布的rpc方法Login(UserServiceRpc是提供给方法提供者使用的，UserServiceRpc_Stub是提供给方法使用者使用的)
    // UserServiceRpc_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    fixbug::LoginResponse response; // 定义响应

    // 调用底层RpcChannel的callMethod，即RpcChannel->RpcChannel::callMethod，集中来做所有rpc方法调用的参数序列化和网络发送
    // 发起rpc方法的调用， 同步的rpc调用过程 MprpcChannel::callMethod
    stub.Login(nullptr, &request, &response, nullptr); 

    // 一次rpc调用完成，读取调用结果，即读取响应
    if(response.result().errcode() == 0)
    {
        // 调用成功
        std::cout << "rpc login response : "<< response.success() << std::endl;
    }
    else{
        // 调用失败
        std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;
    }

    // 演示调用远程发布的rpc方法Register
    // rpc方法的请求参数
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    // 定义响应
    fixbug::RegisterResponse rsp;
    // 以同步的方式发起rpc调用请求，等待返回结果
    // 调用底层RpcChannel的callMethod，即RpcChannel->RpcChannel::callMethod，集中来做所有rpc方法调用的参数序列化和网络发送
    stub.Register(nullptr, &req, &rsp, nullptr);
    // 一次rpc调用完成，读取调用结果，即读取响应
    if(rsp.result().errcode() == 0)
    {
        // 调用成功
        std::cout << "rpc register response : "<< rsp.success() << std::endl;
    }
    else{
        // 调用失败
        std::cout << "rpc register response error : " << rsp.result().errmsg() << std::endl;
    }


    return 0;
}