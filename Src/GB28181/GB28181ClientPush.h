#ifndef GB28181ClientPush_h
#define GB28181ClientPush_h

#include "GB28181Client.h"
#include "GB28181ConnectionSend.h"

using namespace std;

class GB28181ClientPush : public GB28181Client {
public:
    using Ptr = shared_ptr<GB28181ClientPush>;
    using Wptr = weak_ptr<GB28181ClientPush>;

    GB28181ClientPush(const string& ip, int port, const string& app, 
                    const string& stream, int ssrc, int sockType);
    ~GB28181ClientPush();

private:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPush();

private:
    // bool _firstWrite = true;
    bool _sendFlag = true;
    string _peerIp;
    int _peerPort;
    int _sockType;
    int _ssrc;
    string _streamName;
    string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;

    GB28181ConnectionSend::Ptr _connection;
};

#endif //GB28181ClientPull_h