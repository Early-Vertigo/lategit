# 技术栈
    集群和分布式概念以及原理
    RPC远程过程调用原理以及实现
    Protobuf数据序列化和反序列化协议
    ZooKeeper分布式一致性协调服务应用以及编程
    muduo网络库编程
    conf配置文件读取
    异步日志
    CMake构建项目集成编译环境
    github管理项目

# 集群和分布式

    集群下：
        1.受限于硬件资源，服务器承受用户的并发量有限，集群可以提升并发量
        2.任意模块的修改，都会导致整个项目代码重新编译、部署
        3.系统中，有些模块是属于CPU密集型，有些模块是IO密集型，造成各模块对硬件资源的需求是不一样的

        优点：用户的并发量提升了
        缺点：项目代码还是需要整体重新编译，而且需要多次部署

    集群：每一台服务器独立运行一个工程的所有模块。

    分布式：一个工程拆分了很多模块，每一个模块独立部署运行在一个服务器主机上，所有服务器协同工作共同提供服务，每一台服务器称作分布式的一个节点，根据节点的并发要求，对一个节点可以再做节点模块集群部署。

    大系统的软件模块该怎么划分？
        各模块可能会实现大量重复的代码
    各模块之间该怎么访问？
        节点1上的模块怎么调用节点2上的模块的业务方法？
        节点1上的模块进程1怎么调用节点1上的模块进程2里面的一个业务方法？

# RPC通信原理以及项目的技术选型

    RPC（Remote Procedure Call Protocol）远程过程调用协议
        涉及RPC方法参数的打包和解析，也就是数据的序列化和反序列化，使用protobuf
        网络部分，包括寻找rpc服务主机，发起rpc调用请求和响应rpc调用结果，使用muduo网络库和zookeeper服务配置中心(专门做服务发现)。

    protobuf相比json的好处：
        1.protobuf是二进制存储；xml和json都是文本存储。
            protobuf带宽资源利用效率更高
        2.protobuf不需要存储额外的信息；json存储需要存储额外的信息

# 网络I/O模型介绍

    accept + read/write
    不是并发服务器

    accept + fork - process-pre-connection
    适合并发连接数不大，计算任务工作量大于fork的开销

    accept + thread thread-pre-connection
    比方案2的开销小了一点，但是并发造成线程堆积过多

    muduo的设计：reactors in threads - one loop per thread
    方案的特点是one loop per thread，有一个main reactor（I/O）负载accept连接，然后把连接分发到某个sub reactor（Worker），该连接的所用操作都在那个sub reactor所处的线程中完成，多个连接可能被分派到多个线程中，以充分利用CPU。
    如果有过多的耗费CPU I/O的计算任务，可以创建新的线程专门处理耗时的计算任务。

    reactors in process - one loop pre process
    nginx服务器的网络模块设计，基于进程设计，采用多个Reactors充当I/O进程和工作进程，通过一把accept锁，完美解决多个Reactors的“惊群现象”。

# Protobuf安装配置

    protobuf（protocol buffer）是google 的一种数据交换的格式，它独立于平台语言。google 提供了protobuf多种语言的实现：java、c#、c++、go 和 python，每一种实现都包含了相应语言的编译器以及库文件。

    由于它是一种二进制的格式，比使用 xml（20倍） 、json（10倍）进行数据交换快许多。可以把它用于分布式应用之间的数据通信或者异构环境下的数据交换。作为一种效率和兼容性都很优秀的二进制数据传输格式，可以用于诸如网络传输、配置文件、数据存储等诸多领域。

# Protobuf演示

    // 数据 列表 映射表

* Protobuf演示1
    对于登录请求的序列化和反序列化
    创建test.proto根据protobuf定义的语法编写程序，test.proto
    protoc test.prto --cpp_out=./编译得到test.pb.cc test.pb.h
    main.cc中引用test.pb.h，书写业务逻辑：初始化类，类内数据，以及序列化和反序列化
        Loginrequest req; // 初始化登录请求类
        req.set_name; req.set_pwd; // 按照test.proto中的字段进行设置
        req.SerializeToString(&send_str) // 对象数据序列化 

        Loginrequest reqB; // 初始化登录请求类
        reqB.LoginRequest::ParseFromString(send_str); // 从send_str反序列化一个login请求对象

* Protobuf演示2
    定义列表类型

