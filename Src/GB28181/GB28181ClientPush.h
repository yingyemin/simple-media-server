#ifndef GB28181ClientPush_h
#define GB28181ClientPush_h

#include "GB28181Client.h"
#include "GB28181ConnectionSend.h"

// using namespace std;

class GB28181ClientPush : public GB28181Client {
public:
    using Ptr = std::shared_ptr<GB28181ClientPush>;
    using Wptr = std::weak_ptr<GB28181ClientPush>;

    GB28181ClientPush(const std::string& app, const std::string& stream, int ssrc, int sockType);
    ~GB28181ClientPush();

private:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPush();

private:
    // bool _firstWrite = true;
    int _sockType;
    int _ssrc;
    std::string _streamName;
    std::string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;

    GB28181ConnectionSend::Ptr _connection;
};

#endif //GB28181ClientPull_h