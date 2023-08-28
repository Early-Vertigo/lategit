#set(SRC_LIST userservice.cc ../user.pb.cc)
set(SRC_LIST friendservice.cc ../friend.pb.cc)

# 动态链接库
add_executable(provider ${SRC_LIST})
target_link_libraries(provider mprpc protobuf)