#ifndef GB28181ClientPull_h
#define GB28181ClientPull_h

#include "GB28181Client.h"
#include "GB28181Context.h"
#include "GB28181Parser.h"

// using namespace std;

class GB28181ClientPull : public GB28181Client {
public:
    using Ptr = std::shared_ptr<GB28181ClientPull>;
    using Wptr = std::weak_ptr<GB28181ClientPull>;

    GB28181ClientPull(const std::string& app, const std::string& stream, int ssrc, int sockType);
    ~GB28181ClientPull();

private:
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPull();

private:
    // bool _firstWrite = true;
    int _sockType;
    int _ssrc;
    std::string _streamName;
    std::string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;

    GB28181Context::Ptr _context;
    GB28181Parser _parser;
};

#endif //GB28181ClientPull_h