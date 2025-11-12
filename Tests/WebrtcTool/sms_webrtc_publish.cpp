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
// 引入webrtc
#include "Webrtc/WebrtcClient.h"
#include "Webrtc/WebrtcContextManager.h"
#include "Common/FrameMediaSource.h"
#include "Common/Define.h"
// 引入网络套接字
#include "Net/Socket.h"

using namespace std;

// 全局事件循环指针
EventLoop::Ptr g_loop;
UrlParser g_localUrlParser;
MediaSource::Ptr g_source;

void initInLoop()
{
    g_localUrlParser.path_ = "/live/test";
    g_localUrlParser.host_ = DEFAULT_VHOST;
    g_localUrlParser.type_ = DEFAULT_TYPE;
    g_localUrlParser.protocol_ = PROTOCOL_FRAME;

    auto source = MediaSource::getOrCreate(g_localUrlParser.path_, DEFAULT_VHOST
                        , PROTOCOL_FRAME, DEFAULT_TYPE
                        , [](){
                            return make_shared<FrameMediaSource>(g_localUrlParser, g_loop);
                        });

    if (!source) {
        logWarn << "another source is exist: " << g_localUrlParser.path_;

        return ;
    }

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
    if (!frameSrc) {
        logWarn << "this source is not rtmp: " << source->getProtocol();

        return ;
    }
    frameSrc->setOrigin();
    frameSrc->setAction(true);

    g_source = frameSrc;
}

void startPush(int argc, char** argv)
{
    // 检查命令行参数数量是否正确，若不正确则打印使用说明并退出程序
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <mp4 file> dstUrl loopCount" << endl;
        exit(0);
    }

    WebrtcContextManager::instance()->init(g_loop);

    string apiUrl = UrlParser::urlDecode(argv[2]);

    auto client = make_shared<WebrtcClient>(MediaClientType_Push, "live", "test");
    client->start("0.0.0.0", 0, apiUrl, 5000);

    string key = apiUrl;
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });
}

/**
 * @brief 从MP4文件创建数据源并进行处理
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void createSourceFromMp4(int argc, char** argv)
{
    // 检查命令行参数数量是否正确，若不正确则打印使用说明并退出程序
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <mp4 file> dstUrl loopCount" << endl;
        exit(0);
    }

    // 构造MP4文件路径
    string mp4file = "/file/111/" + string(argv[1]) + "/" + string(argv[3]);
    // 创建记录读取器
    static RecordReaderBase::Ptr reader = RecordReaderBase::createRecordReader(mp4file);
    // 若记录读取器创建失败，打印警告信息并退出程序
    if (!reader) {
        logWarn << "create record reader failed";
        exit(0) ;
    }

    // 设置记录读取器的轨道信息回调函数，当获取到轨道信息时会触发此回调
    reader->setOnTrackInfo([](const TrackInfo::Ptr &trackInfo){
        // 打印接收到的轨道信息的编码格式
        logDebug << "on track info, codec: " << trackInfo->codec_ << endl;
        // 将轨道信息添加到source中
        g_source->addTrack(trackInfo);
    });

    // 设置记录读取器的帧回调函数，当读取到新的帧时会触发此回调
    reader->setOnFrame([](const FrameBuffer::Ptr &frame){
        if (!frame) {
            return ;
        }
        // 打印接收到的帧的大小信息
        logDebug << "on frame, size: " << frame->size() << ", keyFrame: " << frame->keyFrame();
        // 将读取到的帧传递给source进行处理
        g_source->onFrame(frame);
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
        initInLoop();
        createSourceFromMp4(argc, argv);
        startPush(argc, argv);
    }, true, false);

    // 主循环，每隔5秒休眠一次
    while(true) {
        sleep(5);
    }
    
    return 0;
}
