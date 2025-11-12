#ifndef Ehome2VodServer_h
#define Ehome2VodServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

// using namespace std;

class Ehome2VodServerInfo
{
public:
    int expire_ = 0;
    uint64_t timeout_ = 0;
    std::string serverId_;
    std::string path_;
    std::string appName_;
};

class Ehome2VodServer : public std::enable_shared_from_this<Ehome2VodServer> {
public:
    using Ptr = std::shared_ptr<Ehome2VodServer>;
    using Wptr = std::weak_ptr<Ehome2VodServer>;

    Ehome2VodServer();
    ~Ehome2VodServer();

public:
    // 单例，为了能在其他的地方管理rtspserver，比如通过api管理
    static Ehome2VodServer::Ptr& instance();

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    // sockType: 1:tcp, 2:udp, 3:both
    void start(const std::string& ip, const std::string& streamId, int port, int sockType);
    void stopByPort(int port, int sockType);
    void delServer(const TcpServer::Ptr& server);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    
    void for_each_server(const std::function<void(const TcpServer::Ptr &)> &cb);
    void for_each_socket(const std::function<void(const Socket::Ptr &)> &cb);

    void setServerInfo(int port, const std::string& path, int expire, const std::string& appName, int timeout);

private:
    std::mutex _mtx;
    // int : port
    std::unordered_map<int, std::vector<TcpServer::Ptr>> _tcpServers;
    std::unordered_map<int, std::vector<Socket::Ptr>> _udpSockets;
    std::unordered_map<int, Ehome2VodServerInfo> _serverInfo;
};

#endif //Ehome2VodServer_h