cmake_minimum_required(VERSION 3.0)
project(base)

#mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置调试信息并开启c++11标准
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

set(SUB_DIR_LIST "Net" "EventPoller" "Util" "Log" "Ssl" "WorkPoller")

foreach(SUB_DIR ${SUB_DIR_LIST})
    #遍历源文件
    #Traverse source file
    aux_source_directory(${SUB_DIR} SRC_LIST)
endforeach()

add_library(base STATIC ${SRC_LIST})
