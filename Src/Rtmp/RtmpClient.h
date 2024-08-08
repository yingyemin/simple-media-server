#ifndef RtmpClient_h
#define RtmpClient_h

#include "Net/TcpClient.h"
#include "Common/UrlParser.h"
#include "RtmpMediaSource.h"
#include "Amf.h"
#include "Decode/RtmpDecode.h"
#include "RtmpChunk.h"
#include "RtmpHandshake.h"
#include "Common/MediaClient.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

enum RtmpState
{
    RTMP_SEND_C0C1,
    RTMP_SEND_C2,
    RTMP_SEND_CONNECT,
    RTMP_SEND_CREATESTREAM,
    RTMP_SEND_PLAY_PUBLISH,
    RTMP_FINISH_PLAY_PUBLISH,
    RTMP_SEND_PAUSE,
    RTMP_SEND_STOP
};

class RtmpClient : public TcpClient, public MediaClient
{
public:
    using Ptr = shared_ptr<RtmpClient>;
    RtmpClient(MediaClientType type, const string& appName, const string& streamName);
    ~RtmpClient();

public:
    // override MediaClient
    void start(const string& localIp, int localPort, const string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;

    // static void addRtmpClient(const string& key, const RtmpClient::Ptr& client);
    // static void delRtmpClient(const string& key);
    // static RtmpClient::Ptr getRtmpClient(const string& key);

protected:
    // override TcpClient
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void onError(const string& err);
    void close();
    void onConnect();

protected:
    void sendC0C1();
    void sendSetChunkSize();
    void sendConnect();
    void sendCreateStream();
    void sendPause();
    void sendDeleteStream();
    bool sendMetaData(AmfObjects meta_data);

    void sendPlayOrPublish();

    void onRtmpChunk(RtmpMessage msg);
    void sendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg);
    bool sendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payload_size);
    bool handleResponse(RtmpMessage& rtmp_msg);
    void handleCmdResult(int bytes_used, RtmpMessage& rtmp_msg);
    void handleCmdOnStatus(int bytes_used, RtmpMessage& rtmp_msg);
    void handleOnMetaData();
    bool handleVideo(RtmpMessage& rtmp_msg);
    bool handleAudio(RtmpMessage& rtmp_msg);
    bool handlePlay();
    bool handlePublish();
    void onPublish(const MediaSource::Ptr &src);

private:
    // static mutex _mapMtx;
    // static unordered_map<string, RtmpClient::Ptr> _mapRtmpClient;

    bool _validVideoTrack = true;
    bool _validAudioTrack = true;
    int _sendInvokerId = 0;
    int _streamId = 0;
    RtmpState _state = RTMP_SEND_C0C1;
    MediaClientType _type = MediaClientType_Pull;

    int _avcHeaderSize = 0;
    shared_ptr<char> _avcHeader;
    int _aacHeaderSize = 0;
    shared_ptr<char> _aacHeader;

    string _localAppName;
    string _localStreamName;

    string _url;
    string _tcUrl;
    string _peerAppName;
    string _peerStreamName;
    AmfDecoder _amfDecoder;
    AmfEncoder _amfEncoder;
    AmfObjects _metaData;
    RtmpChunk _chunk;
    
    UrlParser _localUrlParser;
    UrlParser _peerUrlParser;

    RtmpDecodeTrack::Ptr _rtmpVideoDecodeTrack;
    RtmpDecodeTrack::Ptr _rtmpAudioDecodeTrack;
    shared_ptr<RtmpHandshake> _handshake;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtmpMediaSource::Wptr _source;
    RtmpMediaSource::RingType::DataQueReaderT::Ptr _playReader;

    function<void()> _onClose;
};

#endif //RtmpClient_h