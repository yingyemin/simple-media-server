#ifndef WebrtcClient_H
#define WebrtcClient_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/Socket.h"
#include "Common/UrlParser.h"
#include "Common/MediaClient.h"
#include "Http/HttpClientApi.h"
#include "WebrtcSdpParser.h"
#include "WebrtcMediaSource.h"
#include "WebrtcDtlsSession.h"
#include "WebrtcSrtpSession.h"
#include "WebrtcParser.h"


// using namespace std;

class WebrtcClient : public MediaClient, public std::enable_shared_from_this<WebrtcClient>
{
public:
    using Ptr = std::shared_ptr<WebrtcClient>;
    using Wptr = std::weak_ptr<WebrtcClient>;

    WebrtcClient(MediaClientType type, const std::string& appName, const std::string& streamName);
    ~WebrtcClient();

public:
    static void init();

public:
    // override MediaClient
    bool start(const std::string& localIp, int localPort, const std::string& param, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const std::function<void()>& cb) override;
    void addOnReady(void* key, const std::function<void()>& onReady) override;
    void getProtocolAndType(std::string& protocol, MediaClientType& type) override;

public:
    std::string getPath() {return _urlParser.path_;}
    std::string getSourceUrl() {return _sourceUrl;}

public:
    void close();
    std::string getLocalSdp();
    void setRemoteSdp(const std::string& sdp);

private:
    void onError(const std::string& err);
    void onConnected();
    void onRead(const StreamBuffer::Ptr& buffer);
    void initPuller();
    void initPusher();
    void initLocalSdpTitle(std::stringstream& ss, int trackNum);
    void initPusherLocalSdpMedia(std::stringstream& ss, const std::shared_ptr<TrackInfo>& videoInfo, const std::shared_ptr<TrackInfo>& audioInfo);
    void initPullerLocalSdpMedia(std::stringstream& ss);    
    void onRtcPacket(const char* data, int len);

    void onStunPacket(const StreamBuffer::Ptr& buffer);
    int64_t onDtlsCheck();
    void onDtlsPacket(const StreamBuffer::Ptr& buffer);
    void onRtcpPacket(const StreamBuffer::Ptr& buffer);
    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void startPlay();
    void startPlay(const MediaSource::Ptr &src);
    void sendRtpPacket(const WebrtcMediaSource::DataType &pack);
    void sendMedia(const RtpPacket::Ptr& rtp);

private:
    bool _inited = false;
    bool _dtlsHandshakeDone = false;
    bool _sendDtls = false;
    bool _enableDtls = true;
    bool _enableSrtp = true;
    std::string _request = "pull";
    std::string _iceUfrag;
    std::string _icePwd;
    std::string _username;
    std::string _sourceUrl;
    UrlParser _urlParser;
    UrlParser _peerUrlParser;
    WebrtcParser _parser;
    // HttpClientApi _apiClient;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    WebrtcPtInfo::Ptr _videoPtInfo;
    WebrtcPtInfo::Ptr _audioPtInfo;
    WebrtcDecodeTrack::Ptr _videoDecodeTrack;
    WebrtcDecodeTrack::Ptr _audioDecodeTrack;
    struct sockaddr* _addr = nullptr;
	int _addrLen = 0;
    WebrtcMediaSource::Wptr _source;
    std::shared_ptr<WebrtcSdp> _localSdp;
    std::shared_ptr<WebrtcSdp> _remoteSdp;
    std::shared_ptr<DtlsSession> _dtlsSession;
    std::shared_ptr<SrtpSession> _srtpSession;
    std::shared_ptr<TimerTask> _dtlsTimeTask;
    WebrtcMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    std::function<void()> _onClose;
    std::mutex _mtx;
    std::unordered_map<void*, std::function<void()>> _mapOnReady;
    std::unordered_map<std::string, std::string> _mapParam;
};


#endif //Ehome2Connection_H
