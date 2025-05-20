#ifndef RtspServer_h
#define RtspServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class RtspServer : public enable_shared_from_this<RtspServer> {
public:
    using Ptr = shared_ptr<RtspServer>;
    using Wptr = weak_ptr<RtspServer>;

    RtspServer();
    ~RtspServer();

public:
    // 单例，为了能在其他的地方管理rtspserver，比如通过api管理
    static RtspServer::Ptr& instance();

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    void start(const string& ip, int port, int count, bool enableSsl = false);
    void stopByPort(int port, int count);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    
    void for_each_server(const function<void(const TcpServer::Ptr &)> &cb);

private:
    int _port;
    string _ip;
    mutex _mtx;
    // int : port
    unordered_map<int, vector<TcpServer::Ptr>> _tcpServers;
};

#endif //DnsCache_h