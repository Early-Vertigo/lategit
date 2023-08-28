#include <iostream>
#include "mprpcchannel.h"
#include "rpcheader.pb.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <error.h>
#include "mprpcapplication.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "zookeeperutil.h"

/*
header_size + service_name method_name args_size + args
*/

// 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done)
{
    // 基本过程(rpc服务调用者调用stub中的Callmethod虚函数重写，即现在的Callmethod方法)
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name(); // service_name
    std::string method_name = method->name(); // method_name

    // 获取参数的序列化字符串长度 args_size
    std::string args_str;
    uint32_t args_size = 0;
    if(request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        //std::cout << "serialize request error!" << std::endl;
        controller->SetFailed("serialize request error!");
        return;
    }

    // 定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size  = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str)){
        header_size = rpc_header_str.size();
    }else{
        //std::cout << "serialize rpc header error! "<< std::endl;
        controller->SetFailed("serialize rpc header error! ");
        return;
    }

    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0,std::string((char*)&header_size,4)); // header_size
    send_rpc_str += rpc_header_str; //
    send_rpc_str += args_str; // 

    // 打印调试信息
    std::cout<< "================================================="<< std::endl;
    std::cout << "header_size: "<< header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout<< "================================================="<< std::endl;
    // 以上完成了rpc服务的调用方在local call, package argument(序列化)的部分

    // 以下完成调用方在RPCRuntime中transmitt传输的部分
    // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd){
        // std::cout << "create socket error! error : " << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt,"create socket error ! error:%d",errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 读取配置文件rpcserver的信息(配置zookeeper之前使用下两行)
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();
    // 组装zookeeper znode需要的path
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if(host_data == "")
    {
        controller->SetFailed(method_path + " is not exist !");
        return;
    }
    int idx = host_data.find(":");// 找到ip:port的:
    if(idx == -1) //没找到
    {
        controller->SetFailed(method_path + " address is invalid !");
        return;
    }
    // 获取zookeeper方法节点存储的ip port
    // port需要字符串转整数
    std::string ip = host_data.substr(0,idx);
    uint16_t port = atoi(host_data.substr(idx+1,host_data.size()-idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if(-1 == connect(clientfd,(struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        // std::cout << "connect socket error! error : " << errno << std::endl;
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt,"connect socket error! error : %d",errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 发送rpc请求
    if(-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(),0))
    {
        // std::cout << "send socket error! error : " << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt,"connect socket error! error : %d",errno);
        controller->SetFailed(errtxt);
    }

    // 接受rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1 == (recv_size = recv(clientfd, recv_buf, 1024,0)))
    {
        // std::cout << "recv socket error! error : " << errno << std::endl;
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt,"recv socket error! error :  %d",errno);
        controller->SetFailed(errtxt);
        return;
    }
    // 进行服务调用方的反序列化
    // 反序列化rpc调用的响应数据
    std::string response_str(recv_buf,0,recv_size);// bug出现问题，recv_buf遇到\0后面的数据存不下来了
    //if(!response->ParseFromString(response_str))
    if(!response->ParseFromArray(recv_buf, recv_size))
    {
        // std::cout << "Parse error ! response_str: "<< response_str << std::endl;
        close(clientfd);
        char errtxt[2048] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    close(clientfd);
}