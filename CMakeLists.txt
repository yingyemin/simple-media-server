cmake_minimum_required(VERSION 3.0)

project(SimpleMediaServer)
# set(DEFAULT_ENABLE_GB28181_SIP true)
# set(DEFAULT_ENABLE_SRT true)
# set(DEFAULT_ENABLE_EHOME2 true)
# set(DEFAULT_ENABLE_EHOME5 true)
# set(DEFAULT_ENABLE_EHOME2 true) # ehome2 也不默认启用
# set(DEFAULT_ENABLE_EHOME5 true)
# set(DEFAULT_ENABLE_CUSTOM_SRT true)
# if(WIN32)
#     set(DEFAULT_ENABLE_GB28181_SIP false)
#     set(DEFAULT_ENABLE_SRT false)   # 假设 Windows 上不默认启用 SRT
# 	set(DEFAULT_ENABLE_CUSTOM_SRT false)
# endif()
#内存检测设置
option(ENABLE_ASAN "Enable lib asan" false)

#第三方库设置，目前不能置为false，否则编译无法通过
option(ENABLE_OPENSSL "Enable openssl" true)

#项目设置
option(ENABLE_PROJECT_SIMPLE_MEDIA_SERVER "Enable simple media server" true)
option(ENABLE_PROJECT_SIMPLE_SIP_SERVER "Enable simple sip server" false)
option(ENABLE_PROJECT_GB2818SIP "Enable gb28181 sip server" true)
option(ENABLE_PROJECT_MANAGER_SERVER "Enable manager server" true)
option(ENABLE_PROJECT_TRANSCODEVIDEO "Enable test transcodeVideo" false)
option(ENABLE_PROJECT_TRANSCODEAUDIO "Enable test transcodeAudio" false)


option(ENABLE_SRT "Enable SRT protocol" true)
#option(ENABLE_SRTCUSTOM "Enable srt custom" true)
option(ENABLE_HTTP "Enable http and httpstream" true)
option(ENABLE_RTSP "Enable rtsp and rtp and mpeg and rtcp" true)
option(ENABLE_GB28181 "Enable gb28181 and rtp and mpeg and rtcp" true)
option(ENABLE_RTMP "Enable rtmp" true)
option(ENABLE_JT1078 "Enable jt1078" true)
option(ENABLE_JT808 "Enable jt808" true)
option(ENABLE_HLS "Enable hls and http" true)
option(ENABLE_API "Enable api and http" true)
option(ENABLE_HOOK "Enable hook" true)
option(ENABLE_FFMPEG "Enable ffmpeg" false)
option(ENABLE_WEBRTC "Enable webrtc and rtp and rtcp" true)
option(ENABLE_RTP "Enable rtp and mpeg" true)
option(ENABLE_RTCP "Enable rtcp" true)
option(ENABLE_RECORD "Enable record" true)
option(ENABLE_MPEG "Enable mpeg" true)
option(ENABLE_MP4 "Enable mp4" true)
option(ENABLE_FLV "Enable flv" true)
option(ENABLE_EHOME2   "Enable eHome2 and rtp/mpeg" true)
option(ENABLE_EHOME5   "Enable eHome5 and rtp/mpeg" true)
option(ENABLE_SRTCUSTOM "Enable Custom  Srt" true)
option(ENABLE_URING "Enable io_uring" false)
#设置调试信息并开启c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -std=c++17")
SET(CMAKE_EXE_LINKER_FLAGS " -no-pie") 
#set(BUILD_SHARED_LIBS OFF)  # 确保静态链接
#set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")

# 生成 compile_commands.json 方便 vscode-clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LINK_LIB_LIST base)

###RTSP#####
if (ENABLE_RTSP)
    add_definitions(-DENABLE_RTSP)
    set(ENABLE_RTP true CACHE BOOL "Enable rtp and mpeg" FORCE)
    set(ENABLE_MPEG true CACHE BOOL "Enable mpeg" FORCE)
    set(ENABLE_RTCP true CACHE BOOL "Enable rtcp" FORCE)
endif ()

###MP4########
if (ENABLE_MP4)
    add_definitions(-DENABLE_MP4)
endif ()

###Ehome2########
if (ENABLE_EHOME2)
    add_definitions(-DENABLE_EHOME2)
    set(ENABLE_RTP true CACHE BOOL "Enable rtp and mpeg" FORCE)
    set(ENABLE_MPEG true CACHE BOOL "Enable mpeg" FORCE)
    set(ENABLE_RTCP true CACHE BOOL "Enable rtcp" FORCE)
endif ()

