#1.添加cmake版本说明
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(test_odb)
#3.检查并生成odb框架代码-添加odb映射代码-检测框架代码文件是否生成-保存框架源码
set(odb_path ${CMAKE_CURRENT_SOURCE_DIR}/entity)
set(odb_files student.hxx)
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
add_executable(main ${src_files} ${odb_srcs})
#6.设置头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/entity)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#7.设置需要连接的库
target_link_libraries(main -lgflags -lodb-mysql -lodb -lodb-boost)
#8.设置安装路径
INSTALL(TARGETS main RUNTIME DESTINATION bin)