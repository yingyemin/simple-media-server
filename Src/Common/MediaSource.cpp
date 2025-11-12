#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "MediaSource.h"
#include "EventPoller/SrtEventLoop.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Define.h"
#include "FrameMediaSource.h"
#include "HookManager.h"
#include "Util/TimeClock.h"
#include "Config.h"

using namespace std;

recursive_mutex MediaSource::_mtxTotalSource;
unordered_map<string/*uri*/ , MediaSource::Ptr> MediaSource::_totalSource;
unordered_map<MediaClient*, MediaClient::Ptr> MediaSource::_mapPlayer;

mutex MediaSource::_mtxRegister;
unordered_map<string/*uri_vhost_protocol_type*/ , vector<MediaSource::onReadyFunc>> MediaSource::_mapRegisterEvent;

MediaSource::MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop)
    :_urlParser(urlParser)
    ,_loop(loop)
{
    _createTime = TimeClock::now();
}

MediaSource::~MediaSource()
{
    logTrace << "~MediaSource" << _urlParser.path_;
    {
        lock_guard<mutex> lck(_mtxOnReadyFunc);
        for (auto onready : _mapOnReadyFunc) {
            onready.second();
        }
    }
    {
        lock_guard<mutex> lck(_mtxOnDetachFunc);
        for (auto& iter : _mapOnDetachFunc) {
            iter.second();
        }
    }
    StreamStatusInfo statusInfo{_origin, _urlParser.protocol_, _urlParser.path_, _urlParser.vhost_, 
                                _urlParser.type_, "off", _isPush ? "push" : "pull", "400"};
    auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
    if (hook && isOrigin()) {
        hook->onStreamStatus(statusInfo);
    };
}

MediaSource::Ptr MediaSource::get(const string& uri, const string& vhost)
{
    if (uri.empty() || vhost.empty()) {
        return nullptr;
    }
    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    string key = uri + "_" + vhost;
    if (_totalSource.find(key) == _totalSource.end())
    {
        return nullptr;
    }
    return _totalSource[key];
}

MediaSource::Ptr MediaSource::get(const string& uri, const string& vhost, const string& protocol, const string& type)
{
    if (uri.empty() || vhost.empty()) {
        return nullptr;
    }
    MediaSource::Ptr source;
    {
        lock_guard<recursive_mutex> lck(_mtxTotalSource);
        string key = uri + "_" + vhost;
        if (_totalSource.find(key) != _totalSource.end())
        {
            source = _totalSource[key];
        }
    }
    if (source && protocol == source->getProtocol() && type == source->getType()) {
        return source;
    }
    
    if (source) {
        return source->getFromOrigin(protocol, type);
    }

    return nullptr;
}

unordered_map<string/*uri_vhost*/ , MediaSource::Ptr> MediaSource::getAllSource()
{
    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    return _totalSource;
}

MediaSource::Ptr MediaSource::getOrCreate(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const std::function<MediaSource::Ptr()> &create)
{
    if (uri.empty() || vhost.empty()) {
        return nullptr;
    }

    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    string key = uri + "_" + vhost;
    logDebug << "create source, key: " << key;
    auto& src = _totalSource[key];
    if (src && src->getStatus() == SourceStatus::WAITING) {
        src->setStatus(SourceStatus::INIT);
        src->setLoop(EventLoop::getCurrentLoop());
    } else if (!src && create) {
        src = create();
        logTrace << "create a source: " << src.get();
        src->setStatus(SourceStatus::INIT);
        // src->setLoop(EventLoop::getCurrentLoop());
    } else {
        if (src) {
            logInfo << "another same source is exist";
        }
        return nullptr;
    }
    return src;
}

MediaSource::Ptr MediaSource::getOrCreate(const string &protocol, const string& type, const std::function<MediaSource::Ptr()> &create)
{
    lock_guard<recursive_mutex> lck(_mtxStreamSource);
    auto& src = _streamSource[protocol][type];

    if (src.lock()) {
        logInfo << "another same source is exist";
        return nullptr;
    }

    if (create) {
        auto source = create();
        src = source;

        return source;
    }

    return src.lock();
}

