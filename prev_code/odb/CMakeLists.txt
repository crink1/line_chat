#1.添加cmake版本说明
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(all_test)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test2)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})