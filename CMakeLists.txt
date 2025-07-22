cmake_minimum_required(VERSION 3.0)

project(SimpleMediaServer)

#内存检测设置
option(ENABLE_ASAN "Enable lib asan" false)

#第三方库设置，目前不能置为false，否则编译无法通过
option(ENABLE_OPENSSL "Enable openssl" true)

#项目设置
option(ENABLE_PROJECT_GB2818SIP "Enable test gb28181 sip" false)
option(ENABLE_PROJECT_TRANSCODEVIDEO "Enable test transcodeVideo" false)
option(ENABLE_PROJECT_TRANSCODEAUDIO "Enable test transcodeAudio" false)

#模块设置
option(ENABLE_SRT "Enable srt" true)
option(ENABLE_SRTCUSTOM "Enable srt custom" true)
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
option(ENABLE_EHOME2 "Enable ehome2 and rtp and mpeg" true)
option(ENABLE_EHOME5 "Enable ehome5 and rtp and mpeg" true)

#设置调试信息并开启c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
SET(CMAKE_EXE_LINKER_FLAGS " -no-pie") 
#set(BUILD_SHARED_LIBS OFF)  # 确保静态链接
#set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")

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
    # 设置srtp库的头文件目录
    set(SRTP_INCLUDE_DIR "./Thirdparty/libsrtp/include")
    # 设置srtp库的二进制文件目录
    set(SRTP_LIBRARY "./Thirdparty/libsrtp/lib/")
    # 包含srtp头文件目录
    include_directories(${SRTP_INCLUDE_DIR})
    # 定义要链接的srtp库
    link_directories(${SRTP_LIBRARY})
    list(APPEND LINK_LIB_LIST srtp2)

    # 设置sctp库的头文件目录
    set(SCTP_INCLUDE_DIR "./Thirdparty/libusrsctp/include")
    # 设置srt库的二进制文件目录
    set(SCTP_LIBRARY "./Thirdparty/libusrsctp/lib/")
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

if (ENABLE_OPENSSL)
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
endif ()

if (ENABLE_SRT)
    message(STATUS "开启libsrt")
    add_definitions(-DENABLE_SRT)
    # 设置srt库的头文件目录
    set(SRT_INCLUDE_DIR "./Thirdparty/libsrt/include")
    # 设置srt库的二进制文件目录
    set(SRT_LIBRARY "./Thirdparty/libsrt/lib/")
    # 包含srt头文件目录
    include_directories(${SRT_INCLUDE_DIR})
    # 定义要链接的srt库
    link_directories(${SRT_LIBRARY})
    list(APPEND LINK_LIB_LIST srt)
else()
    message(STATUS "未开启libsrt")
endif()

# aux_source_directory(. DIRSRCS)

include_directories(./ ./Base ./Base/Net ./Base/Log ./Base/EventPoller ./Src)
# link_directories(${PROJECT_SOURCE_DIR}/Base/Log)

add_subdirectory(Base)
add_subdirectory(Src)

add_executable(SimpleMediaServer main.cpp)
#target_link_libraries(SimpleMediaServer -static ${LINK_LIB_LIST} dl pthread)
target_link_libraries(SimpleMediaServer ${LINK_LIB_LIST} dl pthread)

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
    project(SimpleSipServer)
    add_subdirectory(GB28181SIP)
    add_executable(SimpleSipServer gb28181Sip.cpp)
    target_link_libraries(SimpleSipServer gb28181Sip src base ssl crypto dl pthread)
endif ()
