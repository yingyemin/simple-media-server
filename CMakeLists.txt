cmake_minimum_required(VERSION 3.0)
project(SimpleMediaServer)

option(ENABLE_ASAN "Enable lib asan" true)
option(ENABLE_OPENSSL "Enable openssl" true)

#设置调试信息并开启c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

if (ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    message(STATUS "已启用 Address Sanitize")
endif ()

#查找openssl是否安装
# find_package(OpenSSL QUIET)
# if (OPENSSL_FOUND AND ENABLE_OPENSSL)
#     message(STATUS "found library:${OPENSSL_LIBRARIES},ENABLE_OPENSSL defined")
#     include_directories(${OPENSSL_INCLUDE_DIR})
#     add_definitions(-DENABLE_OPENSSL)
#     list(APPEND LINK_LIB_LIST ${OPENSSL_LIBRARIES})
# else()
#     message(WARNING "openssl未找到，rtmp将不支持flash播放器，https/wss/rtsps/rtmps也将失效")
# endif ()

# 设置OpenSSL库的头文件目录
set(OPENSSL_INCLUDE_DIR "./Thirdparty/libopenssl/include")
# 设置OpenSSL库的二进制文件目录
set(OPENSSL_LIBRARY "./Thirdparty/libopenssl/lib/")
# 包含OpenSSL头文件目录
include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIBRARY})

# 设置srtp库的头文件目录
set(SRTP_INCLUDE_DIR "./Thirdparty/libsrtp/include")
# 设置srtp库的二进制文件目录
set(SRTP_LIBRARY "./Thirdparty/libsrtp/lib/")
# 包含srtp头文件目录
include_directories(${SRTP_INCLUDE_DIR})
# 定义要链接的srtp库
link_directories(${SRTP_LIBRARY})

# 设置srt库的头文件目录
set(SRT_INCLUDE_DIR "./Thirdparty/libsrt/include")
# 设置srt库的二进制文件目录
set(SRT_LIBRARY "./Thirdparty/libsrt/lib/")
# 包含srt头文件目录
include_directories(${SRT_INCLUDE_DIR})
# 定义要链接的srt库
link_directories(${SRT_LIBRARY})

aux_source_directory(. DIRSRCS)

include_directories(./ ./Base ./Base/Net ./Base/Log ./Base/EventPoller ./Src)
link_directories(${PROJECT_SOURCE_DIR}/Base/Log)

add_subdirectory(Base)
add_subdirectory(Src)

add_executable(SimpleMediaServer ${DIRSRCS})

target_link_libraries(SimpleMediaServer src base srtp2 srt ssl crypto dl pthread)

