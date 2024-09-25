cmake_minimum_required(VERSION 3.0)
project(src)

#mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# aux_source_directory(. DIRSRCS)

###RTMP#####
aux_source_directory("Rtmp" SRC_RTMP)
aux_source_directory("Rtmp/Decode" SRC_RTMP)
aux_source_directory("Rtmp/Encode" SRC_RTMP)

###RTSP#####
aux_source_directory("Rtsp" SRC_RTSP)

###RTP#####
aux_source_directory("Rtp" SRC_RTP)
aux_source_directory("Rtp/Decoder" SRC_RTP)
aux_source_directory("Rtp/Encoder" SRC_RTP)

###COMMON#####
aux_source_directory("Common" SRC_COMMON)
aux_source_directory("Codec" SRC_COMMON)

###HTTP########
aux_source_directory("Http" SRC_HTTP)
aux_source_directory("Api" SRC_HTTP)

###HOOK########
aux_source_directory("Hook" SRC_HOOK)

###RTCP########
aux_source_directory("Rtcp" SRC_RTCP)

###MPEG########
aux_source_directory("Mpeg" SRC_MPEG)

###MP4########
aux_source_directory("Mp4" SRC_MP4)

###GB28181########
aux_source_directory("GB28181" SRC_GB28181)

###Ehome2########
aux_source_directory("Ehome2" SRC_Ehome2)

###Ehome5########
aux_source_directory("Ehome5" SRC_Ehome5)

###JT1078########
aux_source_directory("JT1078" SRC_JT1078)

###Flv########
aux_source_directory("Flv" SRC_Flv)

###Hls########
aux_source_directory("Hls" SRC_Hls)

###Webrtc########
aux_source_directory("Webrtc" SRC_Webrtc)

###SRT########
aux_source_directory("Srt" SRC_Srt)

###HttpStream########
aux_source_directory("HttpStream" SRC_HttpStream)

###Record########
aux_source_directory("Record" SRC_Record)

###Ffmpeg########
aux_source_directory("Ffmpeg" SRC_Ffmpeg)

add_library(src STATIC 
    ${SRC_RTSP}
    ${SRC_RTMP}
    ${SRC_RTP}
    ${SRC_COMMON}
    ${SRC_HTTP}
    ${SRC_HOOK}
    ${SRC_RTCP}
    ${SRC_MPEG}
    ${SRC_MP4}
    ${SRC_GB28181}
    ${SRC_Ehome2}
    ${SRC_Ehome5}
    ${SRC_JT1078}
    ${SRC_Flv}
    ${SRC_Hls}
    ${SRC_Webrtc}
    ${SRC_Srt}
    ${SRC_HttpStream}
    ${SRC_Record}
    ${SRC_Ffmpeg}
)
