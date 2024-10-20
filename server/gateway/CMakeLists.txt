#1.添加cmake版本说明
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(gateway_server)

set(target "gateway_server")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#3.检查并生成proto框架代码-添加proto映射代码-检测框架代码文件是否生成-保存框架源码
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
set(proto_files base.proto user.proto message.proto friend.proto file.proto transmit.proto gateway.proto message.proto notify.proto speech.proto)
set(proto_h "")
set(proto_cc "")
set(proto_srcs "")

foreach(proto_file ${proto_files})
    string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
    string(REPLACE ".proto" ".pb.h" proto_h ${proto_file})

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${proto_cc})
        add_custom_command(
            PRE_BUILD 
            COMMAND protoc
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} --experimental_allow_proto3_optional ${proto_path}/${proto_file}
            DEPENDS ${proto_path}/${proto_file}
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
            COMMENT "生成protobuf框架文件:" ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
        )
    endif()
#4.获取源码目录下的所有文件
    list(APPEND proto_srcs ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
endforeach()




set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
#5.声明目标依赖
add_executable(${target} ${src_files} ${proto_srcs})
target_link_libraries(${target} -lgflags  -lspdlog -lfmt -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api  -lodb-mysql -lodb -lodb-boost -lcpr -lelasticlient -lhiredis -lredis++ -lpthread -lboost_system -lcpprest -lcurl -lcrypto /usr/lib/x86_64-linux-gnu/libjsoncpp.so.1.8.4)



#6.设置头文件路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)


#8.设置安装路径
INSTALL(TARGETS ${target} RUNTIME DESTINATION bin)