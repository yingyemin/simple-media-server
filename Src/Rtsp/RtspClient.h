#ifndef RtspClient_h
#define RtspClient_h

#include "Net/TcpClient.h"
#include "Common/UrlParser.h"
#include "Common/MediaClient.h"
#include "RtspParser.h"
#include "RtspSdpParser.h"
#include "RtspRtpTransport.h"
#include "RtspRtcpTransport.h"
#include "RtspMediaSource.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

enum RtspState
{
    RTSP_SEND_OPTION,
    RTSP_SEND_DESCRIBE_ANNOUNCE,
    RTSP_SEND_SETUP,
    RTSP_SEND_PLAY_PUBLISH,
    RTSP_FINISH_PLAY_PUBLISH,
    RTSP_SEND_PAUSE,
    RTSP_SEND_STOP
};

class RtspClient : public TcpClient, public MediaClient
{
public:
    using Ptr = shared_ptr<RtspClient>;
    RtspClient(MediaClientType type, const string& appName, const string& streamName);
    ~RtspClient();

public:
    static void init();

    void setUsername(const string& username) {_username = username;}
    void setPassword(const string& pwd) {_pwd = pwd;}

public:
    // override MediaClient
    void start(const string& localIp, int localPort, const string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;
    void addOnReady(void* key, const function<void()>& onReady) override;
    void setTransType(int type) override;

protected:
    // override TcpClient
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void onError(const string& err);
    void close();
    void onConnect();
    void onRtspPacket();

protected:
    void sendOption();
    void sendDescribeOrAnnounce();
    void sendDescribeWithAuthInfo();
    void sendSetup();
    void sendSetup(const RtspMediaSource::Ptr& rtspSrc);
    void sendPlayOrPublish();
    void sendPause(int seekMs = 0);
    void sendTeardown();

    void sendMessage(const string& msg);
    void bindRtxpSocket();

private:
    bool _hasAuth = false;
    int _seq = 0;
    int _setupIndex = 0;
    string _username;
    string _pwd;
    TransportType _rtpType = Transport_TCP;
    RtspState _state = RTSP_SEND_OPTION;
    MediaClientType _type;
    string _url;
    string _sessionId;
    UrlParser _localUrlParser;
    UrlParser _peerUrlParser;
    RtspParser _parser;
    RtspSdpParser _sdpParser;

    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtspMediaSource::Wptr _source;
    RtspMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    // int : index
    unordered_map<int, RtspRtpTransport::Ptr> _mapRtpTransport;
    // int : index
    unordered_map<int, RtspRtcpTransport::Ptr> _mapRtcpTransport;

    function<void()> _onClose;
    mutex _mtx;
    unordered_map<void*, function<void()>> _mapOnReady;
};

#endif // RtspClient_h