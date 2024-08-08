# SimpleMediaServer

#### 介绍
旨在开发一个可商用的，功能丰富、高性能、高可扩展的流媒体服务。支持RTSP、RTMP、WEBRTC、GB28181、HTTP-FLV、HLS、JT1078等协议。各个协议可以单独剥离出来，相互独立；也可以互相转换协议。
项目基于c++11开发，目前只支持Linux。

有兴趣一起开发开源项目的朋友可以联系本人，包括且不限于音视频后端、信令开发、控制台，前端等。

#### 软件架构
1.协议转封装流程
![输入图片说明](img1715737582290.png)

#### 功能
- RTSP
    1. 作为服务端，支持客户端推拉流
    2. 作为客户端，支持向其他服务器推拉流
    3. 支持rtp over udp、tcp

- RTMP
    1. 作为服务端，支持客户端推拉流
    2. 作为客户端，支持向其他服务器推拉流

- GB28181
    1. 作为服务端，支持客户端推拉流
    2. 作为客户端，支持向其他服务器推拉流
    3. 支持rtp over udp、tcp

- WEBRTC
    1. 作为服务端，支持客户端拉流
    2. 作为服务端，支持客户端推流
    3. 作为客户端，向其他服务器推拉流

- HTTP-FLV
    1. 作为服务端，支持客户端拉流
    2. 作为客户端，向其他服务拉流

- HLS
    1. 作为服务端，支持客户端拉流
    2. 作为客户端，向其他服务拉流

- HTTP-TS
    1. 作为服务端，支持客户端拉流
    2. 作为客户端，向其他服务拉流


- HTTP-PS
    1. 作为服务端，支持客户端拉流
    2. 作为客户端，向其他服务拉流


- SRT
    1. 作为服务端，支持客户端推拉流
    2. 作为客户端，向其他服务推拉流


- EHOME2
    1. 作为服务端，支持客户端推流(tcp)


- HTTPS
    1. https协议拉流，访问api，下载文件


- WEBSOCKET(ws/wss)
    1. websocket协议拉流


- JT1078
    1. 作为服务端，支持客户端推流

#### 安装教程

1.  mkdir build
2.  cd build
3.  cmake ..
4.  make -j4
5.  如果需要ssl功能，将conf下的sms.com.crt和sms.com.key，拷贝到build目录
6.  如果需要前端页面实例，拷贝research到build目录，http的根目录默认是build

#### 使用说明

1.  检查配置，如 server.conf
2.  运行程序：./SimpleMediaServer
3.  用ffmpeg推流：ffmpeg -re -i inputfile -c copy -f flv rtmp://ip:port/live/test
4.  用vlc播放
- rtsp地址：rtsp://ip:port/live/test
- rtmp地址：rtmp://ip:port/live/test
- http-flv地址：http://ip:port/live/test.flv
- hls地址：http://ip:port/live/test.m3u8
- webrtc获取sdp：http://ip:port/api/v1/rtc/play
- srt: srt://ip:port?steamid=|path:/live/test|request:pull|

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
