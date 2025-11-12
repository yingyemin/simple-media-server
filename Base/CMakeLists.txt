cmake_minimum_required(VERSION 3.0)
project(base)

#mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置 C++ 标准
#set(CMAKE_CXX_STANDARD 11)
#设置调试信息并开启c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 启用 ASan
if (ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif ()

#set(ENABLE_OPENSSL ON CACHE BOOL "enable openssl")

#find_package(OpenSSL QUIET)
#if (OPENSSL_FOUND AND ENABLE_OPENSSL)
#if (WIN32)
#    set(BASE_LIB_LIST WS2_32 Iphlpapi shlwapi)
#else ()
#    set(BASE_LIB_LIST)
#endif ()
#    message(STATUS "找到openssl库:\"${OPENSSL_INCLUDE_DIR}\",ENABLE_OPENSSL宏已打开")
#    include_directories(${OPENSSL_INCLUDE_DIR})
#    add_definitions(-DENABLE_OPENSSL)
#    list(APPEND BASE_LIB_LIST ${OPENSSL_LIBRARIES})
#endif ()

#设置调试信息并开启c++11标准
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(SUB_DIR_LIST "Net" "EventPoller" "Util" "Log" "Ssl" "WorkPoller")

foreach(SUB_DIR ${SUB_DIR_LIST})
    #遍历源文件
    #Traverse source file
    aux_source_directory(${SUB_DIR} SRC_LIST)
endforeach()

add_library(base STATIC ${SRC_LIST})

#测试程序
#Test program
#if(WIN32)
#    add_subdirectory(Tests)
#endif ()

