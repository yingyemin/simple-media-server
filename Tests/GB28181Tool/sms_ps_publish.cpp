#include <iostream>
#include <unistd.h>

// 引入日志系统
#include "Log/Logger.h"
// 引入事件循环池
#include "EventPoller/EventLoopPool.h"
// 引入工作线程池
#include "WorkPoller/WorkLoopPool.h"
// 引入线程工具
#include "Util/Thread.h"
// 引入记录读取器基础类
#include "Common/RecordReaderBase.h"
// 引入记录读取器
#include "Record/RecordReader.h"
// 引入aac编码轨道
#include "Codec/AacTrack.h"
// 引入av1编码轨道
#include "Codec/AV1Track.h"
// 引入mp3编码轨道
#include "Codec/Mp3Track.h"
// 引入g711编码轨道
#include "Codec/G711Track.h"
// 引入adpcma编码轨道
#include "Codec/AdpcmaTrack.h"
// 引入opus编码轨道
#include "Codec/OpusTrack.h"
// 引入h264编码轨道
#include "Codec/H264Track.h"
// 引入h265编码轨道
#include "Codec/H265Track.h"
// 引入h266编码轨道
#include "Codec/H266Track.h"
// 引入vp8编码轨道
#include "Codec/VP8Track.h"
// 引入vp9编码轨道
#include "Codec/VP9Track.h"

// 引入aac编码帧
#include "Codec/AacFrame.h"
// 引入av1编码帧
#include "Codec/AV1Frame.h"
// 引入h264编码帧
#include "Codec/H264Frame.h"
// 引入h265编码帧
#include "Codec/H265Frame.h"
// 引入h266编码帧
#include "Codec/H266Frame.h"
// 引入vp8编码帧
#include "Codec/VP8Frame.h"
// 引入vp9编码帧
#include "Codec/VP9Frame.h"
// 引入ps复用器
#include "Mpeg/PsMuxer.h"
// 引入rtp编码公共头文件
#include "Rtp/Encoder/RtpEncodeCommon.h"
// 引入网络套接字
#include "Net/Socket.h"

using namespace std;

// 全局事件循环指针
EventLoop::Ptr g_loop;

