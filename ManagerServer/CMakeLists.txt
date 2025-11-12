cmake_minimum_required(VERSION 3.0)
project(Manager)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置 C++ 标准
#set(CMAKE_CXX_STANDARD 11)
#设置调试信息并开启c++11标准
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#设置调试信息并开启c++11标准
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(SUB_DIR_LIST "Common" "Http" "HttpApi" "Manager")

foreach(SUB_DIR ${SUB_DIR_LIST})
    #遍历源文件
    #Traverse source file
    aux_source_directory(${SUB_DIR} SRC_LIST)
endforeach()

add_library(Manager STATIC ${SRC_LIST})

