#ifndef RtpClientPush_h
#define RtpClientPush_h

#include "RtpClient.h"
#include "RtpConnectionSend.h"

using namespace std;

class RtpClientPush : public RtpClient {
public:
    using Ptr = std::shared_ptr<RtpClientPush>;
    using Wptr = std::weak_ptr<RtpClientPush>;

    RtpClientPush(const std::string& app, const std::string& stream, int ssrc, int sockType);
    ~RtpClientPush();

public:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPush();
    virtual void setPayloadType(const std::string& payloadType) {_payloadType = payloadType;}
    virtual void setOnlyTrack(const std::string& onlyTrack) {_onlyTrack = onlyTrack;}

private:
    // bool _firstWrite = true;
    bool _sendFlag = true;
    std::string _peerIp;
    int _peerPort;
    int _sockType;
    int _ssrc;
    std::string _streamName;
    std::string _appName;
    std::string _payloadType = "ps";
    std::string _onlyTrack = "all";

    Socket::Ptr _socket;
    sockaddr _addr;

    RtpConnectionSend::Ptr _connection;
};

#endif //RtpClientPull_h