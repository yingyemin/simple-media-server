#ifndef RtmpConnection_h
#define RtmpConnection_h

#include "TcpConnection.h"
#include "Common/UrlParser.h"
#include "RtmpHandshake.h"
#include "RtmpChunk.h"
#include "RtmpMediaSource.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class RtmpConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<RtmpConnection>;
    using Wptr = weak_ptr<RtmpConnection>;

    RtmpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~RtmpConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onRtmpChunk(RtmpMessage msg);
    bool handleInvoke(RtmpMessage& rtmp_msg);
    bool handleNotify(RtmpMessage& rtmp_msg);
    bool handleUserEvent(RtmpMessage& rtmp_msg);
    bool handleVideo(RtmpMessage& rtmp_msg);
    bool handleAudio(RtmpMessage& rtmp_msg);
    bool handleConnect();
    bool handleCreateStream();
    bool handlePublish();
    bool handlePlay();
    bool handlePlay2();
    void responsePlay(const MediaSource::Ptr &src);
    bool handleDeleteStream();
    bool sendMetaData(AmfObjects meta_data);
    void setPeerBandwidth();
    void sendAcknowledgement();
    void setChunkSize();
    bool sendInvokeMessage(uint32_t csid, const StreamBuffer::Ptr& payload, uint32_t payload_size);
    bool sendNotifyMessage(uint32_t csid, const StreamBuffer::Ptr& payload, uint32_t payload_size);
    bool isKeyFrame(const StreamBuffer::Ptr&, uint32_t payload_size);
    void sendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg);

private:

private:
    bool _isPublish = false;
    bool _isPlay = false;
    bool _validVideoTrack = true;
    bool _validAudioTrack = true;
    bool _addMute = false;
    uint32_t _streamId = 0;
    uint64_t _totalSendBytes = 0;
    uint64_t _intervalSendBytes = 0;
    uint64_t _lastMuteId = 0;
    float _lastBitrate = 0;
    int _avcHeaderSize = 0;
    StreamBuffer::Ptr _avcHeader;
    int _aacHeaderSize = 0;
    StreamBuffer::Ptr _aacHeader;
    string _streamName;
    string _app;
    string _tcUrl;
    string _path;
    string _status;
    AmfObjects _metaData;
    AmfDecoder _amfDecoder;
    AmfEncoder _amfEncoder;
    RtmpChunk _chunk;
    UrlParser _urlParser;
    RtmpDecodeTrack::Ptr _rtmpVideoDecodeTrack;
    RtmpDecodeTrack::Ptr _rtmpAudioDecodeTrack;
    shared_ptr<RtmpHandshake> _handshake;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtmpMediaSource::Wptr _source;
    RtmpMediaSource::RingType::DataQueReaderT::Ptr _playReader;
};

#endif //RtmpConnection_h