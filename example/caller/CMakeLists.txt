#set(SRC_LIST calluserservice.cc ../user.pb.cc)
set(SRC_LIST callfriendservice.cc ../friend.pb.cc)

# 动态链接库
add_executable(consumer ${SRC_LIST})
target_link_libraries(consumer mprpc protobuf)