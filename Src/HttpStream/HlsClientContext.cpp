#include "HlsClientContext.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

HlsClientContext::HlsClientContext(MediaClientType type, const string& appName, const string& streamName)
    :_type(type)
{
    _localUrlParser.path_ = "/" + appName + "/" + streamName;
    _localUrlParser.protocol_ = PROTOCOL_FRAME;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

HlsClientContext::~HlsClientContext()
{
    if (_timeTask) {
        _timeTask->quit = true;
    }
}

bool HlsClientContext::start(const string& localIp, int localPort, const string& url, int timeout)
{
    _localIp = localIp;
    _localPort = localPort;
    _timeout = timeout;
    _originUrl = url;
    _m3u8 = url;

    weak_ptr<HlsClientContext> wSelf = shared_from_this();

    _tsDemuxer.setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
        logInfo << "decode a frame";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        if (self->_frameList.size() == 0) {
            self->_lastPts = frame->pts();
            self->_frameList.emplace(self->_lastPlayStamp, frame);
            return ;
        }

        int interval = frame->pts() - self->_lastPts;
        if (interval < 0) {
            interval = 1;
        }
        self->_lastPlayStamp += interval;
        self->_frameList.emplace(self->_lastPlayStamp, frame);
        self->_lastPts = frame->pts();
    });

    _tsDemuxer.setOnReady([wSelf](){
        logInfo << "source is on ready";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto source = self->_source.lock();
        if (!source) {
            return ;
        }

        source->onReady();
    });

    _tsDemuxer.setOnTrackInfo([wSelf](const std::shared_ptr<TrackInfo> &trackInfo){
        logInfo << "add track info: " << trackInfo->codec_;
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto source = self->_source.lock();
        if (!source) {
            return ;
        }

        source->addTrack(trackInfo);
    });

    _loop = EventLoop::getCurrentLoop();
    _loop->addTimerTask(100, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        // TODO 
        // 1.检查ts队列，是否有ts要解析
        // 2.检查帧列表，队列较小时，下载ts解析（队列小于下一个要播的ts时长时，下载？或者取10秒和下一个ts时长的较大值）

        logInfo << "start to play";
        self->onPlay();

        return 100;
    }, [wSelf](bool success, const shared_ptr<TimerTask>& task){
        auto self = wSelf.lock();
        if (self) {
            self->_timeTask = task;
        } else {
            task->quit = true;
        }
    });

    _hlsClient = make_shared<HttpHlsClient>();
    _hlsClient->start(localIp, localPort, url, timeout);
    _hlsClient->setOnHttpResponce([wSelf](const HttpParser &parser){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto m3u8List = self->_hlsClient->_hlsParser->getM3u8List(self->_hlsClient->_m3u8);
        if (m3u8List.size() == 0) {
            return ;
        }

        if (self->_hlsClient->_hlsParser->isMutilM3u8()) {
            // 获取第一个m3u8，可以调整策略
            self->_m3u8 = m3u8List.begin()->second.url;
            self->getM3u8();
        } else {
            self->putTsList();
            self->_playClock.start();
            self->startPlay();
            // 定时获取m3u8，一次性定时器，获取完后，再继续创建一次性定时器
            // 定时时间用ts列表里的ts的最短时长

            // if (self->_tsList.size() > 0) {
            //     _loop->addTimerTask(self->_tsList.begin()->second.duration * 1000, [wSelf](){
            //         auto self = wSelf.lock();
            //         if (!self) {
            //             return 0;
            //         }

            //         self->getM3u8Inner();

            //         return 0;
            //     }, nullptr);
            // }
        }
    });

    return true;
}

void HlsClientContext::startPlay()
{
    auto source = MediaSource::getOrCreate(_localUrlParser.path_, DEFAULT_VHOST
                        , PROTOCOL_FRAME, DEFAULT_TYPE
                        , [this](){
                            return make_shared<FrameMediaSource>(_localUrlParser, _loop);
                        });

    if (!source) {
        logWarn << "another source is exist: " << _localUrlParser.path_;
        stop();

        return ;
    }

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
    if (!frameSrc) {
        logWarn << "this source is not rtmp: " << source->getProtocol();
        stop();

        return ;
    }

    _source = frameSrc;
}

