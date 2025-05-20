#ifndef RtspConnection_h
#define RtspConnection_h

#include "TcpConnection.h"
#include "RtspParser.h"
#include "RtspSdpParser.h"
#include "RtspMediaSource.h"
#include "RtspRtpTransport.h"
#include "RtspRtcpTransport.h"
#include "Common/UrlParser.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class RtspConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<RtspConnection>;
    using Wptr = weak_ptr<RtspConnection>;

    RtspConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enableSsl = false);
    ~RtspConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onRtspPacket();

private:
    void authFailed();

    void handleOption();
    void handleDescribe();
    void handleAnnounce();
    void handleRecord();
    void handleSetup();
    void handlePlay();
    void handlePause();
    void handleTeardown();
    void handleGet();
    void handlePost();
    void handleSetParam();
    void handleGetParam();
    void handleAnnounce_l();
    void handleDescribe_l();
    void responseDescribe(const MediaSource::Ptr &src);

    void sendMessage(const string& msg);
    void sendBadRequst(const string& msg);
    void sendUnsupportedTransport();
    void sendNotAcceptable();
    void sendSessionNotFound();
    void sendStreamNotFound();

private:
    bool _isPublish = false;
    bool _closed = false;
    uint64_t _totalSendBytes = 0;
    uint64_t _intervalSendBytes = 0;
    float _lastBitrate = 0;
    string _baseUrl;
    string _authNonce;
    string _sessionId;
    string _payloadType;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtspParser _parser;
    RtspSdpParser _sdpParser;
    UrlParser _urlParser;
    RtspMediaSource::Wptr _source;
    RtspMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    // int : index
    unordered_map<int, RtspRtpTransport::Ptr> _mapRtpTransport;
    // int : index
    unordered_map<int, RtspRtcpTransport::Ptr> _mapRtcpTransport;
};

#endif //RtspConnection_h