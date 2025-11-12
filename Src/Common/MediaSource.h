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

// using namespace std;

enum SourceStatus {
    WAITING = 0,
    INIT, // 刚创建
    AVAILABLE, // 注册了，可用
    UNINIT // 准备注销
};

class MediaSource : public std::enable_shared_from_this<MediaSource>
{
public:
    using Ptr = std::shared_ptr<MediaSource>;
    using Wptr = std::weak_ptr<MediaSource>;
    // using FrameRingDataType = std::shared_ptr<list<FrameBuffer::Ptr> >;
    using FrameRingDataType = FrameBuffer::Ptr;
    using FrameRingType = DataQue<FrameRingDataType>;
    using onReadyFunc = std::function<void()>;
    using onDetachFunc = std::function<void()>;

    MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr);
    virtual ~MediaSource();

public:
    static MediaSource::Ptr getOrCreate(const std::string& uri, const std::string& vhost, const std::string &protocol, 
            const std::string& type, const std::function<MediaSource::Ptr()> &create);

    static MediaSource::Ptr getOrCreateInSrt(const std::string& uri, const std::string& vhost, const std::string &protocol, 
            const std::string& type, const std::function<MediaSource::Ptr()> &create);

    static void getOrCreateAsync(const std::string& uri, const std::string& vhost, const std::string &protocol, 
            const std::string& type, const std::function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* key);

    static void loadFromFile(const std::string& uri, const std::string& vhost, const std::string &protocol, 
            const std::string& type, const std::function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey);

    static void pullStreamFromOrigin(const std::string& uri, const std::string& vhost, const std::string &protocol, 
            const std::string& type, const std::function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create, void* connKey);

    void pushStreamToOrigin();

    static void release(const std::string &uri, const std::string& vhost);

    static MediaSource::Ptr get(const std::string& uri, const std::string& vhost);
    static MediaSource::Ptr get(const std::string& uri, const std::string& vhost, const std::string& protocol, const std::string& type);

    static unordered_map<std::string/*uri_vhost*/ , MediaSource::Ptr> getAllSource();

public:
    bool getOrCreateAsync(const std::string &protocol, const std::string& type, 
            const std::function<void(const MediaSource::Ptr &src)> &cb, 
            const std::function<MediaSource::Ptr()> &create,
            void* key);

    MediaSource::Ptr getFromOrigin(const std::string& protocol, const std::string& type);

    MediaSource::Ptr getOrCreate(const std::string &protocol, const std::string& type, const std::function<MediaSource::Ptr()> &create);

    std::unordered_map<std::string/*protocol*/ , std::unordered_map<std::string/*type*/ , MediaSource::Wptr> > 
    getMuxerSource();

    void release();

    virtual void addSink(const MediaSource::Ptr &src);
    virtual void delSink(const MediaSource::Ptr &src);
    virtual void addSource(const MediaSource::Ptr &src);
    virtual void delSource(const MediaSource::Ptr &src);
    virtual void addTrack(const std::shared_ptr<TrackInfo>& track);
    virtual void onFrame(const FrameBuffer::Ptr& frame);
    virtual void onReaderChanged(int size);
    virtual void addOnDetach(void* key, const onDetachFunc& func);
    virtual void delOnDetach(void* key);
    virtual void addOnReady(void* key, const onReadyFunc& func);
    virtual void onReady();
    virtual void addConnection(void* key);
    virtual void delConnection(void* key);
    virtual std::unordered_map<int, std::shared_ptr<TrackInfo>> getTrackInfo();
    virtual int playerCount() {return 0;}
    virtual int totalPlayerCount();
    virtual uint64_t getBytes() {return 0;}
    virtual float getBitrate() {return _bitrate;}
    virtual void getClientList(const std::function<void(const std::list<ClientInfo>& info)>& func) {}
    virtual void setOriginSocket(const Socket::Ptr& socket) {_originSocket = socket;}
    virtual Socket::Ptr getOriginSocket() {return _originSocket.lock();}
    virtual void setAudioStampMode(int mode) {_audioStampMode = mode;}
    virtual int getAudioStampMode() {return _audioStampMode;}
    virtual void setVideoStampMode(int mode) {_videoStampMode = mode;}
    virtual int getVideoStampMode() {return _videoStampMode;}
    
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
    std::string getProtocol() {return _urlParser.protocol_;}
    std::string getType() {return _urlParser.type_;}
    std::string getPath() {return _urlParser.path_;}
    std::string getVhost() {return _urlParser.vhost_;}
    uint64_t getCreateTime() { return _createTime; }

    bool isReady() {return _hasReady;}
    RecordReaderBase::Ptr getReader();

protected:
    bool _origin = false;
    bool _isPush = true;
    uint64_t _createTime = 0;
    SourceStatus _status = WAITING;
    UrlParser _urlParser;
    EventLoop::Ptr _loop;
    MediaSource::Wptr _originSrc;
    RecordReaderBase::Ptr _recordReader;

    std::recursive_mutex _mtxStreamSource;
    std::unordered_map<std::string/*protocol*/ , std::unordered_map<std::string/*type*/ , MediaSource::Wptr> > _streamSource;
    int _sinkSize = 0;
    std::unordered_map<MediaSource*, MediaSource::Ptr> _mapSink;
    std::unordered_map<MediaSource*, MediaSource::Ptr> _mapSource;
    std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
    // 用于给其他线程获取，需要加锁。上面的trackInfo用于本线程用，比较频繁，所以分成两个map，提升性能
    std::mutex _trackInfoLck;
    std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapLockTrackInfo;

    bool _hasAudio = false;
    bool _hasVideo = false;
    bool _videoReady = false;
    bool _audioReady = false;
    bool _stage500Ms = true;

private:
    static std::recursive_mutex _mtxTotalSource;
    static std::unordered_map<std::string/*uri_vhost*/ , MediaSource::Ptr> _totalSource;
    static std::unordered_map<MediaClient*, MediaClient::Ptr> _mapPlayer;

    static std::mutex _mtxRegister;
    static std::unordered_map<std::string/*uri_vhost_protocol_type*/ , std::vector<onReadyFunc>> _mapRegisterEvent;

private:
    bool _hasReady = false;
    int _audioStampMode = 0; //useSourceStamp;
    int _videoStampMode = 0; //useSourceStamp;
    float _bitrate = 0;
    uint64_t _lastBytes_5s = 0;
    Socket::Wptr _originSocket;
    std::shared_ptr<TimerTask> _task;
    std::mutex _mtxConnection;
    std::unordered_map<void*, int> _mapConnection;
    std::unordered_map<MediaClient*, MediaClient::Ptr> _mapPusher;

    std::mutex _mtxOnReadyFunc;
    std::unordered_map<void*, onReadyFunc> _mapOnReadyFunc;

    std::mutex _mtxOnDetachFunc;
    std::unordered_map<void*, onDetachFunc> _mapOnDetachFunc;

    std::list<FrameBuffer::Ptr> _frameList;
};


#endif //MediaSource_H
