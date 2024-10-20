cmake_minimum_required(VERSION 3.1.3)
project(test)

add_executable(test main.cc)

set(target, "main")
set(src_files "")
list(APPEND src_files main.cc test.cc)