MediaSource::Ptr MediaSource::getOrCreateInSrt(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const std::function<MediaSource::Ptr()> &create)
{
#ifdef ENABLE_SRT
    if (uri.empty() || vhost.empty()) {
        return nullptr;
    }

    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    string key = uri + "_" + vhost;
    logInfo << "create source, key: " << key;
    auto& src = _totalSource[key];
    if (src && src->getStatus() == SourceStatus::WAITING) {
        src->setStatus(SourceStatus::INIT);
        src->setLoop(SrtEventLoop::getCurrentLoop());
    } else if (!src && create) {
        src = create();
        logInfo << "srcStream: " << src.get();
        src->setStatus(SourceStatus::INIT);
        src->setLoop(SrtEventLoop::getCurrentLoop());
    } else {
        if (src) {
            logInfo << "another same source is exist";
        }
        return nullptr;
    }
    return src;
#else
    return nullptr;
#endif
}

void MediaSource::getOrCreateAsync(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey)
{
    logTrace << "getOrCreateAsync start";

    if (uri.empty() || vhost.empty()) {
        cb(nullptr);
        return ;
    }

    string key = uri + "_" + vhost;
    auto loop = EventLoop::getCurrentLoop();
    logInfo << "getOrCreateAsync find src, key: " << key;
    do {
        MediaSource::Ptr src;
        {
            // static int enableLoadFromFile = Config::instance()->getAndListen([](const json &config){
            //     enableLoadFromFile = Config::instance()->get("Util", "enableLoadFromFile");
            // }, "Util", "enableLoadFromFile");

            static int waitStreamOnlineTimeMS = Config::instance()->getAndListen([](const json &config){
                waitStreamOnlineTimeMS = Config::instance()->get("Util", "waitStreamOnlineTimeMS");
            }, "Util", "waitStreamOnlineTimeMS");

            if (!waitStreamOnlineTimeMS) {
                waitStreamOnlineTimeMS = 15000;
            }

            static string mode = Config::instance()->getAndListen([](const json &config){
                mode = Config::instance()->get("Cdn", "mode");
            }, "Cdn", "mode");

            lock_guard<recursive_mutex> lck(_mtxTotalSource);
            if (_totalSource.find(key) == _totalSource.end() || !_totalSource[key]) {
                if (startWith(uri, "/file") || startWith(uri, "/record") || startWith(uri, "/dir")
                        || startWith(uri, "/cloud")) {
                    logDebug << "load from file, uri: " << uri;
                    loadFromFile(uri, vhost, protocol, type, cb, create, connKey);
                    return ;
                } else if (mode == "edge") {
                    logDebug << "pull stream with edge mode, uri: " << uri;
                    pullStreamFromOrigin(uri, vhost, protocol, type, cb, create, connKey);
                    return ;
                } else {
                    auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
                    if (hook) {
                        OnStreamNotFoundInfo info;
                        info.uri = uri;
                        hook->onStreamNotFound(info, [uri, vhost, protocol, type, cb, create, connKey](const OnStreamNotFoundResponse &rsp){
                            auto tmpPlayer = MediaClient::getMediaClient(uri);
                            if (tmpPlayer) {
                                tmpPlayer->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey](){
                                    MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
                                });
                                return ;
                            }

                            if (rsp.pullUrl.empty()) {
                                return ;
                            }

                            UrlParser up;
                            up.parse(rsp.pullUrl);
                            auto player = MediaClient::createClient(up.protocol_, uri, MediaClientType_Pull);
                            player->start("0.0.0.0", 0, rsp.pullUrl, 5);

                            MediaClient::addMediaClient(uri, player);

                            player->setOnClose([uri, player](){
                                MediaClient::delMediaClient(uri);
                                const_pointer_cast<MediaClient>(player) = nullptr;
                            });

                            // TODO 
                            // player->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey](){
                            //     MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
                            // });
                        });

                        // return ;
                    }

                    logDebug << "source is not ready yet, waitfor a while, uri: " << uri;;
                    string key = uri + "_" + vhost;// + "_" + protocol + "_" + type;
                    // logInfo << "get a key: " << key;
                    {
                        lock_guard<mutex> lckRegister(_mtxRegister);
                        _mapRegisterEvent[key].push_back([uri, vhost, protocol, type, cb, create, connKey, loop](){
                            loop->async([uri, vhost, protocol, type, cb, create, connKey](){
                                logDebug << "source is ready, start to play again, uri: " << uri;
                                MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
                            }, true);
                        });
                    }
                    loop->addTimerTask(waitStreamOnlineTimeMS, [key, cb](){
                        {
                            logInfo << "get a key: " << key;
                            lock_guard<mutex> lckRegister(_mtxRegister);
                            if (_mapRegisterEvent.find(key) == _mapRegisterEvent.end()) {
                                return 0;
                            }
                            _mapRegisterEvent.erase(key);
                        }
                        cb(nullptr);

                        return 0;
                    }, nullptr);
                    return ;
                }
            }
            logDebug << "getOrCreateAsync find the src, key: " << key;
            src = _totalSource[key];
        
            logTrace << "protocol: " << protocol;
            logTrace << "src->getProtocol(): " << src->getProtocol();
            logTrace << "type: " << type;
            logTrace << "src->getType(): " << src->getType();
            logTrace << "src->getStatus(): " << src->getStatus();
            if (src->getStatus() == SourceStatus::AVAILABLE) {
                if (src->getProtocol() == protocol && src->getType() == type) {
                    logDebug << "getOrCreateAsync src:" << uri << " " << protocol << " " << type;
                    src->addConnection(connKey);
                    cb(src);
                    return ;
                }
            } else {
                logDebug << "source is not ready, uri: " << uri;
                src->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey, loop](){
                    loop->async([uri, vhost, protocol, type, cb, create, connKey](){
                        MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
                    }, true);
                });
                return ;
            }
        }
        logDebug << "getOrCreateAsync find other src, key: " << key;
        // 回调暂时放到这里，后续加上异步上线逻辑后，需要将回调放到异步上线逻辑里
        if (src->getOrCreateAsync(protocol, type, cb, create, connKey)) {
            // cb(nullptr);
            return ;
        } else {
            return ;
        }
    } while (0);

    logInfo << "getOrCreateAsync don't find src";
    cb(nullptr);
}

