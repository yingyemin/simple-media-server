#ifndef JT808Manager_h
#define JT808Manager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "JT808Context.h"
#include "Net/Socket.h"

using namespace std;

class JT808Manager : public enable_shared_from_this<JT808Manager> {
public:
    using Ptr = shared_ptr<JT808Manager>;
    using Wptr = weak_ptr<JT808Manager>;

    JT808Manager();
    ~JT808Manager();

public:
    static JT808Manager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onJT808Packet(const Socket::Ptr& socket, const StreamBuffer::Ptr& req, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(const string& deviceId, const JT808Context::Ptr& context);
    void delContext(const string& deviceId);
    JT808Context::Ptr getContext(const string& deviceId);
    unordered_map<string, JT808Context::Ptr> getContexts();

private:
    bool _isInited = false;
    mutex _contextLck;
    // EventLoop::Ptr _loop;
    unordered_map<string, JT808Context::Ptr> _mapContext;
};

#endif //GB28181SIPManager_h