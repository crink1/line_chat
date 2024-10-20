#1.添加cmake版本说明
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(friend_server)

set(target "friend_server")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#3.检查并生成proto框架代码-添加proto映射代码-检测框架代码文件是否生成-保存框架源码
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
set(proto_files base.proto user.proto message.proto friend.proto)
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

set(odb_path ${CMAKE_CURRENT_SOURCE_DIR}/../odb)
set(odb_files chat_session_member.hxx chat_session.hxx friend_apply.hxx relation.hxx)
set(odb_hxx "")
set(odb_cxx "")
set(odb_srcs "")

foreach(odb_file ${odb_files})
    string(REPLACE ".hxx" "-odb.hxx" odb_hxx ${odb_file})
    string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${odb_cxx})
        add_custom_command(
            PRE_BUILD 
            COMMAND odb
            ARGS -d mysql --generate-query --generate-schema --profile boost/date-time ${odb_path}/${odb_file}
            DEPENDS ${odb_path}/${odb_file}
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
            COMMENT "生成ODB框架文件:" ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
        )
    endif()
#4.获取源码目录下的所有文件
    list(APPEND odb_srcs ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})
endforeach()


set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
#5.声明目标依赖
add_executable(${target} ${src_files} ${proto_srcs} ${odb_srcs})
target_link_libraries(${target} -lgflags  -lspdlog -lfmt -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl -lcrypto /usr/lib/x86_64-linux-gnu/libjsoncpp.so.1.8.4 -lodb-mysql -lodb -lodb-boost -lcpr -lelasticlient)

set(test_client "friend_client")
set(test_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_files)
add_executable(${test_client} ${test_files} ${proto_srcs})
target_link_libraries(${test_client} -lgflags -lgtest -lspdlog -lfmt -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl -lcrypto /usr/lib/x86_64-linux-gnu/libjsoncpp.so.1.8.4)


#6.设置头文件路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)


#8.设置安装路径
INSTALL(TARGETS ${target} ${test_client} RUNTIME DESTINATION bin)