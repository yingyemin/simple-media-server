#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "MediaSource.h"
#include "EventPoller/SrtEventLoop.h"
#include "Logger.h"
#include "Util/String.h"
#include "Define.h"
#include "FrameMediaSource.h"
#include "Hook/MediaHook.h"
#include "Util/TimeClock.h"
#include "Config.h"

using namespace std;

recursive_mutex MediaSource::_mtxTotalSource;
unordered_map<string/*uri*/ , MediaSource::Ptr> MediaSource::_totalSource;
MediaClient::Ptr MediaSource::_player;
unordered_map<MediaClient*, MediaClient::Ptr> MediaSource::_mapPusher;

MediaSource::MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop)
    :_urlParser(urlParser)
    ,_loop(loop)
{
    _createTime = TimeClock::now();
}

MediaSource::~MediaSource()
{
    logInfo << "~MediaSource";
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
    StreamStatusInfo statusInfo{_urlParser.protocol_, _urlParser.path_, _urlParser.vhost_, 
                                _urlParser.type_, "off", "400"};
    MediaHook::instance()->onStreamStatus(statusInfo);
}

MediaSource::Ptr MediaSource::get(const string& uri, const string& vhost)
{
    if (uri.empty() || vhost.empty()) {
        return nullptr;
    }
    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    string key = uri + "_" + vhost;
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

MediaSource::Ptr MediaSource::getOrCreate(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const std::function<MediaSource::Ptr()> &create)
{
    if (uri.empty() || vhost.empty()) {
        return nullptr;
    }

    lock_guard<recursive_mutex> lck(_mtxTotalSource);
    string key = uri + "_" + vhost;
    logInfo << "create source, key: " << key;
    auto& src = _totalSource[key];
    if (src && src->getStatus() == SourceStatus::WAITING) {
        src->setStatus(SourceStatus::INIT);
        src->setLoop(EventLoop::getCurrentLoop());
    } else if (!src && create) {
        src = create();
        logInfo << "srcStream: " << src.get();
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

MediaSource::Ptr MediaSource::getOrCreateInSrt(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const std::function<MediaSource::Ptr()> &create)
{
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
}

void MediaSource::getOrCreateAsync(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey)
{
    logInfo << "getOrCreateAsync start";

    if (uri.empty() || vhost.empty()) {
        cb(nullptr);
        return ;
    }

    string key = uri + "_" + vhost;
    logInfo << "getOrCreateAsync find src, key: " << key;
    do {
        MediaSource::Ptr src;
        {
            static int enableLoadFromFile = Config::instance()->getAndListen([](const json &config){
                enableLoadFromFile = Config::instance()->get("Util", "enableLoadFromFile");
            }, "Util", "enableLoadFromFile");

            static string mode = Config::instance()->getAndListen([](const json &config){
                mode = Config::instance()->get("Cdn", "mode");
            }, "Cdn", "mode");

            lock_guard<recursive_mutex> lck(_mtxTotalSource);
            if (_totalSource.find(key) == _totalSource.end()) {
                if (enableLoadFromFile) {
                    loadFromFile(uri, vhost, protocol, type, cb, create, connKey);
                    return ;
                } else if (mode == "edge") {
                    pullStreamFromOrigin(uri, vhost, protocol, type, cb, create, connKey);
                    return ;
                } else {
                    break;
                }
            }
            logInfo << "getOrCreateAsync find the src, key: " << key;
            src = _totalSource[key];
        
            if (!src) {
                if (enableLoadFromFile) {
                    loadFromFile(uri, vhost, protocol, type, cb, create, connKey);
                    return ;
                } else if (mode == "edge") {
                    pullStreamFromOrigin(uri, vhost, protocol, type, cb, create, connKey);
                    return ;
                } else {
                    break;
                }
            }
            // logInfo << "protocol: " << protocol;
            // logInfo << "src->getProtocol(): " << src->getProtocol();
            // logInfo << "type: " << type;
            // logInfo << "src->getType(): " << src->getType();
            // logInfo << "src->getStatus(): " << src->getStatus();
            if (src->getStatus() == SourceStatus::AVAILABLE) {
                if (src->getProtocol() == protocol && src->getType() == type) {
                    logInfo << "getOrCreateAsync src:" << uri << " " << protocol << " " << type;
                    src->addConnection(connKey);
                    cb(src);
                    return ;
                }
            } else {
                logInfo << "source is not ready";
                src->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey](){
                    MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
                });
                return ;
            }
        }
        logInfo << "getOrCreateAsync find other src, key: " << key;
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
    logInfo << "getOrCreateAsync start";
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
    if (src && src->getStatus() < SourceStatus::AVAILABLE) {
        src->addConnection(connKey);
        src->getLoop()->async([src, cb](){
            cb(src);
        }, true, false);
        return true;
    }

    logInfo << "getOrCreateAsync find frame src";

    src = create();
    src->setLoop(_loop);
    src->addConnection(connKey);
    wsrc = src;
    auto& wframeSource = _streamSource[PROTOCOL_FRAME][DEFAULT_TYPE];
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
    }

    logInfo << "getOrCreateAsync find other src";

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

        logInfo << "getOrCreateAsync find other src end： " << src;
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
    parser.type_ = DEFAULT_TYPE;
    parser.path_ = uri;
    parser.vhost_ = vhost;

    FrameMediaSource::Ptr frameSource = make_shared<FrameMediaSource>(parser, EventLoop::getCurrentLoop());
    _totalSource[uri + "_" + vhost] = frameSource;

    RecordReaderBase::Ptr reader = RecordReaderBase::createRecordReader(uri);
    if (!reader) {
        logWarn << "create record reader failed";
        return ;
    }
    void* key = reader.get();
    weak_ptr<FrameMediaSource> wFrameSrc = frameSource;
    reader->setOnFrame([wFrameSrc](const FrameBuffer::Ptr &frame){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return ;
        }
        frameSrc->getLoop()->async([frameSrc, frame](){
            frameSrc->onFrame(frame);
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
    reader->setOnReady([wFrameSrc, uri, vhost, protocol, type, cb, create, connKey](){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return ;
        }
        frameSrc->getLoop()->async([frameSrc, uri, vhost, protocol, type, cb, create, connKey](){
            frameSrc->onReady();
            MediaSource::getOrCreateAsync(uri, vhost, protocol, type, cb, create, connKey);
        }, true);
    });
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
        cb(nullptr);
        _totalSource.erase(uri + "_" + vhost);
        return ;
    }
    frameSource->setOrigin();
    frameSource->_recordReader = reader;

    EventLoop::getCurrentLoop()->addTimerTask(5000, [wFrameSrc](){
        auto frameSrc = wFrameSrc.lock();
        if (!frameSrc) {
            return 0;
        }

        if (frameSrc->getStatus() < AVAILABLE) {
            frameSrc->release();
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

    string url = pullProtocol + "://" + endpoint + uri;

    _player = MediaClient::createClient(pullProtocol, uri, MediaClientType_Pull);
    _player->start("0.0.0.0", 0, url, 5);

    MediaClient::addMediaClient(uri, _player);

    _player->setOnClose([uri](){
        MediaClient::delMediaClient(uri);
    });

    // TODO 
    _player->addOnReady(connKey, [uri, vhost, protocol, type, cb, create, connKey](){
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
    logInfo << "delete source: " << key;
    _totalSource.erase(key);
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
    logInfo << "delete source, _origin: " << _origin;
    if (_origin) {
        MediaSource::release(_urlParser.path_, _urlParser.vhost_);
        _streamSource.clear();
        {
            // lock_guard<recursive_mutex> lck(_mtxOnDetachFunc);
            // auto tmp = _mapOnDetachFunc;
            // _mapOnDetachFunc.clear();
            // for (auto detach : tmp) {
            //     detach.second();
            // }
        }
        
        logInfo << "delete source, _origin: " << _origin;
        for (auto sink : _mapSink) {
            // sink.second.lock()->release();
            logInfo << "del sink";
            sink.second->release();
        }
        _mapSink.clear();
    } else {
        MediaSource::Ptr src;
        {
            auto origin = _originSrc.lock();
            if (!origin) {
                return ;
            }
            lock_guard<recursive_mutex> lck(origin->_mtxStreamSource);
            origin->_streamSource[_urlParser.protocol_].erase(_urlParser.type_);
        }
        
        // lock_guard<mutex> lck(src->_mtxStreamSource);
        auto self = shared_from_this();
        // for (auto source : _mapSource) {
        //     source.second->delSink(self);
        // }
        for (auto& sink : _mapSink) {
            logInfo << "del sink";
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
    logInfo << "MediaSource::addSink";
    _mapSink.emplace(src.get(), src);
}

void MediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "MediaSource::delSink";
    _mapSink.erase(src.get());
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

    _mapTrackInfo[track->index_] = track;
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
    logInfo << "onReaderChanged: " << size;
    if (size != 0) {
        return ;
    }
    
    if (_origin) {
        static string onNonePlayer = Config::instance()->getAndListen([](const json& config){
            onNonePlayer = Config::instance()->get("Hook", "Http", "onNonePlayer");
        }, "Hook", "Http", "onNonePlayer");

        if (!onNonePlayer.empty()) {
            MediaHook::instance()->onNonePlayer(_urlParser.protocol_, _urlParser.path_, _urlParser.vhost_, _urlParser.type_);
            // return ;
        }
    }

    static int stopNonePlayerStream = Config::instance()->getAndListen([](const json& config){
        stopNonePlayerStream = Config::instance()->get("Util", "stopNonePlayerStream");
    }, "Util", "stopNonePlayerStream");

    logInfo << "stopNonePlayerStream: " << stopNonePlayerStream;

    if (!_origin || stopNonePlayerStream) {
        static int nonePlayerWaitTime = Config::instance()->getAndListen([](const json& config){
            nonePlayerWaitTime = Config::instance()->get("Util", "nonePlayerWaitTime");
        }, "Util", "nonePlayerWaitTime");

        logInfo << "nonePlayerWaitTime: " << nonePlayerWaitTime;

        weak_ptr<MediaSource> wSelf = shared_from_this();
        _loop->addTimerTask(nonePlayerWaitTime, [wSelf, size](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            logInfo << "onReaderChanged task";
            if (size == 0 && self->_origin) {
                lock_guard<recursive_mutex> lock(MediaSource::_mtxTotalSource);
                logInfo << "onReaderChanged _mtxStreamSource";
                if (self->_mapConnection.size() > 0 || self->_mapSink.size() > 0) {
                    return 0;
                }
                logInfo << "onReaderChanged relese";
                self->release();

            } else if (size == 0 && !self->_origin) {
                auto src = MediaSource::get(self->_urlParser.path_, self->_urlParser.vhost_);
                logInfo << "onReaderChanged get src";
                if (src) {
                    logInfo << "onReaderChanged _mtxStreamSource";
                    lock_guard<recursive_mutex> lck(src->_mtxStreamSource);
                    if (self->_mapConnection.size() > 0 || self->_mapSink.size() > 0) {
                        return 0;
                    }
                    logInfo << "onReaderChanged relese";
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

    StreamStatusInfo statusInfo{_urlParser.protocol_, _urlParser.path_, _urlParser.vhost_, 
                                _urlParser.type_, "on", "200"};
    MediaHook::instance()->onStreamStatus(statusInfo);

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

    string uri = _urlParser.path_;
    string url = pushProtocol + "://" + endpoint + uri;

    // TODO 可以指定多个endpoint进行推流
    auto pusher = MediaClient::createClient(pushProtocol, uri, MediaClientType_Push);
    pusher->start("0.0.0.0", 0, url, 5);

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
    logInfo << "del key: " << key;
    lock_guard<mutex> lck(_mtxConnection);
    _mapConnection.erase(key);
}

void MediaSource::addConnection(void* key)
{
    logInfo << "add key: " << key;
    lock_guard<mutex> lck(_mtxConnection);
    _mapConnection[key] = 1;
}
////////////////////////