* Protobuf演示3+4
    Protobuf不提供通信功能，只提供序列化和反序列化的功能
    
    在protobuf里面怎么定义rpc方法的类型 - service
        // option 选项
        // 定义下面的选项，表示生成service服务类和rpc方法描述，默认不生成，必须加入Option 
        option cc_generic_services = true;

        // 在protobuf里面怎么定义描述rpc方法的类型 - service
        // 定义服务类，以及服务类的方法
        service UserServiceRpc
        {
            rpc Login(LoginRequest) returns (LoginResponse);
            rpc GetFriendLists(GetFriendListsRequest) returns (GetFriendListsResponse);
        }

    .proto文件中:           .cc文件中
        message LoginRequest -> .cc class LoginRequest : public :: google :: protobuf :: Message
                                name() pwd() set_name() set_pwd()

        message LoginResponse -> .cc class LoginResponse : public google :: protobuf :: Message
                                成员变量的读写操作方法

    service UserServiceRpc
    {
        rpc Login(LoginRequest) returns(LoginResponse);
    }
        该service生成的.cc文件中的class UserServiceRpc继承于protobuf::service，并且继承了2个虚函数(因为service中定义了两个方法)和1个const常量GetDescriptor()用于描述当前服务所拥有的方法

        class UserServiceRpc叫做Callee ServiceProvider rpc服务提供者

    class UserServiceRpc_Stub : public UserServiceRpc

        class UserServiceRpc_Stub叫做Caller ServiceConsumer rpc服务消费者
        该类下的有一个构造函数UserServiceRpc_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
        也有两个方法：void Login，void GetFriendLists，是由基类的虚函数继承而来重写的函数，两个函数都是通过调用CallMethod实现的
            channel_->CallMethod(descriptor()->method(0),controller, request, response, done);
            channel_->CallMethod(descriptor()->method(1),controller, request, response, done);       
            这两个CallMethod都是继承于基类(service.h下的RpcChannel，仅含有纯虚函数的抽象类)的虚函数virtual void CallMethod()函数    
        class UserServiceRpc_Stub中有一个成员变量：::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;

    总结流程：
        message定义的类型是专门用作rpc方法参数的序列化和反序列化，这些类型都是由public google :: protobuf :: Message继承而来

        定义的描述的方法的服务类service会生成两个类，一个是同名类，一个是同名_stub类
            同名类里面有同数量的虚函数，以及GetDescriptor()用于描述，不用传参
            同名_stub代理类角色类似(User_stub,调用方的桩类、代理类),主要负责rpc调用数据的序列化，以及接受响应等，需要传入对象，传入的对象是::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;
            
# 本地服务怎么发布rpc方法
    比较重要
    文件:
        example/callee
        example/user.proto
* 部分1
    先做本地服务，再改为rpc方法

* 部分2
    实例文件：
        ./src/include/mprpcapplication.h & rpcprovider.h
        ./src/mprpcapplication.cc & rpcprovider.cc
        ./example/callee/userservice.cc
    // 理想的使用方式
        int main()(int argc, char ** argv)
        {
            // 调用框架的初始化操作
            MprpcApplication::Init(argc,argv);

            // provider是一个rpc网络服务对象，把UserService对象发布到rpc节点上
            // 把UserService对象发布到rpc节点上
            RpcProviderr provider; // 在框架上发布服务
            provider.NotifyService(new UserService());
            provider.NotifyService(new ProductService());

            // 启动一个rpc服务节点，run以后进入阻塞状态，等待远程的rpc调用请求
            provider.Run();

            return 0;
        }

# Mprpc框架项目动态库编译

    要将本地服务变为可远程分布的rpc服务要先：
        定义类型，描述方法的名字、类型，使得rpc调用方和提供方有协议
    框架的搭建
    框架的使用
        调用框架的初始化方法
        启动rpc服务节点的方法

    本节：
        完善调用框架的初始化方法Mprpcapplication::Init()
        完善CMakeLists使用CMake编译

# Mprpc框架的配置文件加载

* 一
* 二
    加入了mprpcconfig.h mprpcconfig.cc 框架读取配置文件类
    完善了Init()中对conf文件的读取
    完善了CMakeLists，同时发现了CMakeLists中使用链接库(example)
        # 使用下行的缺点，一旦make产生makefile，之后新建新的源文件无法识别
        aux_source_directory(. SRC_LIST)

# 开发RpcProvider的网络服务
    通过使用Muduo网络库，完成了
        1.TCP连接
        2.服务端网络地址信息存储
        3.创建TcpServer对象
        4.设置muduo库的线程数量
        5.启动网络服务
        6.新的socket连接回调 RpcProvider::OnConnection
        7.已建立连接用户的读写时间回调 RpcProvider::Message

# RpcProvider发布服务方法

