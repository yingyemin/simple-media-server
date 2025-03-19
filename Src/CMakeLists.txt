cmake_minimum_required(VERSION 3.0)

#mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# aux_source_directory(. DIRSRCS)

###COMMON#####
project(common)
aux_source_directory("Common" SRC_COMMON)
aux_source_directory("Codec" SRC_COMMON)
add_library(common STATIC 
    ${SRC_COMMON}
)
list(INSERT LINK_LIB_LIST 0 common)

###MPEG########
if (ENABLE_MPEG)
    project(mpeg)
    aux_source_directory("Mpeg" SRC_MPEG)
    add_library(mpeg STATIC 
        ${SRC_MPEG}
    )
    list(INSERT LINK_LIB_LIST 0 mpeg)
endif ()

###RTP#####
if (ENABLE_RTSP OR ENABLE_RTP OR ENABLE_WEBRTC OR ENABLE_GB28181 OR ENABLE_EHOME5 OR ENABLE_EHOME2)
    project(rtp)
    aux_source_directory("Rtp" SRC_RTP)
    aux_source_directory("Rtp/Decoder" SRC_RTP)
    aux_source_directory("Rtp/Encoder" SRC_RTP)
    add_library(rtp STATIC 
        ${SRC_RTP}
    )
    list(INSERT LINK_LIB_LIST 0 rtp)
endif ()

###MP4########
if (ENABLE_MP4)
    project(mp4)
    aux_source_directory("Mp4" SRC_MP4)
    add_library(mp4 STATIC 
        ${SRC_MP4}
    )
    list(INSERT LINK_LIB_LIST 0 mp4)
endif ()

###GB28181########
if (ENABLE_GB28181 OR ENABLE_EHOME5 OR ENABLE_EHOME2)
    project(gb28181)
    aux_source_directory("GB28181" SRC_GB28181)
    add_library(gb28181 STATIC 
        ${SRC_GB28181}
    )
    list(INSERT LINK_LIB_LIST 0 gb28181)
endif ()

###RTMP#####
if (ENABLE_RTMP)
    project(rtmp)
    aux_source_directory("Rtmp" SRC_RTMP)
    aux_source_directory("Rtmp/Decode" SRC_RTMP)
    aux_source_directory("Rtmp/Encode" SRC_RTMP)
    add_library(rtmp STATIC 
        ${SRC_RTMP}
    )
    list(INSERT LINK_LIB_LIST 0 rtmp)
endif ()

###RTSP#####
if (ENABLE_RTSP)
    project(rtsp)
    aux_source_directory("Rtsp" SRC_RTSP)
    add_library(rtsp STATIC 
        ${SRC_RTSP}
    )
    list(INSERT LINK_LIB_LIST 0 rtsp)
endif ()

###Hls########
if (ENABLE_HLS)
    project(hls)
    aux_source_directory("Hls" SRC_Hls)
    add_library(hls STATIC 
        ${SRC_Hls}
    )
    list(INSERT LINK_LIB_LIST 0 hls)
endif ()

###HOOK########
if (ENABLE_HOOK)
    project(hook)
    aux_source_directory("Hook" SRC_HOOK)
    add_library(hook STATIC 
        ${SRC_HOOK}
    )
    list(INSERT LINK_LIB_LIST 0 hook)
endif ()

###RTCP########
if (ENABLE_RTSP OR ENABLE_GB28181 OR ENABLE_WEBRTC)
    project(rtcp)
    aux_source_directory("Rtcp" SRC_RTCP)
    add_library(rtcp STATIC 
        ${SRC_RTCP}
    )
    list(INSERT LINK_LIB_LIST 0 rtcp)
endif ()

###Ehome2########
if (ENABLE_EHOME2)
    project(ehome2)
    aux_source_directory("Ehome2" SRC_Ehome2)
    add_library(ehome2 STATIC 
        ${SRC_Ehome2}
    )
    list(INSERT LINK_LIB_LIST 0 ehome2)
endif ()

###Ehome5########
if (ENABLE_EHOME5)
    project(ehome5)
    aux_source_directory("Ehome5" SRC_Ehome5)
    add_library(ehome5 STATIC 
        ${SRC_Ehome5}
    )
    list(INSERT LINK_LIB_LIST 0 ehome5)
endif ()

###JT1078########
if (ENABLE_JT1078)
    project(jt1078)
    aux_source_directory("JT1078" SRC_JT1078)
    add_library(jt1078 STATIC 
        ${SRC_JT1078}
    )
    list(INSERT LINK_LIB_LIST 0 jt1078)
endif ()

###Flv########
if (ENABLE_FLV)
    project(flv)
    aux_source_directory("Flv" SRC_Flv)
    add_library(flv STATIC 
        ${SRC_Flv}
    )
    list(INSERT LINK_LIB_LIST 0 flv)
endif ()

###Webrtc########
if (ENABLE_WEBRTC)
    project(webrtc)
    aux_source_directory("Webrtc" SRC_Webrtc)
    add_library(webrtc STATIC 
        ${SRC_Webrtc}
    )
    list(INSERT LINK_LIB_LIST 0 webrtc)
endif ()

###SRT########
if (ENABLE_SRT)
    project(srtapp)
    aux_source_directory("Srt" SRC_Srt)
    add_library(srtapp STATIC 
        ${SRC_Srt}
    )
    list(INSERT LINK_LIB_LIST 0 srtapp)
endif ()


###Record########
if (ENABLE_RECORD)
    project(record)
    aux_source_directory("Record" SRC_Record)
    add_library(record STATIC 
        ${SRC_Record}
    )
    list(INSERT LINK_LIB_LIST 0 record)
endif ()

###Ffmpeg########
if (ENABLE_FFMPEG)
    project(ffmpeg)
    aux_source_directory("Ffmpeg" SRC_Ffmpeg)
    add_library(ffmpeg STATIC 
        ${SRC_Ffmpeg}
    )
    list(INSERT LINK_LIB_LIST 0 ffmpeg)
endif ()

###HTTP and HttpStream########
if (ENABLE_HTTP OR ENABLE_API OR ENABLE_HLS)
project(http)
    aux_source_directory("Http" SRC_HTTP)
    aux_source_directory("HttpStream" SRC_HTTP)
    add_library(http STATIC 
        ${SRC_HTTP}
    )
    list(INSERT LINK_LIB_LIST 0 http)
endif ()

###API########
if (ENABLE_API)
    project(api)
    aux_source_directory("Api" SRC_API)
    add_library(api STATIC 
        ${SRC_API}
    )
    list(INSERT LINK_LIB_LIST 0 api)
endif ()

# 获取 Git 版本信息
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LINK_LIB_LIST "${LINK_LIB_LIST}" PARENT_SCOPE)

# 将 Git 版本信息传递给编译器
target_compile_definitions(
    api
    PRIVATE
    GIT_VERSION="${GIT_VERSION}"
)

# 输出获取的 Git 版本信息
message(STATUS "Git Version: ${GIT_VERSION}")