###Ehome5########
if (ENABLE_EHOME5)
    add_definitions(-DENABLE_EHOME5)
    set(ENABLE_RTP true CACHE BOOL "Enable rtp and mpeg" FORCE)
    set(ENABLE_MPEG true CACHE BOOL "Enable mpeg" FORCE)
    set(ENABLE_RTCP true CACHE BOOL "Enable rtcp" FORCE)
endif ()

###GB28181########
if (ENABLE_GB28181)
    add_definitions(-DENABLE_GB28181)
    set(ENABLE_RTP true CACHE BOOL "Enable rtp and mpeg" FORCE)
    set(ENABLE_MPEG true CACHE BOOL "Enable mpeg" FORCE)
    set(ENABLE_RTCP true CACHE BOOL "Enable rtcp" FORCE)
endif ()

if (ENABLE_WEBRTC)
    message(STATUS "开启WEBRTC，添加srtp和usrsctp")
	if(WIN32)
	    # 设置srtp库的头文件目录
		set(SRTP_INCLUDE_DIR "./Thirdparty/libsrtp/include/srtp_win")
		# 设置srtp库的二进制文件目录
		set(SRTP_LIBRARY "./Thirdparty/libsrtp/lib/srtp2_win")

        
        # 设置sctp库的头文件目录
        set(SCTP_INCLUDE_DIR "./Thirdparty/libusrsctp/include")
        # 设置srt库的二进制文件目录
        set(SCTP_LIBRARY "./Thirdparty/libusrsctp/lib/win/")
	else()
		# 设置srtp库的头文件目录
		set(SRTP_INCLUDE_DIR "./Thirdparty/libsrtp/include")
		# 设置srtp库的二进制文件目录
		set(SRTP_LIBRARY "./Thirdparty/libsrtp/lib/")

        
        # 设置sctp库的头文件目录
        set(SCTP_INCLUDE_DIR "./Thirdparty/libusrsctp/include")
        # 设置srt库的二进制文件目录
        set(SCTP_LIBRARY "./Thirdparty/libusrsctp/lib/")
	endif()
    # 包含srtp头文件目录
    include_directories(${SRTP_INCLUDE_DIR})
    # 定义要链接的srtp库
    link_directories(${SRTP_LIBRARY})
    list(APPEND LINK_LIB_LIST srtp2)

    # 包含srt头文件目录
    include_directories(${SCTP_INCLUDE_DIR})
    # 定义要链接的srt库
    link_directories(${SCTP_LIBRARY})
    list(APPEND LINK_LIB_LIST usrsctp)
    
    add_definitions(-DENABLE_WEBRTC)
    set(ENABLE_RTP true CACHE BOOL "Enable rtp and mpeg" FORCE)
    set(ENABLE_RTCP true CACHE BOOL "Enable rtcp" FORCE)
else()
    message(STATUS "未开启WEBRTC")
endif ()

###RTMP#####
if (ENABLE_RTMP)
    add_definitions(-DENABLE_RTMP)
endif ()

###Hls########
if (ENABLE_HLS)
    add_definitions(-DENABLE_HLS)
    set(ENABLE_HTTP true CACHE BOOL "Enable http and httpstream" FORCE)
endif ()

###HOOK########
if (ENABLE_HOOK)
    add_definitions(-DENABLE_HOOK)
endif ()

###RTCP########
if (ENABLE_RTCP)
    add_definitions(-DENABLE_RTCP)
endif ()

###JT1078########
if (ENABLE_JT1078)
    add_definitions(-DENABLE_JT1078)
endif ()

###JT808########
if (ENABLE_JT808)
    add_definitions(-DENABLE_JT808)
endif ()

###Flv########
if (ENABLE_FLV)
    add_definitions(-DENABLE_FLV)
endif ()

###Record########
if (ENABLE_RECORD)
    add_definitions(-DENABLE_RECORD)
endif ()

###RTP#####
if (ENABLE_RTP)
    add_definitions(-DENABLE_RTP)
endif ()

###MPEG########
if (ENABLE_MPEG)
    add_definitions(-DENABLE_MPEG)
endif ()

###API########
if (ENABLE_API)
    add_definitions(-DENABLE_API)
    set(ENABLE_HTTP true CACHE BOOL "Enable http and httpstream" FORCE)
endif ()

###HTTP and HttpStream########
if (ENABLE_HTTP)
    add_definitions(-DENABLE_HTTP)
endif ()

###SRT Custom########
if (ENABLE_SRTCUSTOM)
    add_definitions(-DENABLE_SRTCUSTOM)
endif ()

