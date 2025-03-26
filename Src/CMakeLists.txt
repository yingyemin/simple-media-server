cmake_minimum_required(VERSION 3.0)

#mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# aux_source_directory(. DIRSRCS)

###COMMON#####
message(STATUS "add common moudle")
project(common)
aux_source_directory("Common" SRC_COMMON)
aux_source_directory("Codec" SRC_COMMON)
add_library(common STATIC 
    ${SRC_COMMON}
)
list(INSERT LINK_LIB_LIST 0 common)

###MPEG########
if (ENABLE_MPEG OR ENABLE_RTSP)
    message(STATUS "add mpeg moudle")
    project(mpeg)
    add_definitions(-DENABLE_MPEG)
    aux_source_directory("Mpeg" SRC_MPEG)
    add_library(mpeg STATIC 
        ${SRC_MPEG}
    )
    list(INSERT LINK_LIB_LIST 0 mpeg)
else()
    message(STATUS "not add mpeg moudle")
endif ()

###RTP#####
if (ENABLE_RTSP OR ENABLE_RTP OR ENABLE_WEBRTC OR ENABLE_GB28181 OR ENABLE_EHOME5 OR ENABLE_EHOME2)
    message(STATUS "add rtp moudle")
    project(rtp)
    add_definitions(-DENABLE_RTP)
    aux_source_directory("Rtp" SRC_RTP)
    aux_source_directory("Rtp/Decoder" SRC_RTP)
    aux_source_directory("Rtp/Encoder" SRC_RTP)
    add_library(rtp STATIC 
        ${SRC_RTP}
    )
    list(INSERT LINK_LIB_LIST 0 rtp)
else()
    message(STATUS "not add rtp moudle")
endif ()

###MP4########
if (ENABLE_MP4)
    message(STATUS "add mp4 moudle")
    project(mp4)
    add_definitions(-DENABLE_MP4)
    aux_source_directory("Mp4" SRC_MP4)
    add_library(mp4 STATIC 
        ${SRC_MP4}
    )
    list(INSERT LINK_LIB_LIST 0 mp4)
else()
    message(STATUS "not add mp4 moudle")
endif ()

###GB28181########
if (ENABLE_GB28181)
    message(STATUS "add gb28181 moudle")
    project(gb28181)
    add_definitions(-DENABLE_GB28181)
    aux_source_directory("GB28181" SRC_GB28181)
    add_library(gb28181 STATIC 
        ${SRC_GB28181}
    )
    list(INSERT LINK_LIB_LIST 0 gb28181)
else()
    message(STATUS "not add gb28181 moudle")
endif ()

###RTMP#####
if (ENABLE_RTMP)
    message(STATUS "add rtmp moudle")
    project(rtmp)
    add_definitions(-DENABLE_RTMP)
    aux_source_directory("Rtmp" SRC_RTMP)
    aux_source_directory("Rtmp/Decode" SRC_RTMP)
    aux_source_directory("Rtmp/Encode" SRC_RTMP)
    add_library(rtmp STATIC 
        ${SRC_RTMP}
    )
    list(INSERT LINK_LIB_LIST 0 rtmp)
else()
    message(STATUS "not add rtmp moudle")
endif ()

###RTSP#####
if (ENABLE_RTSP)
    message(STATUS "add rtsp moudle")
    project(rtsp)
    add_definitions(-DENABLE_RTSP)
    aux_source_directory("Rtsp" SRC_RTSP)
    add_library(rtsp STATIC 
        ${SRC_RTSP}
    )
    list(INSERT LINK_LIB_LIST 0 rtsp)
else()
    message(STATUS "not add rtsp moudle")
endif ()

###Hls########
if (ENABLE_HLS)
    message(STATUS "add hls moudle")
    project(hls)
    add_definitions(-DENABLE_HLS)
    aux_source_directory("Hls" SRC_Hls)
    add_library(hls STATIC 
        ${SRC_Hls}
    )
    list(INSERT LINK_LIB_LIST 0 hls)
else()
    message(STATUS "not add hls moudle")
endif ()

###HOOK########
if (ENABLE_HOOK)
    message(STATUS "add hook moudle")
    project(hook)
    add_definitions(-DENABLE_HOOK)
    aux_source_directory("Hook" SRC_HOOK)
    add_library(hook STATIC 
        ${SRC_HOOK}
    )
    list(INSERT LINK_LIB_LIST 0 hook)
else()
    message(STATUS "not add hls moudle")
endif ()

###RTCP########
if (ENABLE_RTSP OR ENABLE_GB28181 OR ENABLE_WEBRTC)
    message(STATUS "add rtcp moudle")
    project(rtcp)
    add_definitions(-DENABLE_RTCP)
    aux_source_directory("Rtcp" SRC_RTCP)
    add_library(rtcp STATIC 
        ${SRC_RTCP}
    )
    list(INSERT LINK_LIB_LIST 0 rtcp)