void HlsClientContext::onPlay()
{
    auto frameSrc = _source.lock();
    if (!frameSrc) {
        logInfo << "src is not exist";
        return ;
    }
    if (_frameList.size() == 0) {
        downloadTs();
    } else if (_frameList.end()->first - _frameList.begin()->first < 5 * 1000) {
        downloadTs();
    }

    for (auto it = _frameList.begin(); it != _frameList.end();) {
        logInfo << "it->first: " << it->first << ", _playClock.startToNow(): " << _playClock.startToNow();
        if (it->first < _playClock.startToNow()) {
            _aliveClock.update();
            logInfo << "input a frame, index: " << it->second->getTrackIndex() << ", time: " << it->second->pts();
            frameSrc->onFrame(it->second);
            it = _frameList.erase(it);
        } else {
            break;
        }
    }

    logInfo << "frame list size: " << _frameList.size();
    if (_frameList.size() == 0 && _tsList.size() == 0 && _aliveClock.startToNow() > 10 * 1000) {
        stop();
    }
}

void HlsClientContext::downloadTs()
{
    if (_tsList.size() == 0) {
        return ;
    }

    weak_ptr<HlsClientContext> wSelf = shared_from_this();

    auto iter = _tsList.begin();
    auto url = iter->second.url;
    _preTsIndex = iter->first;
    _tsList.erase(iter);
    auto httppos = url.find("http://");
    auto httpspos = url.find("https://");
    if (httppos != 0 && httpspos != 0) {
        if (url.find("/") == 0) {
            url = _originUrl.substr(0, _originUrl.find("/", 8)) + url;
        } else {
            url = _originUrl.substr(0, _originUrl.rfind("/") + 1) + url;
        }
    }

    logInfo << "ts url: " << url
            << ", _originUrl: " << _originUrl
            << ", http pos: " << httppos
            << ", https pos: " << httpspos;

    _tsClient = make_shared<HttpHlsTsClient>();
    _tsClient->setOnTsPacket([wSelf](const char *data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        logInfo << "start to demuxer ts packet";
        self->_tsDemuxer.onTsPacket((char*)data, len, 0);
    });
    _tsClient->start(_localIp, _localPort, url, _timeout);
}

void HlsClientContext::stop()
{
    if (_onClose) {
        _onClose();
    }
}

void HlsClientContext::pause()
{

}

void HlsClientContext::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

string HlsClientContext::getM3u8()
{
    logInfo << "get M3u8: " << _m3u8;
    auto httppos = _m3u8.find("http://");
    auto httpspos = _m3u8.find("https://");
    if (httppos != 0 && httpspos != 0) {
        if (_m3u8.find("/") == 0) {
            _m3u8 = _originUrl.substr(0, _originUrl.find("/", 8)) + _m3u8;
        } else {
            _m3u8 = _originUrl.substr(0, _originUrl.rfind("/") + 1) + _m3u8;
        }
    }

    logInfo << "ts url: " << _originUrl
            << ", _m3u8: " << _m3u8
            << ", http pos: " << httppos
            << ", https pos: " << httpspos;
            
    weak_ptr<HlsClientContext> wSelf = shared_from_this();
    
    _hlsClient = make_shared<HttpHlsClient>();
    _hlsClient->start(_localIp, _localPort, _m3u8, _timeout);
    _hlsClient->setOnHttpResponce([wSelf](const HttpParser &parser){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto m3u8List = self->_hlsClient->_hlsParser->getM3u8List(self->_hlsClient->_m3u8);
        if (m3u8List.size() == 0) {
            return ;
        }

        if (self->_hlsClient->_hlsParser->isMutilM3u8()) {
            // 获取第一个m3u8，可以调整策略
            self->_m3u8 = m3u8List.begin()->second.url;
            self->getM3u8();
        } else {
            self->putTsList();
            self->_playClock.start();
            self->startPlay();
        }
    });

    return "";
}

void HlsClientContext::getM3u8Inner()
{
    weak_ptr<HlsClientContext> wSelf = shared_from_this();
    _hlsClient = make_shared<HttpHlsClient>();
    _hlsClient->start(_localIp, _localPort, _m3u8, _timeout);
    _hlsClient->setOnHttpResponce([wSelf](const HttpParser &parser){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->putTsList();
    });
}

void HlsClientContext::putTsList()
{
    auto tsList = _hlsClient->_hlsParser->getTsList(_hlsClient->_m3u8);
    // if (_preTsIndex < 0) {
    //     _preTsIndex = tsList.begin()->first;
    // }
    // int curIndex = _tsList.begin()->first;
    for (auto& ts : tsList) {
        logInfo << "ts index: " << ts.first << ", cur index: " << _preTsIndex;
        if (ts.first <= _preTsIndex) {
            continue;
        }
        _tsList.emplace(ts.first, ts.second);
    }

    if (_tsList.size() > 0) {
        weak_ptr<HlsClientContext> wSelf = shared_from_this();
        _loop->addTimerTask(_tsList.begin()->second.duration * 1000, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }

            self->getM3u8Inner();

            return 0;
        }, nullptr);
    }
}