bool MediaSource::getOrCreateAsync(const string &protocol, const string& type, 
            const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create,
            void* connKey)
{
    logTrace << "getOrCreateAsync start, uri: " << _urlParser.path_;
    lock_guard<recursive_mutex> lck(_mtxStreamSource);
    auto& wsrc = _streamSource[protocol][type];
    auto src = wsrc.lock();
    if (src && src->getStatus() >= SourceStatus::AVAILABLE) {
        cb(src);
        // src->setStatus(SourceStatus::AVAILABLE);
        src->addConnection(connKey);
        return true;
    }

    // if (!getLoop()->isCurrent()) {
    //     weak_ptr<MediaSource> wSelf = shared_from_this();
    //     getLoop()->async([wSelf, protocol, type, cb, create, connKey](){
    //         auto self = wSelf.lock();
    //         if (!self) {
    //             return ;
    //         }
    //         lock_guard<mutex> lck(self->_mtxTotalSource);
    //         self->getOrCreateAsync(protocol, type, cb, create, connKey);
    //     }, true, false);
    //     return true;
    // }

    // 看来不能用注册回调hook，应该在src里增加一个回调函数变量
    // 增加一个ready函数，里面设置 SourceStatus::AVAILABLE ，可控制加锁
    // 上面的获取源函数需要hook回调，因为没有src可以存回调函数
    if (src /*&& src->getStatus() < SourceStatus::AVAILABLE*/) {
        src->addConnection(connKey);
        // src->getLoop()->async([src, cb](){
        //     cb(src);
        // }, true, false);
        src->addOnReady(connKey, [src, cb](){ 
            cb(src);
        });
        return true;
    }

    logTrace << "getOrCreateAsync find frame src, uri: " << _urlParser.path_;

    if (!src) {
        src = create();
        if (!src) {
            cb(nullptr);
            return true;
        }
        wsrc = src;
    }
    
    src->setLoop(_loop);
    src->addConnection(connKey);
    
    string frameSrcType = type == "transcode" ? type : DEFAULT_TYPE;
    auto& wframeSource = _streamSource[PROTOCOL_FRAME][frameSrcType];
    auto frameSource = wframeSource.lock();
    
    if (frameSource) {
        frameSource->getLoop()->async([frameSource, src, cb](){
            // src->setLoop(frameSource->getLoop());
            src->addSource(frameSource);
            src->setStatus(SourceStatus::INIT);
            src->setOrigin(frameSource->getOrigin());
            frameSource->addSink(src);
            cb(src);
        }, true, true);
        return true;
    } else if (type == "transcode") {
        cb(nullptr);
        return true;
    }

    logTrace << "getOrCreateAsync find other protocol src, uri: " << _urlParser.path_;

    UrlParser parser = _urlParser;
    parser.protocol_ = PROTOCOL_FRAME;
    parser.type_ = DEFAULT_TYPE;

    frameSource = make_shared<FrameMediaSource>(parser, _loop);
    wframeSource = frameSource;
    auto srcStream = shared_from_this();
    _loop->async([frameSource, srcStream, src, cb](){
        frameSource->setOrigin(srcStream);
        src->setOrigin(srcStream);

        frameSource->setStatus(SourceStatus::INIT);
        src->setStatus(SourceStatus::INIT);

        // src->setLoop(frameSource->getLoop());
        srcStream->addSink(frameSource);
        frameSource->addSink(src);
        
        frameSource->addSource(srcStream);
        src->addSource(frameSource);

        logTrace << "getOrCreateAsync find other src end： " << src;
        cb(src);
    }, true, true);

    return true;
}