/**
 * @brief 从MP4文件创建数据源并进行处理
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void createSourceFromMp4(int argc, char** argv)
{
    // 检查命令行参数数量是否正确，若不正确则打印使用说明并退出程序
    if (argc != 6) {
        cout << "Usage: " << argv[0] << " <mp4 file> dstIp dstPort transType[tcp/udp] loopCount" << endl;
        exit(0);
    }

    // 创建一个Socket对象，使用全局事件循环
    Socket::Ptr socket = make_shared<Socket>(g_loop);
    int offset = 0;
    // 根据传输类型选择TCP或UDP
    if (strcasecmp(argv[4], "tcp") == 0) {
        // 创建TCP套接字
        socket->createSocket(SOCKET_TCP);
        // 连接到目标IP和端口
        socket->connect(argv[2], atoi(argv[3]));
        offset = 2;
    } else {
        // 创建UDP套接字并绑定到目标地址
        auto addr = socket->createSocket(argv[2], atoi(argv[3]), SOCKET_UDP);
        socket->bindPeerAddr((sockaddr*)&addr);
        offset = 4;
    }

    // 设置Socket错误回调函数，当Socket出现错误时打印错误信息并退出程序
    socket->setErrorCb([](const string& err){
        logError << "socket error, err: " << err;
        exit(1);
    });

    // 构造MP4文件路径
    string mp4file = "/file/111/" + string(argv[1]) + "/" + string(argv[5]);
    // 创建记录读取器
    static RecordReaderBase::Ptr reader = RecordReaderBase::createRecordReader(mp4file);
    // 若记录读取器创建失败，打印警告信息并退出程序
    if (!reader) {
        logWarn << "create record reader failed";
        exit(0) ;
    }

    // 创建PS复用器
    PsMuxer::Ptr muxer = make_shared<PsMuxer>();
    // 若PS复用器创建失败，打印警告信息并退出程序
    if (!muxer) {
        logWarn << "create ps muxer failed";
        exit(0) ;
    }

    static TrackInfo::Ptr videoTrackInfo;

    // 创建轨道信息对象
    auto trackInfo = make_shared<TrackInfo>();
    trackInfo->codec_ = "ps";
    trackInfo->index_ = 0;
    trackInfo->samplerate_ = 90000;
    trackInfo->ssrc_ = trackInfo->index_;
    trackInfo->payloadType_ = 96;
    // 创建RTP编码器
    RtpEncodeCommon::Ptr encoder = make_shared<RtpEncodeCommon>(trackInfo);
    // 设置RTP数据包回调函数，当有RTP数据包生成时发送数据包
    encoder->setOnRtpPacket([socket, offset](const RtpPacket::Ptr& packet, bool start){
        auto data = packet->data();
        data[2] = (packet->size() - 4) >> 8;
        data[3] = (packet->size() - 4) & 0x00FF;

        logTrace << "on rtp packet, size: " << packet->size();
        // 调用socket的send方法发送RTP数据包，第二个参数1的含义需结合Socket类实现确定，offset为偏移量
        socket->send(packet->buffer(), 1, offset);
    });

    // 设置PS复用器的PS帧回调函数，当有PS帧生成时会触发此回调
    muxer->setOnPsFrame([encoder](const FrameBuffer::Ptr& buffer){
        // 打印接收到的PS帧的大小信息
        logTrace << "on ps frame, size: " << buffer->size();
        // 调用编码器对PS帧进行编码
        encoder->encode(buffer);
    });

    // 设置记录读取器的轨道信息回调函数，当获取到轨道信息时会触发此回调
    reader->setOnTrackInfo([muxer](const TrackInfo::Ptr &trackInfo){
        // 打印接收到的轨道信息的编码格式
        logDebug << "on track info, codec: " << trackInfo->codec_ << endl;
        // 将轨道信息添加到PS复用器中
        muxer->addTrackInfo(trackInfo);
        if (trackInfo->trackType_ == "video") {
            videoTrackInfo = trackInfo;
        }
    });

    // 设置记录读取器的帧回调函数，当读取到新的帧时会触发此回调
    reader->setOnFrame([muxer](const FrameBuffer::Ptr &frame){
        if (!frame) {
            return ;
        }
        // 打印接收到的帧的大小信息
        logDebug << "on frame, size: " << frame->size() << ", keyFrame: " << frame->keyFrame();
        if (frame->keyFrame()) {
            FrameBuffer::Ptr vps;
            FrameBuffer::Ptr sps;
            FrameBuffer::Ptr pps;
            videoTrackInfo->getVpsSpsPps(vps, sps, pps);
            if (vps) {
                logDebug << "vps on frame, size: " << vps->size();
                muxer->onFrame(vps);
            }
            if (sps) {
                logDebug << "sps on frame, size: " << sps->size();
                muxer->onFrame(sps);
            }
            if (pps) {
                logDebug << "pps on frame, size: " << pps->size();
                muxer->onFrame(pps);
            }
        }
        // 将读取到的帧传递给PS复用器进行处理
        muxer->onFrame(frame);
    });

    // 设置记录读取器的关闭回调函数，当记录读取器关闭时会触发此回调，当前为空实现
    reader->setOnClose([](){
    });

    // 尝试启动记录读取器
    if (!reader->start()) {
        // 若启动失败，将关闭回调函数置空
        reader->setOnClose(nullptr);
        // 打印启动失败的错误信息
        logError << "start reader failed";
        // 程序退出
        exit(0) ;
    }

    // 启动PS复用器的编码过程
    muxer->startEncode();
}

int main(int argc, char** argv)
{
    // 设置当前线程的名称
    Thread::setThreadName("sms_ps_publish");
    // 初始化事件循环池
    EventLoopPool::instance()->init(0, true, true);
    // 初始化工作循环池
    WorkLoopPool::instance()->init(0, true, true);

    // 为日志记录器添加控制台输出通道，并设置日志级别为LTrace
    Logger::instance()->addChannel(std::make_shared<ConsoleChannel>("ConsoleChannel", LTrace));
    Logger::instance()->setLevel(LInfo);

#ifdef ENABLE_HOOK
    // 初始化媒体钩子
    MediaHook::instance()->init();
    // 设置钩子报告回调函数
    HookManager::instance()->setOnHookReportByHttp(HttpClientApi::reportByHttp);
#endif

#ifdef ENABLE_RECORD
    // 初始化记录读取器
    RecordReader::init();
#endif

    // 注册各种音频和视频轨道信息
    AacTrack::registerTrackInfo();
    AV1Track::registerTrackInfo();
    Mp3Track::registerTrackInfo();
    G711aTrack::registerTrackInfo();
    G711uTrack::registerTrackInfo();
    AdpcmaTrack::registerTrackInfo();
    OpusTrack::registerTrackInfo();
    H264Track::registerTrackInfo();
    H265Track::registerTrackInfo();
    H266Track::registerTrackInfo();
    VP8Track::registerTrackInfo();
    VP9Track::registerTrackInfo();

    // 注册各种音频和视频帧
    AacFrame::registerFrame();
    AV1Frame::registerFrame();
    H264Frame::registerFrame();
    H265Frame::registerFrame();
    H266Frame::registerFrame();
    VP8Frame::registerFrame();
    VP9Frame::registerFrame();

    // 从事件循环池中获取一个事件循环
    g_loop = EventLoopPool::instance()->getLoopByCircle();
    // 异步执行从MP4文件创建数据源的函数
    g_loop->async([argc, argv](){
        createSourceFromMp4(argc, argv);
    }, true, false);

    // 主循环，每隔5秒休眠一次
    while(true) {
        sleep(5);
    }
    
    return 0;
}