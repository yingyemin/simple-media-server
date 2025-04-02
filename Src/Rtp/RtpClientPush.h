#ifndef RtpClientPush_h
#define RtpClientPush_h

#include "RtpClient.h"
#include "RtpConnectionSend.h"

using namespace std;

class RtpClientPush : public RtpClient {
public:
    using Ptr = shared_ptr<RtpClientPush>;
    using Wptr = weak_ptr<RtpClientPush>;

    RtpClientPush(const string& app, const string& stream, int ssrc, int sockType);
    ~RtpClientPush();

public:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPush();
    virtual void setPayloadType(const string& payloadType) {_payloadType = payloadType;}
    virtual void setOnlyTrack(const string& onlyTrack) {_onlyTrack = onlyTrack;}

private:
    // bool _firstWrite = true;
    bool _sendFlag = true;
    string _peerIp;
    int _peerPort;
    int _sockType;
    int _ssrc;
    string _streamName;
    string _appName;
    string _payloadType = "ps";
    string _onlyTrack = "all";

    Socket::Ptr _socket;
    sockaddr _addr;

    RtpConnectionSend::Ptr _connection;
};

#endif //RtpClientPull_h