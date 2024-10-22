#ifndef JT1078Server_h
#define JT1078Server_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class JT1078ServerInfo
{
public:
    string serverId_;
    string path_;
};

class JT1078Server : public enable_shared_from_this<JT1078Server> {
public:
    using Ptr = shared_ptr<JT1078Server>;
    using Wptr = weak_ptr<JT1078Server>;

    JT1078Server();
    ~JT1078Server();

public:
    // 单例，为了能在其他的地方管理HttpServer，比如通过api管理
    static JT1078Server::Ptr& instance();
    static JT1078Server::Ptr& instance(const string& key);

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    void start(const string& ip, int port, int count);
    void stopByPort(int port, int count);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    void setServerId(const string& key);
    void setStreamPath(int port, const string& path);
    
    void for_each_server(const function<void(const TcpServer::Ptr &)> &cb);

private:
    int _port;
    string _ip;
    string _serverId;
    string _path;
    mutex _mtx;
    // int : port
    unordered_map<int, vector<TcpServer::Ptr>> _tcpServers;
    unordered_map<int, JT1078ServerInfo> _serverInfo;
};

#endif //JT1078Server_h