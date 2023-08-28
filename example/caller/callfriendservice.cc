#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

int main(int argc, char **argv)
{
    // calluserservice是主要关于业务，框架由callee提供
    // 提供方提供proto文件
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);


    // 演示调用远程发布的rpc方法Login(UserServiceRpc是提供给方法提供者使用的，UserServiceRpc_Stub是提供给方法使用者使用的)
    // UserServiceRpc_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::GetFriendsListRequest request;
    request.set_userid(1000);

    fixbug::GetFriendsListResponse response; // 定义响应

    // 初始化controller对象，给stub的callMethod传参
    MprpcController controller;

    // 调用底层RpcChannel的callMethod，即RpcChannel->RpcChannel::callMethod，集中来做所有rpc方法调用的参数序列化和网络发送
    // 发起rpc方法的调用， 同步的rpc调用过程 MprpcChannel::callMethod
    stub.GetFriendsList(&controller, &request, &response, nullptr); 

    // 一次rpc调用完成，读取调用结果，即读取响应
    // 加入controller的控制模块用于检测stub的逻辑是否正常运行
    if(controller.Failed()){
        // 具体的ErrorText()在mprpcchannel.cc中
        std::cout << controller.ErrorText() << std::endl;
    }else{
        if(response.result().errcode() == 0)
        {
            // 调用成功
            std::cout << "rpc GetFriendsList response : " << std::endl;
            std::cout << "Friends List print shown here: " <<  std::endl;
            int size = response.friends_size();
            for(int i = 0; i<size; i++){
                std::cout << "index : "<< (i+1) << ", name: " << response.friends(i) << std::endl;
            }
        }
        else{
         // 调用失败
            std::cout << "rpc GetFriendsList response error : " << response.result().errmsg() << std::endl;
        }
    }
    

    return 0;
}