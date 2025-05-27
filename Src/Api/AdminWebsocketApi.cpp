#include "AdminWebsocketApi.h"
#include "Common/ApiUtil.h"
#include "Common/ApiResponse.h"
#include "Common/ErrorCodes.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/MediaSource.h"
#include "Common/MediaClient.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

// AdminWebsocketManager 实现
AdminWebsocketManager& AdminWebsocketManager::instance()
{
    static AdminWebsocketManager s_instance;
    return s_instance;
}

void AdminWebsocketManager::addConnection(const string& connectionId, weak_ptr<WebsocketConnection> conn)
{
    lock_guard<mutex> lck(_mtx);
    _connections[connectionId] = conn;
    logInfo << "Admin WebSocket connection added: " << connectionId << ", total: " << _connections.size();

    // 新连接时立即发送当前状态
    auto connPtr = conn.lock();
    if (connPtr) {
        // 发送欢迎消息和初始数据
        json welcomeMsg;
        welcomeMsg["type"] = "welcome";
        welcomeMsg["message"] = "Connected to MediaServer Admin WebSocket";
        welcomeMsg["timestamp"] = time(nullptr);

        string welcomeData = welcomeMsg.dump();
        // TODO: 发送WebSocket消息到客户端

        // 发送初始数据
        string serverStats = generateServerStatsMessage();
        string streamList = generateStreamListMessage();
        string clientList = generateClientListMessage();

        // TODO: 发送初始数据到客户端
    }
}

void AdminWebsocketManager::removeConnection(const string& connectionId)
{
    lock_guard<mutex> lck(_mtx);
    _connections.erase(connectionId);
    logInfo << "Admin WebSocket connection removed: " << connectionId << ", total: " << _connections.size();
}

void AdminWebsocketManager::broadcastMessage(const string& message)
{
    lock_guard<mutex> lck(_mtx);

    // 清理已断开的连接
    auto it = _connections.begin();
    while (it != _connections.end()) {
        auto conn = it->second.lock();
        if (!conn) {
            it = _connections.erase(it);
        } else {
            // TODO: 发送消息到WebSocket连接
            // conn->sendWebsocketMessage(message);
            ++it;
        }
    }
}

void AdminWebsocketManager::broadcastServerStats()
{
    string message = generateServerStatsMessage();
    broadcastMessage(message);
}

void AdminWebsocketManager::broadcastStreamUpdate()
{
    string message = generateStreamListMessage();
    broadcastMessage(message);
}

void AdminWebsocketManager::broadcastClientUpdate()
{
    string message = generateClientListMessage();
    broadcastMessage(message);
}

size_t AdminWebsocketManager::getConnectionCount() const
{
    lock_guard<mutex> lck(_mtx);
    return _connections.size();
}

string AdminWebsocketManager::generateServerStatsMessage()
{
    json msg;
    msg["type"] = "server_stats";
    msg["timestamp"] = time(nullptr);

    // 获取服务器统计信息
    json data;
    auto allSources = MediaSource::getAllSource();
    auto allClients = MediaClient::getAllMediaClient();

    data["activeStreams"] = allSources.size();
    data["onlineClients"] = allClients.size();

    // 计算总带宽
    uint64_t totalBandwidth = 0;
    for (const auto& pair : allSources) {
        if (pair.second) {
            totalBandwidth += (uint64_t)(pair.second->getBitrate() * 1000); // 转换为bps
        }
    }
    data["totalBandwidth"] = totalBandwidth;

    // 运行时间（模拟数据，实际应该从Config获取启动时间）
    data["uptime"] = 3600; // 1小时，实际应该计算

    // CPU和内存使用率（模拟数据，实际应该从系统获取）
    data["cpuUsage"] = 45.2;
    data["memoryUsage"] = 62.8;

    msg["data"] = data;
    return msg.dump();
}

