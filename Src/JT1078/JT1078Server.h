#ifndef JT1078Server_h
#define JT1078Server_h

#include "TcpServer.h"
#include "Common/PortManager.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

// using namespace std;

class JT1078ServerInfo
{
public:
    int expire_ = 0;
    uint64_t timeout_ = 0;
    std::string serverId_;
    std::string path_;
    std::string appName_;
};

class JT1078Server : public std::enable_shared_from_this<JT1078Server> {
public:
    using Ptr = std::shared_ptr<JT1078Server>;
    using Wptr = std::weak_ptr<JT1078Server>;

    JT1078Server();
    ~JT1078Server();

public:
    // 单例，为了能在其他的地方管理HttpServer，比如通过api管理
    static JT1078Server::Ptr& instance();
    static JT1078Server::Ptr& instance(const std::string& key);

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    void start(const std::string& ip, int port, int count, bool isTalk = false);
    void stopByPort(int port, int count);
    void delServer(const TcpServer::Ptr& server);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    void setServerId(const std::string& key);
    void setStreamPath(int port, const std::string& path, int expire, const std::string& appName, int timeout);
    void setPortRange(int minPort, int maxPort);
    int getPort() {return _portManager.get();}
    PortInfo getPortInfo() {return _portManager.getPortInfo();}
    
    void for_each_server(const std::function<void(const TcpServer::Ptr &)> &cb);

private:
    int _port;
    std::string _ip;
    std::string _serverId;
    std::string _path;
    PortManager _portManager;

    std::mutex _mtx;
    // int : port
    std::unordered_map<int, std::vector<TcpServer::Ptr>> _tcpServers;
    std::unordered_map<int, JT1078ServerInfo> _serverInfo;
};

#endif //JT1078Server_h