#ifndef HttpServer_h
#define HttpServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class HttpServer : public enable_shared_from_this<HttpServer> {
public:
    using Ptr = shared_ptr<HttpServer>;
    using Wptr = weak_ptr<HttpServer>;

    HttpServer();
    ~HttpServer();

public:
    // 单例，为了能在其他的地方管理HttpServer，比如通过api管理
    static HttpServer::Ptr& instance();
    static HttpServer::Ptr& instance(const string& key);

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    void start(const string& ip, int port, int count, bool enbaleSsl = false, bool isWebsocket = false);
    void stopByPort(int port, int count);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    void setServerId(const string& key);
    
    void for_each_server(const function<void(const TcpServer::Ptr &)> &cb);

private:
    int _port;
    string _ip;
    string _serverId;
    mutex _mtx;
    // int : port
    unordered_map<int, vector<TcpServer::Ptr>> _tcpServers;
};

#endif //HttpServer_h