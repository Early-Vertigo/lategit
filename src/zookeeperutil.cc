#include "zookeeperutil.h"
#include "mprpcapplication.h" // 框架的应用类
#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器  zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT) // 回调的消息类型是和会话相关的消息类型
    {
        if (state == ZOO_CONNECTED_STATE) // zkclient和zkserver连接成功
        {   // sem是Start()中给zhandle_t绑定的信号量
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem); // 给信号资源量+1
            // +1则得到新的信号量数值，接下来会返回给Start()中的sem_wait，以便完成Start()流程
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{

}

ZkClient::~ZkClient()
{
    if(m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 已经连接，关闭句柄，释放资源
    }
}

// 连接zkserver
// Start()的逻辑，连接server API是通过zookeeper_init(异步的)，通过注册的回调消息函数回调为ZOO_CONNECTED_STATE时认为注册成功。
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;
    /*
        zookeeper_mt:多线程版本
        zookeeper的API客户端程序提供了三个线程
            API调用线程
            网络I/O线程(通过pthread_create创建poll)
            watcher回调线程(通过pthread_create创建watcher)

        
        zhandle_t *zookeeper_init(const char *host, watcher_fn fn, int recv_timeout, const clientid_t *clientid, void *context, int flags)
            参数：host-记录 fn-回调函数 recv_timeout-会话的超时事件
    */
    // 创建本地句柄
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if(nullptr == m_zhandle)
    {
        // 成功与否都不代表连接是否成功，只是单纯反馈该函数的运行情况
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // 创建信号量
    sem_t sem;
    sem_init(&sem, 0, 0);
    // 给指定的句柄(m_zhandle)添加一些信息，即绑定信号量sem
    zoo_set_context(m_zhandle, &sem);
    // 等待信号量的反馈(通过global_watcher的回调的信号量的变化得知)
    sem_wait(&sem);
    std::cout << "zookeeper_init success ! "<< std::endl;
    // 连接成功
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 以同步方式检查该节点是否已存在
    // 判断path表示的znode节点是否存在，如果存在，就不再重复创建
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if(ZNONODE == flag) // ZNONODE-不存在
    {
        // 创建指定path的znode节点
        flag = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        // state表示创建节点选择使用临时性节点还是永久性节点
        if(flag == ZOK) // 创建成功
        {
            std::cout <<"znode create success... path : "<< path <<std::endl;
        }
        else // 创建失败
        {
            std::cout << "flag:" << flag << std::endl;
            std::cout <<"znode create error... path: "<< path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

// 根据指定的path获取znode节点的值
// 以同步方式获取值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    // zoo_get : gets the data associated with a node synchronously.
    // 以同步方式获取指定path下node的值
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if(flag != ZOK) // 指定path的节点访问失败
    {
        std::cout << "get znode error...path: "<< path << std::endl;
        return "";
    }
    else
    {
        return buffer;
    }
}