﻿#ifndef WebrtcP2PClient_H
#define WebrtcP2PClient_H

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
#include "WebrtcIce.h"


using namespace std;

class WebrtcP2PClient : public MediaClient, public enable_shared_from_this<WebrtcP2PClient>
{
public:
    using Ptr = shared_ptr<WebrtcP2PClient>;
    using Wptr = weak_ptr<WebrtcP2PClient>;

    WebrtcP2PClient(MediaClientType type, const string& appName, const string& streamName);
    ~WebrtcP2PClient();

public:
    static void init();

public:
    // override MediaClient
    bool start(const string& localIp, int localPort, const string& param, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;
    void addOnReady(void* key, const function<void()>& onReady) override;
    void getProtocolAndType(string& protocol, MediaClientType& type) override;

public:
    string getPath() {return _urlParser.path_;}
    string getSourceUrl() {return _sourceUrl;}

public:
    void close();
    string getLocalSdp();
    void setRemoteSdp(const string& sdp);

private:
    void onError(const string& err);
    void onConnected();
    void onRead(const StreamBuffer::Ptr& buffer);
    void initPuller();
    void initPusher();
    void initLocalSdpTitle(stringstream& ss, int trackNum);
    void initPusherLocalSdpMedia(stringstream& ss, const shared_ptr<TrackInfo>& videoInfo, const shared_ptr<TrackInfo>& audioInfo);
    void initPullerLocalSdpMedia(stringstream& ss);
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
    string _request = "pull";
    string _iceUfrag;
    string _icePwd;
    string _username;
    string _sourceUrl;
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
    WebrtcIce::Ptr _ice;
    struct sockaddr* _addr = nullptr;
	int _addrLen = 0;
    WebrtcMediaSource::Wptr _source;
    shared_ptr<WebrtcSdp> _localSdp;
    shared_ptr<WebrtcSdp> _remoteSdp;
    shared_ptr<DtlsSession> _dtlsSession;
    shared_ptr<SrtpSession> _srtpSession;
    shared_ptr<TimerTask> _dtlsTimeTask;
    WebrtcMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    function<void()> _onClose;
    mutex _mtx;
    unordered_map<void*, function<void()>> _mapOnReady;
    unordered_map<string, string> _mapParam;
};


#endif //WebrtcP2PClient_H