* 一
    服务端-Rpc提供方
        RpcApplication::Init()
            加载配置文件
        RpcProvider
            网络功能-通过muduo库实现(网络收发)
            序列化和反序列化-protobuf
            发布功能-NotifyService(Service*)
                接受一个rpc调用请求时，它怎么知道要调用应用程序的哪个服务对象的哪个rpc方法呢？(比如要使用UserServiceRpc服务对象的Login方法)
                NotifyService需要生成一张表，记录服务对象和其发布的所有服务方法
                    Service(描述对象)             Method类(描述方法)
                    UserService:                    Login Register
                    FriendService:          AddFriend DelFriend GetFriendList

    RpcProvider使用者(将本地服务发送到rpc)，Rpc服务方法的发布方
        class UserService : public UserServiceRpc
        {
            login <=本地方法
            login <= 重写protobuf提供的virtual虚函数
            {
                1.从LoginRequest获取参数的值
                2.执行本地服务login，并获取返回值
                3.用上面的返回值填写LoginResponse
                4.一个回调，把LoginResponse发送给rpc client
            }
        }
* 二
    代码实现
        RpcProvider::NotifyService
        如何发布，以及NotifyService的作用

    总结：

# RpcProvider分发RPC服务

* 一
* 二

    首先，服务的提供方Callee，首先通过RpcProvider注册服务对象和方法，通过Protobuf的抽象层的Service和Method，把服务对象和服务方法记录在Map表当中，启动后，相当于启动了Epoll的服务器，启动后可以接受远程连接。
    如果由连接进来，那么服务端收到相应连接后，OnMessage相当于RPCRuntime处等待，数据到达后根据协商好的方式解析得到对象和方法，再通过抽象层动态生成method的请求和相应，并且记录。
    在框架上调用业务的方法(Login方法)，从请求里面拿数据做业务，做response和回调，绑定方法是将响应进行序列化发送给服务的调用者，发送结束后将短连接服务断开。

# RpcChannel的调用过程
    Rpc调用方-caller

# 实现RPC方法的调用过程

* 一
    calluserservice.cc 中 服务调用者使用rpc方法，stub.Login();
* 二
    完善mprpcChannel.cc 中服务调用方整个流程的部分，包括
        rpc请求的数组组织， 数据的序列化
        发送rpc请求 wait
        接受rpc响应
        完成反序列化

# 点对点rpc通信功能测试
    example下实现了rpc的发布和caller下的Rpc方法的调用

# Mprpc框架的应用示例
    新增业务Register
        1.在user.proto中声明
            message RegisterRequest
            message RegisterResponse
        2.编译user.proto 
            by using "protoc user.proto --cpp_out=./"
        3.在user.pb.h寻找到Register提供的虚函数
        4.在userservice.cc中重写虚函数方法，得到想要的Register方法
            bool Register(uint32_t id, std::string name, std::string pwd)
            void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
        5.在calluserservice.cc中调用远程发布的rpc方法Register
            通过
                调用底层RpcChannel的callMethod，即RpcChannel->RpcChannel::callMethod，集中来做所有rpc方法调用的参数序列化和网络发送
                rpc方法的请求参数
                定义响应
                以同步的方式发起rpc调用请求，等待返回结果
                一次rpc调用完成，读取调用结果，即读取响应
            完成对Register方法的调用过程编程

# RpcController控制模块实现
    没有控制模块前：
        RPC服务的调用方，在调用时：
            1.先定义一个代理对象(stub),传一个channel()，框架提供该功能
            2.填入参数
            3.调用stub的方法，即发起rpc方法的调用 MprpcChannel::callMethod
        问题：
            第3步认为是一定成功的，直接认为errcode()=0后获取响应，即理想化callMethod中的逻辑是一定成功的
        实际情况：
            因为当用代理对象调用rpc方法的时候，该过程都转到Channel::callMethod的逻辑中
            callMethod主要负责：    
                rpc请求的序列化
                网络发送
                接受响应
                反序列化
                关闭网络连接
            得到callMethod的响应后，回到call中(服务调用里)查看response的具体情况
            但是，在callMethod中，如果序列化失败(以及该流程中任意一个部分失败)，都会直接返回，跳过了后续的步骤，则这个时候返回到call中查看response是无效的
        解决情况：
            fixbug::FriendServiceRpc_Stub::GetFriendsList(google::protobuf::RpcController *controller, const fixbug::GetFriendsListRequest *request, fixbug::GetFriendsListResponse *response, google::protobuf::Closure *done)
            中的第一个参数即controller，是为了查看stub中(即callMethod)的运行状态
        如何使用：
            创建mprpccontroller.h .cc 继承自service.h下的抽象类
            在mprpcchannel.cc中使用
                错误信息填入到controller->SetFailed(to be filled); to be filled中

