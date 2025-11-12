#ifndef AdminWebsocketApi_h
#define AdminWebsocketApi_h

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"
#include "Http/WebsocketConnection.h"
#include "EventPoller/EventLoop.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <mutex>

// using namespace std;

// 管理界面WebSocket连接管理器
class AdminWebsocketManager
{
public:
    static AdminWebsocketManager& instance();
    
    // 添加/移除WebSocket连接
    void addConnection(const std::string& connectionId, std::weak_ptr<WebsocketConnection> conn);
    void removeConnection(const std::string& connectionId);
    
    // 广播消息到所有连接的管理界面
    void broadcastMessage(const std::string& message);
    void broadcastServerStats();
    void broadcastStreamUpdate();
    void broadcastClientUpdate();
    
    // 获取连接数量
    std::size_t getConnectionCount() const;
    
private:
    AdminWebsocketManager() = default;
    mutable std::mutex _mtx;
    std::unordered_map<std::string, std::weak_ptr<WebsocketConnection>> _connections;
    
    // 生成服务器统计数据
    std::string generateServerStatsMessage();
    std::string generateStreamListMessage();
    std::string generateClientListMessage();
};

class AdminWebsocketApi
{
public:
    static void initApi();
    
    // 管理界面WebSocket连接端点
    static void connectAdmin(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    
    // 手动触发数据推送的API端点
    static void pushServerStats(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void pushStreamUpdate(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void pushClientUpdate(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
};

#endif //AdminWebsocketApi_h