void MediaSource::loadFromFile(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey)
{
    UrlParser parser;
    parser.protocol_ = PROTOCOL_FRAME;
    parser.type_ = "file";
    parser.path_ = uri;// + "-" + randomStr(5) + "-" + to_string(TimeClock::now());
    parser.vhost_ = vhost;

    FrameMediaSource::Ptr frameSource = make_shared<FrameMediaSource>(parser, EventLoop::getCurrentLoop());
    _totalSource[parser.path_ + "_" + vhost] = frameSource;

    RecordReaderBase::Ptr reader = RecordReaderBase::createRecordReader(uri);
    if (!reader) {
        logWarn << "create record reader failed";
        return ;
    }
    void* key = reader.get();
    weak_ptr<FrameMediaSource> wFrameSrc = frameSource;
    frameSource->setOrigin();
    frameSource->addOnReady(key, [wFrameSrc, protocol, type, cb, create, connKey](){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return ;
        }
        frameSrc->getLoop()->async([frameSrc, protocol, type, cb, create, connKey](){
            // frameSrc->onReady();
            frameSrc->getOrCreateAsync(protocol, type, cb, create, connKey);
        }, true);
    });
    reader->setOnFrame([wFrameSrc](const FrameBuffer::Ptr &frame){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return ;
        }
        frameSrc->getLoop()->async([wFrameSrc, frame](){
            auto frameSrc = wFrameSrc.lock();
            if (!frameSrc) {
                return ;
            }
            // if (frameSrc->isReady()) {
                frameSrc->onFrame(frame);
            // }
        }, true);
    });
    reader->setOnTrackInfo([wFrameSrc](const TrackInfo::Ptr &trackInfo){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return ;
        }
        frameSrc->getLoop()->async([frameSrc, trackInfo](){
            frameSrc->addTrack(trackInfo);
        }, true);
    });
    // reader->setOnReady([wFrameSrc, uri, vhost, protocol, type, cb, create, connKey](){
    //     auto frameSrc = wFrameSrc.lock();
    //     if (!frameSrc) {
    //         return ;
    //     }
    //     frameSrc->getLoop()->async([frameSrc, uri, vhost, protocol, type, cb, create, connKey](){
    //         frameSrc->onReady();
    //         MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
    //     }, true);
    // });
    reader->setOnClose([wFrameSrc, key, cb](){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return ;
        }
        frameSrc->getLoop()->async([frameSrc, key, cb](){
            frameSrc->_recordReader = nullptr;
            frameSrc->release();
            frameSrc->delConnection(key);
            if (frameSrc->_status != AVAILABLE) {
                cb(nullptr);
            }
        }, true);
    });
    if (!reader->start()) {
        frameSource->release();
        frameSource->delConnection(key);
        cb(nullptr);
        _totalSource.erase(uri + "_" + vhost);
        reader->setOnClose(nullptr);
        return ;
    }
    frameSource->_recordReader = reader;

    EventLoop::getCurrentLoop()->addTimerTask(5000, [wFrameSrc, cb](){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return 0;
        }

        if (frameSrc->getStatus() < AVAILABLE) {
            frameSrc->release();
            cb(nullptr);
        }

        return 0;
    }, nullptr);
}