else()
    message(STATUS "not add rtcp moudle")
endif ()

###Ehome2########
if (ENABLE_EHOME2)
    message(STATUS "add ehome2 moudle")
    project(ehome2)
    add_definitions(-DENABLE_EHOME2)
    aux_source_directory("Ehome2" SRC_Ehome2)
    add_library(ehome2 STATIC 
        ${SRC_Ehome2}
    )
    list(INSERT LINK_LIB_LIST 0 ehome2)
else()
    message(STATUS "not add ehome2 moudle")
endif ()

###Ehome5########
if (ENABLE_EHOME5)
    message(STATUS "add ehome5 moudle")
    project(ehome5)
    add_definitions(-DENABLE_EHOME5)
    aux_source_directory("Ehome5" SRC_Ehome5)
    add_library(ehome5 STATIC 
        ${SRC_Ehome5}
    )
    list(INSERT LINK_LIB_LIST 0 ehome5)
else()
    message(STATUS "not add ehome5 moudle")
endif ()

###JT1078########
if (ENABLE_JT1078)
    message(STATUS "add jt1078 moudle")
    project(jt1078)
    add_definitions(-DENABLE_JT1078)
    aux_source_directory("JT1078" SRC_JT1078)
    add_library(jt1078 STATIC 
        ${SRC_JT1078}
    )
    list(INSERT LINK_LIB_LIST 0 jt1078)
else()
    message(STATUS "not add jt1078 moudle")
endif ()

###Flv########
if (ENABLE_FLV)
    message(STATUS "add flv moudle")
    project(flv)
    add_definitions(-DENABLE_FLV)
    aux_source_directory("Flv" SRC_Flv)
    add_library(flv STATIC 
        ${SRC_Flv}
    )
    list(INSERT LINK_LIB_LIST 0 flv)
else()
    message(STATUS "not add flv moudle")
endif ()

###SRT########
if (ENABLE_SRT)
    message(STATUS "add srt moudle")
    project(srtapp)
    add_definitions(-DENABLE_SRT)
    aux_source_directory("Srt" SRC_Srt)
    add_library(srtapp STATIC 
        ${SRC_Srt}
    )
    list(INSERT LINK_LIB_LIST 0 srtapp)
else()
    message(STATUS "not add srt moudle")
endif ()


###Record########
if (ENABLE_RECORD)
    message(STATUS "add record moudle")
    project(record)
    add_definitions(-DENABLE_RECORD)
    aux_source_directory("Record" SRC_Record)
    add_library(record STATIC 
        ${SRC_Record}
    )
    list(INSERT LINK_LIB_LIST 0 record)
else()
    message(STATUS "not add record moudle")
endif ()

###Ffmpeg########
if (ENABLE_FFMPEG)
    message(STATUS "add ffmpeg moudle")
    project(ffmpeg)
    add_definitions(-DENABLE_FFMPEG)
    aux_source_directory("Ffmpeg" SRC_Ffmpeg)
    add_library(ffmpeg STATIC 
        ${SRC_Ffmpeg}
    )
    list(INSERT LINK_LIB_LIST 0 ffmpeg)
else()
    message(STATUS "not add ffmpeg moudle")
endif ()

###HTTP and HttpStream########
if (ENABLE_HTTP OR ENABLE_API OR ENABLE_HLS)
    message(STATUS "add http moudle")
    project(http)
    add_definitions(-DENABLE_HTTP)
    aux_source_directory("Http" SRC_HTTP)
    aux_source_directory("HttpStream" SRC_HTTP)
    add_library(http STATIC 
        ${SRC_HTTP}
    )
    list(INSERT LINK_LIB_LIST 0 http)
else()
    message(STATUS "not add http moudle")
endif ()

###API########
if (ENABLE_API)
    message(STATUS "add api moudle")
    project(api)
    aux_source_directory("Api" SRC_API)
    add_library(api STATIC 
        ${SRC_API}
    )
    list(INSERT LINK_LIB_LIST 0 api)

    # 将 Git 版本信息传递给编译器
    target_compile_definitions(
        api
        PRIVATE
        GIT_VERSION="${GIT_VERSION}"
    )

    # 输出获取的 Git 版本信息
    message(STATUS "Git Version: ${GIT_VERSION}")
else()
    message(STATUS "not add api moudle")
endif ()

###Webrtc########
if (ENABLE_WEBRTC)
    message(STATUS "add webrtc moudle")
    project(webrtc)
    add_definitions(-DENABLE_WEBRTC)
    aux_source_directory("Webrtc" SRC_Webrtc)
    add_library(webrtc STATIC 
        ${SRC_Webrtc}
    )
    list(INSERT LINK_LIB_LIST 0 webrtc)
else()
    message(STATUS "not add webrtc moudle")
endif ()

# 获取 Git 版本信息
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LINK_LIB_LIST "${LINK_LIB_LIST}" PARENT_SCOPE)