string AdminWebsocketManager::generateStreamListMessage()
{
    json msg;
    msg["type"] = "stream_list";
    msg["timestamp"] = time(nullptr);

    // 获取流列表
    json streams = json::array();
    auto allSources = MediaSource::getAllSource();
    for (const auto& pair : allSources) {
        if (pair.second) {
            json streamInfo;
            streamInfo["path"] = pair.second->getPath();
            streamInfo["protocol"] = pair.second->getProtocol();
            streamInfo["type"] = pair.second->getType();
            streamInfo["vhost"] = pair.second->getVhost();
            streamInfo["playerCount"] = pair.second->playerCount();
            streamInfo["bitrate"] = pair.second->getBitrate();
            streamInfo["createTime"] = pair.second->getCreateTime();
            streamInfo["status"] = pair.second->getStatus();
            streamInfo["bytes"] = pair.second->getBytes();
            streams.push_back(streamInfo);
        }
    }

    msg["data"] = streams;
    return msg.dump();
}

string AdminWebsocketManager::generateClientListMessage()
{
    json msg;
    msg["type"] = "client_list";
    msg["timestamp"] = time(nullptr);

    // 获取客户端列表
    json clients = json::array();
    auto allClients = MediaClient::getAllMediaClient();
    for (const auto& pair : allClients) {
        if (pair.second) {
            json clientInfo;
            string protocol, type;
            MediaClientType clientType;
            pair.second->getProtocolAndType(protocol, clientType);

            clientInfo["key"] = pair.first;
            clientInfo["protocol"] = protocol;
            clientInfo["type"] = (clientType == MediaClientType_Push) ? "push" : "pull";

            // 注意：MediaClient基类没有提供IP、端口等信息的接口
            // 这些信息需要从具体的客户端实现类中获取，或者通过其他方式获取
            clientInfo["ip"] = "unknown";
            clientInfo["port"] = 0;
            clientInfo["stream"] = "unknown";
            clientInfo["bitrate"] = 0;
            clientInfo["connectTime"] = 0;

            clients.push_back(clientInfo);
        }
    }

    msg["data"] = clients;
    return msg.dump();
}

// AdminWebsocketApi 实现
void AdminWebsocketApi::initApi()
{
    g_mapApi.emplace("/api/v1/admin/websocket/connect", AdminWebsocketApi::connectAdmin);
    g_mapApi.emplace("/api/v1/admin/websocket/push/server", AdminWebsocketApi::pushServerStats);
    g_mapApi.emplace("/api/v1/admin/websocket/push/streams", AdminWebsocketApi::pushStreamUpdate);
    g_mapApi.emplace("/api/v1/admin/websocket/push/clients", AdminWebsocketApi::pushClientUpdate);
}

void AdminWebsocketApi::connectAdmin(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        // 这个端点将被WebSocket连接处理，这里只是注册API路径
        // 实际的WebSocket握手和连接管理在WebsocketConnection中处理

        json data;
        data["endpoint"] = "/api/v1/admin/websocket/connect";
        data["protocol"] = "websocket";
        data["description"] = "Admin WebSocket connection endpoint";

        ApiResponse::success(rspFunc, data, "Admin WebSocket endpoint ready");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

void AdminWebsocketApi::pushServerStats(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        AdminWebsocketManager::instance().broadcastServerStats();

        json data;
        data["connections"] = AdminWebsocketManager::instance().getConnectionCount();
        data["message"] = "Server stats pushed to all connected admin clients";

        ApiResponse::success(rspFunc, data, "Server stats broadcast successful");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

void AdminWebsocketApi::pushStreamUpdate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        AdminWebsocketManager::instance().broadcastStreamUpdate();

        json data;
        data["connections"] = AdminWebsocketManager::instance().getConnectionCount();
        data["message"] = "Stream list pushed to all connected admin clients";

        ApiResponse::success(rspFunc, data, "Stream update broadcast successful");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

void AdminWebsocketApi::pushClientUpdate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        AdminWebsocketManager::instance().broadcastClientUpdate();

        json data;
        data["connections"] = AdminWebsocketManager::instance().getConnectionCount();
        data["message"] = "Client list pushed to all connected admin clients";

        ApiResponse::success(rspFunc, data, "Client update broadcast successful");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}