void MediaSource::pullStreamFromOrigin(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey)
{
    auto tmpPlayer = MediaClient::getMediaClient(uri);
    if (tmpPlayer) {
        tmpPlayer->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey](){
            MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
        });
        return ;
    }
    static string pullProtocol = Config::instance()->getAndListen([](const json &config){
        pullProtocol = Config::instance()->get("Cdn", "protocol");
    }, "Cdn", "protocol");
    
    static string endpoint = Config::instance()->getAndListen([](const json &config){
        endpoint = Config::instance()->get("Cdn", "endpoint");
    }, "Cdn", "endpoint");
    
    static string params = Config::instance()->getAndListen([](const json &config){
        params = Config::instance()->get("Cdn", "params");
    }, "Cdn", "params");

    string url = pullProtocol + "://" + endpoint + uri;
    if (!params.empty()) {
        url += "?" + params;
    }

    auto player = MediaClient::createClient(pullProtocol, uri, MediaClientType_Pull);
    player->start("0.0.0.0", 0, url, 5);

    MediaClient::addMediaClient(uri, player);

    player->setOnClose([uri, player](){
        MediaClient::delMediaClient(uri);
        const_pointer_cast<MediaClient>(player) = nullptr;
    });

    // TODO 
    player->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey](){
        MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
    });
}

MediaSource::Ptr MediaSource::getFromOrigin(const string& protocol, const string& type)
{
    if (!_origin) {
        auto origin = _originSrc.lock();
        if (!origin) {
            return nullptr;
        }
        lock_guard<recursive_mutex> lck(origin->_mtxStreamSource);
        return origin->_streamSource[protocol][type].lock();
    } else {
        lock_guard<recursive_mutex> lck(_mtxStreamSource);
        return _streamSource[protocol][type].lock();
    }
}

void MediaSource::setOrigin(const MediaSource::Ptr &src)
{
    _originSrc = src;
}

MediaSource::Ptr MediaSource::getOrigin()
{
    return _originSrc.lock();
}

void MediaSource::release(const string &uri, const string& vhost)
{
    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    string key = uri + "_" + vhost;
    logTrace << "delete source: " << key;
    _totalSource.erase(key);
}

unordered_map<string/*protocol*/ , unordered_map<string/*type*/ , MediaSource::Wptr> > 
MediaSource::getMuxerSource() 
{
    lock_guard<recursive_mutex> lck(_mtxStreamSource);
    return _streamSource;
}

void MediaSource::release()
{
    // if (!getLoop()->isCurrent()) {
    //     weak_ptr<MediaSource> wSelf = shared_from_this();
    //     getLoop()->async([wSelf](){
    //         auto self = wSelf.lock();
    //         if (!self) {
    //             return ;
    //         }
    //         self->release();
    //     }, true, false);
    //     return ;
    // }
    logInfo << "delete source: " << _urlParser.path_ + "_" + _urlParser.vhost_ << ", _origin: " << _origin
            << ", type: " << _urlParser.type_ << ", protocol: " << _urlParser.protocol_;
    if (_origin) {
        MediaSource::release(_urlParser.path_, _urlParser.vhost_);
        {
            lock_guard<recursive_mutex> lck(_mtxStreamSource);
            _streamSource.clear();
        }
        {
            // lock_guard<recursive_mutex> lck(_mtxOnDetachFunc);
            // auto tmp = _mapOnDetachFunc;
            // _mapOnDetachFunc.clear();
            // for (auto detach : tmp) {
            //     detach.second();
            // }
        }
        
        logDebug << "delete source, _origin: " << _origin;
        for (auto sink : _mapSink) {
            // sink.second.lock()->release();
            logTrace << "del sink";
            sink.second->release();
        }
        _mapSink.clear();
    } else {
        MediaSource::Ptr src;
        {
            auto origin = _originSrc.lock();
            if (origin) {
                lock_guard<recursive_mutex> lck(origin->_mtxStreamSource);
                origin->_streamSource[_urlParser.protocol_].erase(_urlParser.type_);
            }
        }
        
        // lock_guard<mutex> lck(src->_mtxStreamSource);
        auto self = shared_from_this();
        // for (auto source : _mapSource) {
        //     source.second->delSink(self);
        // }
        for (auto& sink : _mapSink) {
            logTrace << "del sink";
            // sink.second.lock()->release();
            sink.second->release();
        }
        _mapSink.clear();
        {
            // lock_guard<recursive_mutex> lck(_mtxOnDetachFunc);
            // for (auto detach : _mapOnDetachFunc) {
            //     detach.second();
            // }
            // _mapOnDetachFunc.clear();
        }
        _mapSource.clear();
    }
}

