cmake_minimum_required(VERSION 2.8)

#项目名称
project(libcoroutine)

set (CMAKE_VERBOSE_MAKEFILE false )

find_package(Protobuf REQUIRED)

#统一输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

include_directories(./build/pb/)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/pb/)

#公共编译选项
set (CMAKE_C_FLAGS 
"-g  -ggdb -Werror -Wall ")

set (CMAKE_CXX_FLAGS 
"-g  -ggdb -Werror -Wall -std=c++0x -Wno-literal-suffix")

## -Wl,-z,defs -Wshadow -Winline


ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(pb)
ADD_SUBDIRECTORY(test1)
ADD_SUBDIRECTORY(test2)
