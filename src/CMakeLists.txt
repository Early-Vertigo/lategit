# 使用下行的缺点，一旦make产生makefile，之后新建新的源文件无法识别
#aux_source_directory(. SRC_LIST)

set(SRC_LIST mprpcapplication.cc mprpcconfig.cc rpcheader.pb.cc rpcprovider.cc mprpcchannel.cc mprpccontroller.cc logger.cc zookeeperutil.cc)
add_library(mprpc ${SRC_LIST})
# 依赖muduo库
# net是base的依赖，所以放在前面
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)