// 这几个函数都会切到源所在线程执行，所以不用加锁
void MediaSource::addSink(const MediaSource::Ptr &src)
{
    logTrace << "MediaSource::addSink";
    _mapSink.emplace(src.get(), src);
    _sinkSize = _mapSink.size();
}

void MediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "MediaSource::delSink";
    _mapSink.erase(src.get());
    _sinkSize = _mapSink.size();
}

void MediaSource::addSource(const MediaSource::Ptr &src)
{
    _mapSource.emplace(src.get(), src);
}

void MediaSource::delSource(const MediaSource::Ptr &src)
{
    _mapSource.erase(src.get());
}

void MediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    if (!track) {
        logWarn << "track is empty";
        return ;
    }

    if (track->trackType_ == "video") {
        _hasVideo = true;
    } else if (track->trackType_ == "audio") {
        _hasAudio = true;
    } else {
        logWarn << "get a invalid track: " << track->trackType_;
        return ;
    }

    _mapTrackInfo[track->index_] = track;
    track->setUrlParser(_urlParser);

    lock_guard<mutex> lck(_trackInfoLck);
    _mapLockTrackInfo[track->index_] = track;
}

void MediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    static int firstTrackWaitTime = Config::instance()->getAndListen([](const json &config){
        firstTrackWaitTime = Config::instance()->get("Util", "firstTrackWaitTime");
    }, "Util", "firstTrackWaitTime");

    if (firstTrackWaitTime == 0) {
        firstTrackWaitTime = 500;
    }

    static int sencondTrackWaitTime = Config::instance()->getAndListen([](const json &config){
        sencondTrackWaitTime = Config::instance()->get("Util", "sencondTrackWaitTime");
    }, "Util", "sencondTrackWaitTime");

    if (sencondTrackWaitTime == 0) {
        sencondTrackWaitTime = 5000;
    }

    if (_hasReady && _hasAudio && _hasVideo) {
        return ;
    }

    if (!_hasReady) {
        _frameList.push_back(frame);
        if (_frameList.size() > 100) {
            _frameList.clear();
        }
    }

    logTrace << "frame type:" << (int)frame->getNalType() << " track type:" << frame->getTrackType();

    weak_ptr<MediaSource> wSelf = shared_from_this();
    if (frame->_trackType == VideoTrackType) {
        if (_videoReady) {
            return ;
        }

        _hasVideo = true;

        auto iter = _mapTrackInfo.find(frame->_index);
        if (iter == _mapTrackInfo.end()) {
            logInfo << "video track not found:" << frame->_index;
            return ;
        }

        auto track = iter->second;
        track->onFrame(frame);
        if (track->isReady()) {
            logInfo << "video track ready: " << track->codec_;
            _videoReady = true;
        } else {
            return ;
        }

        _loop->addTimerTask(firstTrackWaitTime, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }

            if (self->_hasReady) {
                return 0;
            }

            if (self->_hasAudio && self->_stage500Ms) {
                self->_stage500Ms = false;
                return sencondTrackWaitTime;
            } else {
                self->onReady();
            }

            return 0;
        }, nullptr);
    } else if (frame->_trackType == AudioTrackType) {
        if (_audioReady) {
            return ;
        }

        auto iter = _mapTrackInfo.find(frame->_index);
        if (iter == _mapTrackInfo.end()) {
            _hasAudio = true;
            return ;
        }

        auto track = iter->second;
        track->onFrame(frame);
        if (track->isReady()) {
            logInfo << "audio track ready: " << track->codec_;
            _audioReady = true;
        } else {
            return ;
        }

        _loop->addTimerTask(firstTrackWaitTime, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }

            if (self->_hasReady) {
                return 0;
            }

            if (self->_hasVideo && self->_stage500Ms) {
                self->_stage500Ms = false;
                return sencondTrackWaitTime;
            } else {
                self->onReady();
            }

            return 0;
        }, nullptr);
    }

    logDebug << "_audioReady: " << _audioReady;
    logDebug << "_videoReady: " << _videoReady;
    if (_audioReady && _videoReady && !_hasReady) {
        onReady();
    }
}

unordered_map<int, shared_ptr<TrackInfo>> MediaSource::getTrackInfo()
{
    lock_guard<mutex> lck(_trackInfoLck);
    return _mapLockTrackInfo;
}

