#ifndef Ehome2VodServer_h
#define Ehome2VodServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class Ehome2VodServerInfo
{
public:
    int expire_ = 0;
    uint64_t timeout_ = 0;
    string serverId_;
    string path_;
    string appName_;
};

class Ehome2VodServer : public enable_shared_from_this<Ehome2VodServer> {
public:
    using Ptr = shared_ptr<Ehome2VodServer>;
    using Wptr = weak_ptr<Ehome2VodServer>;

    Ehome2VodServer();
    ~Ehome2VodServer();

public:
    // 单例，为了能在其他的地方管理rtspserver，比如通过api管理
    static Ehome2VodServer::Ptr& instance();

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    // sockType: 1:tcp, 2:udp, 3:both
    void start(const string& ip, const string& streamId, int port, int sockType);
    void stopByPort(int port, int sockType);
    void delServer(const TcpServer::Ptr& server);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    
    void for_each_server(const function<void(const TcpServer::Ptr &)> &cb);
    void for_each_socket(const function<void(const Socket::Ptr &)> &cb);

    void setServerInfo(int port, const string& path, int expire, const string& appName, int timeout);

private:
    mutex _mtx;
    // int : port
    unordered_map<int, vector<TcpServer::Ptr>> _tcpServers;
    unordered_map<int, vector<Socket::Ptr>> _udpSockets;
    unordered_map<int, Ehome2VodServerInfo> _serverInfo;
};

#endif //Ehome2VodServer_h