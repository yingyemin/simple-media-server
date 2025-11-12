#ifndef JT808Manager_h
#define JT808Manager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "JT808Context.h"
#include "Net/Socket.h"

// using namespace std;

class JT808Manager : public std::enable_shared_from_this<JT808Manager> {
public:
    using Ptr = std::shared_ptr<JT808Manager>;
    using Wptr = std::weak_ptr<JT808Manager>;

    JT808Manager();
    ~JT808Manager();

public:
    static JT808Manager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onJT808Packet(const Socket::Ptr& socket, const StreamBuffer::Ptr& req, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(const std::string& deviceId, const JT808Context::Ptr& context);
    void delContext(const std::string& deviceId);
    JT808Context::Ptr getContext(const std::string& deviceId);
    std::unordered_map<std::string, JT808Context::Ptr> getContexts();

private:
    bool _isInited = false;
    std::mutex _contextLck;
    // EventLoop::Ptr _loop;
    std::unordered_map<std::string, JT808Context::Ptr> _mapContext;
};

#endif //GB28181SIPManager_h