int MediaSource::totalPlayerCount()
{
    int totalPlayerCount = playerCount();

    auto muxerSource = getMuxerSource();
    for (auto& mIt : muxerSource) {
        auto tSource = mIt.second;
        for (auto& tIt: tSource) {
            auto mSource = tIt.second.lock();
            if (!mSource) {
                continue;
            }
            totalPlayerCount += mSource->playerCount();
        }
    }

    return totalPlayerCount;
}

void MediaSource::addOnDetach(void* key, const onDetachFunc& func)
{
    lock_guard<mutex> lck(_mtxOnDetachFunc);
    _mapOnDetachFunc[key] = func;
}

void MediaSource::delOnDetach(void* key)
{
    lock_guard<mutex> lck(_mtxOnDetachFunc);
    _mapOnDetachFunc.erase(key);
}

void MediaSource::onReaderChanged(int size)
{
    logDebug << "onReaderChanged: " << size << ", path: " << _urlParser.path_
            << ", type: " << _urlParser.type_ << ", protocol: " << _urlParser.protocol_ << ", ready: " << isReady();
    if (!isReady()) {
        return ;
    }
    
    if (size != 0) {
        return ;
    }
    
    if (_origin) {
        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onNonePlayer(_urlParser.protocol_, _urlParser.path_, _urlParser.vhost_, _urlParser.type_);
        };
    }

    static int stopNonePlayerStream = Config::instance()->getAndListen([](const json& config){
        stopNonePlayerStream = Config::instance()->get("Util", "stopNonePlayerStream");
    }, "Util", "stopNonePlayerStream");

    logTrace << "stopNonePlayerStream: " << stopNonePlayerStream << ", path: " << _urlParser.path_;

    if (!_origin || stopNonePlayerStream) {
        static int nonePlayerWaitTime = Config::instance()->getAndListen([](const json& config){
            nonePlayerWaitTime = Config::instance()->get("Util", "nonePlayerWaitTime");
        }, "Util", "nonePlayerWaitTime");

        logTrace << "nonePlayerWaitTime: " << nonePlayerWaitTime << ", path: " << _urlParser.path_;

        weak_ptr<MediaSource> wSelf = shared_from_this();
        _loop->addTimerTask(nonePlayerWaitTime, [wSelf, size](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            logTrace << "onReaderChanged task" << ", path: " << self->_urlParser.path_;
            if (size == 0 && self->_origin) {
                lock_guard<recursive_mutex> lock(MediaSource::_mtxTotalSource);
                logTrace << "onReaderChanged _mtxStreamSource";
                if (self->_mapConnection.size() > 0 || self->_mapSink.size() > 0) {
                    return 0;
                }
                logTrace << "onReaderChanged relese" << ", path: " << self->_urlParser.path_;
                self->release();

            } else if (size == 0 && !self->_origin) {
                auto src = MediaSource::get(self->_urlParser.path_, self->_urlParser.vhost_);
                logTrace << "onReaderChanged get src" << ", path: " << self->_urlParser.path_;
                if (src) {
                    logTrace << "onReaderChanged _mtxStreamSource" << ", path: " << self->_urlParser.path_;
                    lock_guard<recursive_mutex> lck(src->_mtxStreamSource);
                    if (self->_mapConnection.size() > 0 || self->_mapSink.size() > 0) {
                        return 0;
                    }
                    logTrace << "onReaderChanged relese" << ", path: " << self->_urlParser.path_;
                    for (auto source : self->_mapSource) {
                        source.second->delSink(self);
                    }
                    self->_mapSource.clear();
                }
            }
            return 0;
        }, [wSelf](bool flag, const shared_ptr<TimerTask>& task){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            if (self->_task) {
                self->_task->quit = false;
            }
            self->_task = task;
        });
    }
    
}

void MediaSource::addOnReady(void* key, const onReadyFunc& func)
{
    {
        lock_guard<recursive_mutex> lck(_mtxStreamSource);
        if (_status == SourceStatus::AVAILABLE) {
            func();
            return ;
        }
        if (_mapOnReadyFunc.find(key) == _mapOnReadyFunc.end()) {
            _mapOnReadyFunc[key] = func;
        }
    }
}