if (ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    message(STATUS "已启用 Address Sanitize")
else()
    message(STATUS "停用 Address Sanitize")
endif ()

if (ENABLE_FFMPEG)
    message(STATUS "开启FFMPEG")
    add_definitions(-DENABLE_FFMPEG)
    # 设置ffmpeg库的头文件目录
    set(FFMPEG_INCLUDE_DIR "./Thirdparty/libffmpeg/include")
    # 设置ffmpeg库的二进制文件目录
    set(FFMPEG_LIBRARY "./Thirdparty/libffmpeg/lib/")
    # 包含ffmpeg头文件目录
    include_directories(${FFMPEG_INCLUDE_DIR})
    # 定义要链接的srtp库
    link_directories(${FFMPEG_LIBRARY})
    list(APPEND LINK_LIB_LIST avformat avfilter avcodec swresample swscale postproc avutil z x264 x265 aom opus mp3lame fdk-aac vpx bz2 m freetype lzma drm)
else()
    message(STATUS "未开启FFMPEG")
endif ()
if(WIN32)
# 防止 Windows.h 包含 Winsock.h
  list(APPEND COMPILE_DEFINITIONS
    WIN32_LEAN_AND_MEAN MP4V2_NO_STDINT_DEFS
    # 禁用警告
    _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
endif()
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
#set(OPENSSL_INCLUDE_DIR "./Thirdparty/libopenssl/include")
# 设置OpenSSL库的二进制文件目录
#set(OPENSSL_LIBRARY "./Thirdparty/libopenssl/lib/")
# 包含OpenSSL头文件目录
#include_directories(${OPENSSL_INCLUDE_DIR})
#link_directories(${OPENSSL_LIBRARY})
#list(APPEND LINK_LIB_LIST ssl crypto)

#查找openssl是否安装
#Find out if openssl is installed
if (WIN32 AND ENABLE_OPENSSL)
    find_package(OpenSSL QUIET)
    if (OPENSSL_FOUND AND ENABLE_OPENSSL)
        message(STATUS "找到openssl库:\"${OPENSSL_INCLUDE_DIR}\",ENABLE_OPENSSL宏已打开")
        include_directories(${OPENSSL_INCLUDE_DIR})
        add_definitions(-DENABLE_OPENSSL)
        list(APPEND LINK_LIB_LIST ${OPENSSL_LIBRARIES})
    else()
        # 手动设置 OpenSSL 路径（适用于本地自定义目录）
        set(OPENSSL_ROOT_DIR "./Thirdparty/libopenssl_win")
		# if(NOT EXISTS "${OPENSSL_ROOT_DIR}")
        #     message(FATAL_ERROR "OpenSSL  dir not found: ${OPENSSL_ROOT_DIR}")
        # endif()
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
        #set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")
        set(OPENSSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/")
        # if(NOT EXISTS "${OPENSSL_INCLUDE_DIR}")
        #     message(FATAL_ERROR "OpenSSL include dir not found: ${OPENSSL_INCLUDE_DIR}")
        # endif()

        #if(NOT EXISTS "${OPENSSL_LIBRARIES}")
            #message(FATAL_ERROR "OpenSSL libraries not found: ${OPENSSL_LIBRARIES}")
        #endif()
        # 添加手动指定的头文件路径和宏定义
        include_directories(${OPENSSL_INCLUDE_DIR})
        link_directories(${OPENSSL_LIBRARY})

        add_definitions(-DENABLE_OPENSSL)
        list(APPEND LINK_LIB_LIST ssl-3-x64 crypto-3-x64)
    endif ()
elseif (ENABLE_OPENSSL)
    message(STATUS "开启openssl")
    add_definitions(-DENABLE_OPENSSL)
    # 设置OpenSSL库的头文件目录
    set(OPENSSL_INCLUDE_DIR "./Thirdparty/libopenssl/include")
    # 设置OpenSSL库的二进制文件目录
    set(OPENSSL_LIBRARY "./Thirdparty/libopenssl/lib/")
    # 包含OpenSSL头文件目录
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_directories(${OPENSSL_LIBRARY})
    list(APPEND LINK_LIB_LIST ssl crypto)
else()
    message(STATUS "未开启openssl")
endif()

if (ENABLE_SRT)
    message(STATUS "开启libsrt")
    add_definitions(-DENABLE_SRT)
    if (WIN32)
        # 设置srt库的头文件目录
        set(SRT_INCLUDE_DIR "./Thirdparty/libsrt/include")
        # 设置srt库的二进制文件目录
        set(SRT_LIBRARY "./Thirdparty/libsrt/win_lib/")
    else()
        # 设置srt库的头文件目录
        set(SRT_INCLUDE_DIR "./Thirdparty/libsrt/include")
        # 设置srt库的二进制文件目录
        set(SRT_LIBRARY "./Thirdparty/libsrt/lib/")
    endif()
    # 包含srt头文件目录
    include_directories(${SRT_INCLUDE_DIR})
    # 定义要链接的srt库
    link_directories(${SRT_LIBRARY})
    list(APPEND LINK_LIB_LIST srt)
else()
    message(STATUS "未开启libsrt")
endif()

if (ENABLE_URING)
    message(STATUS "开启uring")
    add_definitions(-DENABLE_URING)

    list(APPEND LINK_LIB_LIST uring)
else()
    message(STATUS "未开启uring")
endif()

# aux_source_directory(. DIRSRCS)

include_directories(./ ./Base ./Base/Net ./Base/Log ./Base/EventPoller ./Base/Util)
# link_directories(${PROJECT_SOURCE_DIR}/Base/Log)

add_subdirectory(Base)

if (ENABLE_PROJECT_SIMPLE_MEDIA_SERVER)
    include_directories(./Src)
    message(STATUS "开启 SimpleMediaServer")
    add_subdirectory(Src)
    add_executable(SimpleMediaServer main.cpp)
    if(WIN32)
        target_link_libraries(SimpleMediaServer ${LINK_LIB_LIST} WS2_32 Iphlpapi shlwapi)
    else()
        target_link_libraries(SimpleMediaServer ${LINK_LIB_LIST} dl pthread)
    endif()
    #target_link_libraries(SimpleMediaServer -static ${LINK_LIB_LIST} dl pthread)
endif()

if (ENABLE_PROJECT_SIMPLE_SIP_SERVER)
    message(STATUS "开启 SipServer")

    include_directories(./SipServer)

    message(STATUS "osip exosip")
    # 设置ffmpeg库的头文件目录
    set(EXOSIP_INCLUDE_DIR "./Thirdparty/libexosip2-5.3.0/include")
    # 设置ffmpeg库的二进制文件目录
    if(WIN32)
        set(EXOSIP_LIBRARY "./Thirdparty/libexosip2-5.3.0/lib-win/")
    else()
        set(EXOSIP_LIBRARY "./Thirdparty/libexosip2-5.3.0/lib-linux/")
    endif()
    # 包含ffmpeg头文件目录
    include_directories(${EXOSIP_INCLUDE_DIR})
    # 定义要链接的srtp库
    link_directories(${EXOSIP_LIBRARY})

    list(INSERT LINK_LIB_LIST 0 SipServer eXosip2 osip2 osipparser2)

    add_subdirectory(SipServer)
    add_executable(SimpleSipServer sipServer.cpp)
    if(WIN32)
        target_link_libraries(SimpleSipServer ${LINK_LIB_LIST} WS2_32 Iphlpapi shlwapi)
    else()
        target_link_libraries(SimpleSipServer ${LINK_LIB_LIST} dl pthread resolv)
    endif()
    #target_link_libraries(SimpleMediaServer -static ${LINK_LIB_LIST} dl pthread)
endif()

if (ENABLE_PROJECT_TRANSCODEVIDEO)
    project(transcodeVideo)
    add_executable(transcodeVideo Tests/transcode/transcodeVideo.cpp)
    target_link_libraries(transcodeVideo ${LINK_LIB_LIST} dl pthread)
endif ()

if (ENABLE_PROJECT_TRANSCODEAUDIO)
    project(transcodeAudio)
    add_executable(transcodeAudio Tests/transcode/transcodeAudio.cpp)
    target_link_libraries(transcodeAudio ${LINK_LIB_LIST} dl pthread)
endif ()

if (ENABLE_PROJECT_GB2818SIP)
    include_directories(./Src)

    project(GB28181SipServer)
    add_subdirectory(GB28181SIP)
    add_executable(GB28181SipServer gb28181Sip.cpp)
    if(WIN32)
        target_link_libraries(GB28181SipServer gb28181Sip http common base ${LINK_LIB_LIST} ssl crypto WS2_32 Iphlpapi shlwapi)
    else()
        target_link_libraries(GB28181SipServer gb28181Sip http common base ${LINK_LIB_LIST} ssl crypto dl pthread resolv)
    endif()
endif ()

if (ENABLE_PROJECT_MANAGER_SERVER)
    include_directories(./ManagerServer)

    project(SimpleManagerServer)
    add_subdirectory(ManagerServer)
    add_executable(SimpleManagerServer managerServer.cpp)
    list(INSERT LINK_LIB_LIST 0 Manager)
    if(WIN32)
        target_link_libraries(SimpleManagerServer ${LINK_LIB_LIST} WS2_32 Iphlpapi shlwapi)
    else()
        target_link_libraries(SimpleManagerServer ${LINK_LIB_LIST} dl pthread resolv)
    endif()
endif ()
