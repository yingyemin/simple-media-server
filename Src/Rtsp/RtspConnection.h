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

// using namespace std;

class RtspConnection : public TcpConnection
{
public:
    using Ptr = std::shared_ptr<RtspConnection>;
    using Wptr = std::weak_ptr<RtspConnection>;

    RtspConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enableSsl = false);
    ~RtspConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const std::string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;
    ssize_t send(Buffer::Ptr pkt, bool flag, size_t offset = 0, size_t len = 0) override;

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

    void sendMessage(const std::string& msg);
    void sendBadRequst(const std::string& msg);
    void sendUnsupportedTransport();
    void sendNotAcceptable();
    void sendSessionNotFound();
    void sendStreamNotFound();

    void onCombineMediaSource();
    void onPollingMediaSource();

private:
    bool _isPublish = false;
    bool _closed = false;
    uint64_t _totalSendBytes = 0;
    uint64_t _intervalSendBytes = 0;
    float _lastBitrate = 0;
    std::string _baseUrl;
    std::string _authNonce;
    std::string _sessionId;
    std::string _payloadType;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtspParser _parser;
    RtspSdpParser _sdpParser;
    UrlParser _urlParser;
    RtspMediaSource::Wptr _source;
    RtspMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    // int : index
    std::unordered_map<int, RtspRtpTransport::Ptr> _mapRtpTransport;
    // int : index
    std::unordered_map<int, RtspRtcpTransport::Ptr> _mapRtcpTransport;
};

#endif //RtspConnection_h