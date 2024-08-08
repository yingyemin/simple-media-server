#ifndef WebrtcServer_h
#define WebrtcServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class WebrtcServer : public enable_shared_from_this<WebrtcServer> {
public:
    using Ptr = shared_ptr<WebrtcServer>;
    using Wptr = weak_ptr<WebrtcServer>;

    WebrtcServer();
    ~WebrtcServer();

public:
    // 单例，为了能在其他的地方管理rtspserver，比如通过api管理
    static WebrtcServer::Ptr& instance();

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    // sockType: 1:tcp, 2:udp, 3:both
    void start(const string& ip, int port, int count, int sockType);
    void stopByPort(int port, int count, int sockType);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    
    void for_each_server(const function<void(const TcpServer::Ptr &)> &cb);
    void for_each_socket(const function<void(const Socket::Ptr &)> &cb);

private:
    mutex _mtx;
    // int : port
    unordered_map<int, vector<TcpServer::Ptr>> _tcpServers;
    unordered_map<int, vector<Socket::Ptr>> _udpSockets;
};

#endif //GB28181Server_h