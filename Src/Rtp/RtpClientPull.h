#ifndef RtpClientPull_h
#define RtpClientPull_h

#include "RtpClient.h"
#include "RtpContext.h"
#include "RtpParser.h"

// using namespace std;

class RtpClientPull : public RtpClient {
public:
    using Ptr = std::shared_ptr<RtpClientPull>;
    using Wptr = std::weak_ptr<RtpClientPull>;

    RtpClientPull(const std::string& app, 
                    const std::string& stream, int ssrc, int sockType);
    ~RtpClientPull();

private:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPull();

private:
    // bool _firstWrite = true;
    bool _sendFlag = true;
    std::string _peerIp;
    int _peerPort;
    int _sockType;
    int _ssrc;
    std::string _streamName;
    std::string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;

    RtpContext::Ptr _context;
    RtpParser _parser;
};

#endif //RtpClientPull_h