void MediaSource::onReady()
{
    if (_hasReady) {
        return ;
    }
    _hasReady = true;

    logInfo << "on ready, path: " << _urlParser.path_ << ", protocol: " << _urlParser.protocol_
                << ", type: " << _urlParser.type_;

    for (auto track : _mapTrackInfo) {
        bool ready = track.second->isReady();
        string msg = ready ? "ready" : "expire";
        logInfo << "track type: " << track.second->trackType_ << ", codec: " << track.second->codec_ << "(" << msg << ")";
    }

    StreamStatusInfo statusInfo{isOrigin(), _urlParser.protocol_, _urlParser.path_, _urlParser.vhost_, 
                                _urlParser.type_, "on", _isPush ? "push" : "pull", "200"};
    auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
    if (hook && isOrigin()) {
        hook->onStreamStatus(statusInfo);
    };

    {
        string key = _urlParser.path_ + "_" + _urlParser.vhost_;// + "_" + _urlParser.protocol_ + "_" + _urlParser.type_;
        logDebug << "start onready from player: " << key;
        lock_guard<mutex> lckRegister(_mtxRegister);
        auto iter = _mapRegisterEvent.find(key);
        if (iter != _mapRegisterEvent.end()) {
            for (auto& func : iter->second) {
                func();
            }
            _mapRegisterEvent.erase(iter);
        }
    }

    lock_guard<recursive_mutex> lck(_mtxStreamSource);
    _status = SourceStatus::AVAILABLE;
    auto mapOnReadyFunc = _mapOnReadyFunc;
    for (auto onReadyFunc : mapOnReadyFunc) {
        onReadyFunc.second();
    }
    _mapOnReadyFunc.clear();

    for (auto sink : _mapSink) {
        // sink.second.lock()->onReady();
        sink.second->onReady();
    }

    if (_origin) {
        pushStreamToOrigin();

        if (_loop) {
            weak_ptr<MediaSource> wSelf = shared_from_this();
            _loop->addTimerTask(5000, [wSelf](){
                auto self = wSelf.lock();
                if (!self) {
                    return 0;
                }

                self->_bitrate = (self->getBytes() - self->_lastBytes_5s) * 8 / 5.0;
                self->_lastBytes_5s = self->getBytes();

                return 5000;
            }, nullptr);
        }
    }

    if (_frameList.size() > 0) {
        for (auto frame : _frameList) {
            onFrame(frame);
        }
        _frameList.clear();
    }
}

void MediaSource::pushStreamToOrigin()
{
    static string mode = Config::instance()->getAndListen([](const json &config){
        mode = Config::instance()->get("Cdn", "mode");
    }, "Cdn", "mode");

    if (mode != "forward") {
        return ;
    }
    
    static string pushProtocol = Config::instance()->getAndListen([](const json &config){
        pushProtocol = Config::instance()->get("Cdn", "protocol");
    }, "Cdn", "protocol");
    
    static string endpoint = Config::instance()->getAndListen([](const json &config){
        endpoint = Config::instance()->get("Cdn", "endpoint");
    }, "Cdn", "endpoint");
    
    static string params = Config::instance()->getAndListen([](const json &config){
        params = Config::instance()->get("Cdn", "params");
    }, "Cdn", "params");

    string uri = _urlParser.path_;
    string url = pushProtocol + "://" + endpoint + uri;

    if (!params.empty()) {
        url += "?" + params;
    }

    // TODO 可以指定多个endpoint进行推流
    auto pusher = MediaClient::createClient(pushProtocol, uri, MediaClientType_Push);
    if (!pusher->start("0.0.0.0", 0, url, 5)) {
        return ;
    }

    MediaClient::addMediaClient(uri, pusher);
    auto key = pusher.get();

    weak_ptr<MediaSource> wSelf = shared_from_this();
    pusher->setOnClose([uri, key, wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        MediaClient::delMediaClient(uri);
        self->_mapPusher.erase(key);
    });

    _mapPusher.emplace(key, pusher);
}

void MediaSource::delConnection(void* key)
{
    logTrace << "del key: " << key;
    lock_guard<mutex> lck(_mtxConnection);
    _mapConnection.erase(key);
}

void MediaSource::addConnection(void* key)
{
    logTrace << "add key: " << key;
    lock_guard<mutex> lck(_mtxConnection);
    _mapConnection[key] = 1;
}

RecordReaderBase::Ptr MediaSource::getReader()
{
    if (_origin) {
        return _recordReader;
    } else {
        auto originSrc = _originSrc.lock();
        if (originSrc) {
            return originSrc->getReader();
        }
    }

    return nullptr;
}

////////////////////////