#ifndef MediaSource_H
#define MediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <list>

#include "UrlParser.h"
#include "DataQue.h"
#include "Frame.h"
#include "Track.h"
#include "RecordReaderBase.h"
#include "EventPoller/EventLoop.h"
#include "Net/TcpConnection.h"
#include "MediaClient.h"

using namespace std;

enum SourceStatus {
    WAITING = 0,
    INIT, // 刚创建
    AVAILABLE, // 注册了，可用
    UNINIT // 准备注销
};

class MediaSource : public enable_shared_from_this<MediaSource>
{
public:
    using Ptr = shared_ptr<MediaSource>;
    using Wptr = weak_ptr<MediaSource>;
    // using FrameRingDataType = std::shared_ptr<list<FrameBuffer::Ptr> >;
    using FrameRingDataType = FrameBuffer::Ptr;
    using FrameRingType = DataQue<FrameRingDataType>;
    using onReadyFunc = function<void()>;
    using onDetachFunc = function<void()>;

    MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr);
    virtual ~MediaSource();

public:
    static MediaSource::Ptr getOrCreate(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const std::function<MediaSource::Ptr()> &create);

    static MediaSource::Ptr getOrCreateInSrt(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const std::function<MediaSource::Ptr()> &create);

    static void getOrCreateAsync(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* key);

    static void loadFromFile(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey);

    static void pullStreamFromOrigin(const string& uri, const string& vhost, const string &protocol, 
            const string& type, const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey);

    void pushStreamToOrigin();

    static void release(const string &uri, const string& vhost);

    static MediaSource::Ptr get(const string& uri, const string& vhost);
    static MediaSource::Ptr get(const string& uri, const string& vhost, const string& protocol, const string& type);

    static unordered_map<string/*uri_vhost*/ , MediaSource::Ptr> getAllSource();

public:
    bool getOrCreateAsync(const string &protocol, const string& type, 
            const function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create,
            void* key);

    MediaSource::Ptr getFromOrigin(const string& protocol, const string& type);

    MediaSource::Ptr getOrCreate(const string &protocol, const string& type, const std::function<MediaSource::Ptr()> &create);

    unordered_map<string/*protocol*/ , unordered_map<string/*type*/ , MediaSource::Wptr> > 
    getMuxerSource();

    void release();

    virtual void addSink(const MediaSource::Ptr &src);
    virtual void delSink(const MediaSource::Ptr &src);
    virtual void addSource(const MediaSource::Ptr &src);
    virtual void delSource(const MediaSource::Ptr &src);
    virtual void addTrack(const shared_ptr<TrackInfo>& track);
    virtual void onFrame(const FrameBuffer::Ptr& frame);
    virtual void onReaderChanged(int size);
    virtual void addOnDetach(void* key, const onDetachFunc& func);
    virtual void delOnDetach(void* key);
    virtual void addOnReady(void* key, const onReadyFunc& func);
    virtual void onReady();
    virtual void addConnection(void* key);
    virtual void delConnection(void* key);
    virtual unordered_map<int, shared_ptr<TrackInfo>> getTrackInfo();
    virtual int playerCount() {return 0;}
    virtual uint64_t getBytes() {return 0;}
    virtual void getClientList(const function<void(const list<ClientInfo>& info)>& func) {}
    virtual void setOriginSocket(const Socket::Ptr& socket) {_originSocket = socket;}
    virtual Socket::Ptr getOriginSocket() {return _originSocket.lock();}
    
    void setStatus(const SourceStatus status) {_status = status;}
    int getStatus() {return _status;}
    void setOrigin() {_origin = true;}
    void setAction(bool isPush) {_isPush = isPush;}
    bool getAction() {return _isPush;}
    void setOrigin(const MediaSource::Ptr &src);
    MediaSource::Ptr getOrigin();
    bool isOrigin() {return _origin;}
    void setLoop(const EventLoop::Ptr& loop) {_loop = loop;}
    EventLoop::Ptr getLoop() {return _loop;}
    string getProtocol() {return _urlParser.protocol_;}
    string getType() {return _urlParser.type_;}
    string getPath() {return _urlParser.path_;}
    string getVhost() {return _urlParser.vhost_;}
    uint64_t getCreateTime() { return _createTime; }

    bool isReady() {return _hasReady;}

protected:
    bool _origin = false;
    bool _isPush = true;
    uint64_t _createTime = 0;
    SourceStatus _status = WAITING;
    UrlParser _urlParser;
    EventLoop::Ptr _loop;
    MediaSource::Wptr _originSrc;
    RecordReaderBase::Ptr _recordReader;

    recursive_mutex _mtxStreamSource;
    unordered_map<string/*protocol*/ , unordered_map<string/*type*/ , MediaSource::Wptr> > _streamSource;
    int _sinkSize = 0;
    unordered_map<MediaSource*, MediaSource::Ptr> _mapSink;
    unordered_map<MediaSource*, MediaSource::Ptr> _mapSource;
    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
    // 用于给其他线程获取，需要加锁。上面的trackInfo用于本线程用，比较频繁，所以分成两个map，提升性能
    mutex _trackInfoLck;
    unordered_map<int, shared_ptr<TrackInfo>> _mapLockTrackInfo;

    bool _hasAudio = false;
    bool _hasVideo = false;
    bool _videoReady = false;
    bool _audioReady = false;
    bool _stage500Ms = true;

private:
    static recursive_mutex _mtxTotalSource;
    static unordered_map<string/*uri_vhost*/ , MediaSource::Ptr> _totalSource;
    static MediaClient::Ptr _player;
    static unordered_map<MediaClient*, MediaClient::Ptr> _mapPusher;

    static mutex _mtxRegister;
    static unordered_map<string/*uri_vhost_protocol_type*/ , vector<onReadyFunc>> _mapRegisterEvent;

private:
    bool _hasReady = false;
    Socket::Wptr _originSocket;
    shared_ptr<TimerTask> _task;
    mutex _mtxConnection;
    unordered_map<void*, int> _mapConnection;

    mutex _mtxOnReadyFunc;
    unordered_map<void*, onReadyFunc> _mapOnReadyFunc;

    mutex _mtxOnDetachFunc;
    unordered_map<void*, onDetachFunc> _mapOnDetachFunc;
};


#endif //MediaSource_H
