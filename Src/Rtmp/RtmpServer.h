#ifndef RtmpServer_h
#define RtmpServer_h

#include "TcpServer.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class RtmpServer : public enable_shared_from_this<RtmpServer> {
public:
    using Ptr = shared_ptr<RtmpServer>;
    using Wptr = weak_ptr<RtmpServer>;

    RtmpServer();
    ~RtmpServer();

public:
    // 单例，为了能在其他的地方管理RtmpServer，比如通过api管理
    static RtmpServer::Ptr& instance();

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    void start(const string& ip, int port, int count);
    void stopByPort(int port, int count);
    void stopListenByPort(int port, int count);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    
    void for_each_server(const function<void(const TcpServer::Ptr &)> &cb);

private:
    int _port;
    string _ip;
    mutex _mtx;
    // int : port
    unordered_map<int, vector<TcpServer::Ptr>> _tcpServers;
    unordered_map<int, vector<TcpServer::Ptr>> _delServers;
};

#endif //RtmpServer_h