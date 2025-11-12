#ifndef GB28181SIPServer_h
#define GB28181SIPServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

// using namespace std;

class GB28181SIPServer : public std::enable_shared_from_this<GB28181SIPServer> {
public:
    using Ptr = std::shared_ptr<GB28181SIPServer>;
    using Wptr = std::weak_ptr<GB28181SIPServer>;

    GB28181SIPServer();
    ~GB28181SIPServer();

public:
    // 单例，为了能在其他的地方管理rtspserver，比如通过api管理
    static GB28181SIPServer::Ptr& instance();

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    // sockType: 1:tcp, 2:udp, 3:both
    void start(const std::string& ip, int port, int count, int sockType);
    void stopByPort(int port, int count, int sockType);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    
    void for_each_server(const std::function<void(const TcpServer::Ptr &)> &cb);
    void for_each_socket(const std::function<void(const Socket::Ptr &)> &cb);

private:
    std::mutex _mtx;
    // int : port
    std::unordered_map<int, std::vector<TcpServer::Ptr>> _tcpServers;
    std::unordered_map<int, std::vector<Socket::Ptr>> _udpSockets;
    
    std::unordered_map<int, std::vector<TcpServer::Ptr>> _tcpSendServers;
    std::unordered_map<int, std::vector<Socket::Ptr>> _udpSendSockets;
};

#endif //GB28181SIPServer_h