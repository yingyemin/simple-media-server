#ifndef JT808Server_h
#define JT808Server_h

#include "TcpServer.h"
#include "Common/PortManager.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

// using namespace std;

class JT808Server : public std::enable_shared_from_this<JT808Server> {
public:
    using Ptr = std::shared_ptr<JT808Server>;
    using Wptr = std::weak_ptr<JT808Server>;

    JT808Server();
    ~JT808Server();

public:
    // 单例，为了能在其他的地方管理HttpServer，比如通过api管理
    static JT808Server::Ptr& instance();
    static JT808Server::Ptr& instance(const std::string& key);

    // 可多次调用，为了可以动态的增减端口或者线程数
    // 比如想动态换一个监听端口，或者动态加一个监听端口
    void start(const std::string& ip, int port, int count);
    void stopByPort(int port, int count);

    // 后面考虑增加IP参数
    // void stopByIp(int port, int count);
    void setServerId(const std::string& key);
    
    void for_each_server(const std::function<void(const TcpServer::Ptr &)> &cb);

private:
    int _port;
    std::string _ip;
    std::string _serverId;
    // PortManager _portManager;

    std::mutex _mtx;
    // int : port
    std::unordered_map<int, std::vector<TcpServer::Ptr>> _tcpServers;
};

#endif //JT808Server_h
