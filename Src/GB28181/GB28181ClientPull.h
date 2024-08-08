#ifndef GB28181ClientPull_h
#define GB28181ClientPull_h

#include "GB28181Client.h"
#include "GB28181Context.h"
#include "GB28181Parser.h"

using namespace std;

class GB28181ClientPull : public GB28181Client {
public:
    using Ptr = shared_ptr<GB28181ClientPull>;
    using Wptr = weak_ptr<GB28181ClientPull>;

    GB28181ClientPull(const string& ip, int port, const string& app, 
                    const string& stream, int ssrc, int sockType);
    ~GB28181ClientPull();

private:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPull();

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

    GB28181Context::Ptr _context;
    GB28181Parser _parser;
};

#endif //GB28181ClientPull_h