# logger日志系统设计实现

* 一
                Mprpc框架
    rpc请求->RpcProvider->写日志信息->queue队列结构->写日志线程，磁盘I/O->log.txt
    rpc响应<-RpcChannel

    使用queue结构是因为读写是内存操作，速度快
    将磁盘I/O操作从RpcProvider业务中剥离，避免避免时间，放在其他业务中(磁盘I/O较慢，放在这里可以避免rpc业务中占用时间)
    队列必须保证线程安全(因为RpcProvider支持epoll+多线程)
    不引用互斥锁，因为抢锁会拖慢多个线程写日志的速度
    队列为空写日志应该是一直等待，不为空应该是优先写日志，所以需要涉及到线程间通信    
    log.txt保存在当前目录下Log内
    使用kafka可以作为日志系统的快速调用，但本项目不涉及

* 二
    创建lockqueue.h编写异步写日志的日志队列(queue)

# Zookeeper分布式协调服务

    32-讲解面试和沟通的技巧

    需要在分布式环境中记录每个节点的IP地址和端口号
    思路式锁
    rpc节点-部署服务-使用Zookeeper协调服务
        分布式环境中全局命名服务
        服务注册中心
        全局分布式锁

# ZK服务配置中心介绍和znode节点介绍
    zk的数据是怎么组织的-znode节点

    znode
        节点携带1MB的数据
    
    永久性节点和临时性节点
    
    sudo netstat -tanp
        tcp6       0      0 :::2181                 :::*                    LISTEN      195923/java
    
    zk客户端常用命令
        ls、get、create、set、delete

    心跳消息：
        建立TCP连接的双方，在网络环境比较复杂的时候，网络节点发生异常，并没有受到socket异常的挥手消息，此时可以维护一个心跳
        设置心跳的频率，通过心跳的反馈判断网络连接的情况
    
    session会话：
        存储了客户端和服务端所有的会话记录(待补充)

    zookeeper节点：
        对于永久性节点(zookeeper查看节点信息中ZOO_EPHEMERAL,为0x0)，如果rpc节点超时未发送心跳，zk不会删除这个节点。
        对于临时性节点，rpc节点超时未发送心跳消息，zk会自动删除临时性节点。

# zk的watcher机制和原生API安装

    示例：

        当客户端想要调用rpc服务的时候，会查询以rpc名字和方法名字组成路径查询节点路径，如果存在的话得到节点的ip地址和端口。
        如果没有则没有提供该项服务，不可调用该服务。

        watcher机制，即通知回调机制。客户端可以通过API添加一个watcher(监听器)，监听特定的事件类型(如节点的变化)，在客户端处维护一个map表，键是znode节点的名字，值是znode节点携带的内容。客户端对某个父节点(即服务名)添加一个watcher，watcher则对父节点的子节点监听变化，当节点数量、状况发生变化，watcher监听后会主动向客户端发送通知。

    原生ZkClient API存在的问题

        Zookeeper原生提供了C和Java的客户端编程接口，但是使用起来相对复杂，几个弱点：

        1.不会自动发送心跳消息 <==== 错误，源码上会在1/3的Timeout时间发送ping心跳消息
        2.设置监听watcher只能是一次性的，每次触发后需要重复设置
        3.znode节点只存储简单的byte字节数组，如果存储对象，需要自己转换对象生成字节数组
    
# 封装zookeeper的客户端类
    zookeeperutil.h
    zookeeperutil.cc(方法实现注意)

    src下CMakeLists.txt记得添加zookeeper_mt(多线程版本库)，另外也有zookeeper_st(单线程版本库)

# zk在项目上的应用实践

    服务提供方：在rpcprovider.cc中void RpcProvider::Run()加入zk的相关操作，包括注册服务的节点，以及服务下属方法的节点
    服务调用方：在rpcchannel.cc中void MprpcChannel::CallMethod中加入通过zookeeper读取配置文件rpcserver中方法的信息(ip port)

    通过加入zk，调用方通过连接zk注册中心获取方法的端口和ip后再直接连接服务提供方方法的主机后进行rpc服务

    运行抓包检查zookeeper发送的Ping包(心跳消息)
        sudo tcpdum -i lo port 2181(指定端口是2181)

# 项目总结以及编译脚本

    编译脚本：
        chmod 777 autobuild.sh // 
        ./autobuild.sh