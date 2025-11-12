#include "DownDeviceManagerApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "ManagerServer/Common/ApiUtil.h"
#include "Manager/DeviceManager.h"
#include "Manager/StreamManager.h"
#include "Manager/SMSServer.h"
#include "Manager/SipServer.h"
#include "Http/HttpUtil.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void DownDeviceManagerApi::initApi()
{
    g_mapApi.emplace("/api/v1/down/device/play/start", DownDeviceManagerApi::Play);
    g_mapApi.emplace("/api/v1/down/device/play/stop", DownDeviceManagerApi::Stop);
    g_mapApi.emplace("/api/v1/down/device/list", DownDeviceManagerApi::DeviceList);
    g_mapApi.emplace("/api/v1/down/device/info", DownDeviceManagerApi::DeviceInfo);
    g_mapApi.emplace("/api/v1/down/device/ptz", DownDeviceManagerApi::DevicePtz);
    g_mapApi.emplace("/api/v1/down/device/sync", DownDeviceManagerApi::DeviceSync);
    g_mapApi.emplace("/api/v1/down/device/syncStatus", DownDeviceManagerApi::DeviceSyncStatus);

    g_mapApi.emplace("/api/v1/down/device/channel/list", DownDeviceManagerApi::ChannelList);
    g_mapApi.emplace("/api/v1/down/device/channel/snap", DownDeviceManagerApi::ChannelSnap);

    g_mapApi.emplace("/api/v1/down/device/play/createSipClient", DownDeviceManagerApi::PlaySipClient);
    g_mapApi.emplace("/api/v1/down/device/play/startSipClient", DownDeviceManagerApi::startSipClient);
    // 透传
    g_mapApi.emplace("/api/v1/down/device/transparent/control", DownDeviceManagerApi::MessageTransparent);

    g_mapApi.emplace("/api/v1/down/device/onDeviceStatus", DownDeviceManagerApi::onDeviceStatus);
    g_mapApi.emplace("/api/v1/down/device/onDeviceChannel", DownDeviceManagerApi::onDeviceChannel);
    g_mapApi.emplace("/api/v1/down/device/onDeviceInfo", DownDeviceManagerApi::onDeviceInfo);
    g_mapApi.emplace("/api/v1/down/device/onKeepAlive", DownDeviceManagerApi::onKeepAlive);
}

void DownDeviceManagerApi::Stop(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id", "channel_id" });
    std::string str_content;
    std::string device_id = parser._body["device_id"];
    std::string channel_id = parser._body["channel_id"];

    // TODO 调用信令的api接口

    HttpResponse rsp;
    rsp._status = 200;
    rsp.setContent(str_content);
    rspFunc(rsp);
}
template<typename Type>
std::string DownDeviceManagerApi::_mk_response(int status, Type t, std::string msg)
{
    return nlohmann::json
    {
        {"code",status},
        {"msg",msg},
        {"data",t}
    }.dump(4);
}

void DownDeviceManagerApi::PlaySipClient(const HttpParser& parser, const UrlParser& urlParser,
    const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id", "channel_id", "protocol_type" ,"target_ip","target_port","ssrc"});
    std::string str_content;
    auto device_id = parser._body["device_id"];
    auto channel_id = parser._body["channel_id"];
    int protocol_type = toInt(parser._body["protocol_type"]);
    auto target_ip = parser._body["target_ip"];
    int port = toInt(parser._body["target_port"]);
    auto _ssrc = parser._body["ssrc"];

    // 调用级联信令的接口
}

void DownDeviceManagerApi::startSipClient(const HttpParser& parser, const UrlParser& urlParser,
    const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id", "channel_id"});
    string device_id = parser._body["device_id"];
    string channel_id = parser._body["channel_id"];
    string target_ip = parser._body["target_ip"];
    int target_port = toInt(parser._body["target_port"]);
    string ssrc = parser._body["ssrc"];
    string content;

    std::string stream_id = device_id + "_" + channel_id;

    do {
        auto device = DeviceManager::instance()->GetDevice(device_id);
        if (device == nullptr)
        {
            content = _mk_response(1, "", "device not found");
            break;
        }

        if (!device->IsRegistered())
        {
            content = _mk_response(1, "", "device not online");
            break;
        }

        auto channel = device->GetChannel(channel_id);
        if (channel == nullptr)
        {
            content = _mk_response(1, "", "channel not found");
            break;
        }

        auto stream = StreamManager::instance()->GetStream(stream_id);
        if (stream)
        {
            auto session = std::dynamic_pointer_cast<CallSession>(stream);
            if (session->IsConnected())
            {
                auto session = std::dynamic_pointer_cast<CallSession>(stream);
                auto server = SMSServer::instance()->getServer(session->getServerId());
                if (!server) {
                    content = _mk_response(1, "", "media server not found");
                    break;
                }

                server->StartRtpSender(ssrc, target_ip, target_port, [session, rspFunc, server](){
                    auto serverInfo = server->getInfo();
                    json value;
                    value["code"] = 0;
                    value["data"]["app"] = session->GetApp();
                    value["data"]["stream"] = session->GetStreamID();
                    value["data"]["ip"] = serverInfo->IP;

                    HttpResponse rsp;
                    rsp._status = 200;
                    rsp.setContent(value.dump());
                    rspFunc(rsp);
                });
                
                return ;
            } 
        }
        
    } while (0);

    HttpResponse rsp;
    rsp._status = 200;

    rsp.setContent(content);
    rspFunc(rsp);
}

void DownDeviceManagerApi::Play(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id", "channel_id", "protocol_type" });
    
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    std::string device_id = parser._body["device_id"];
    std::string channel_id = parser._body["channel_id"];
    int protocol_type = toInt(parser._body["protocol_type"]);

    // 调用信令的接口
    do {
        auto device = DeviceManager::instance()->GetDevice(device_id);
        if (!device) {
            value["code"] = 404;
            value["msg"] = "device is not exist";
            break;
        }
        std::string signalServerId = device->GetSignalServerId();
        auto sipServer = SipServer::instance()->getServer(signalServerId);
        if (!sipServer) {
            logInfo << "sipServer is not exist, sipServerId: " << signalServerId;;
            value["code"] = 404;
            value["msg"] = "sip server is not exist";
            break;
        }

        auto mediaServer = SMSServer::instance()->getServerByCircly();

        // TODO 优化ssrc生成逻辑，用一个类专门管理
        // 是否可以设备和ssrc一对一绑定，但是这样回放不好处理
        // 实时流一对一绑定，回放循环递增？
        // 用port manager那一套的话，初始化的数组太大了
        static int ssrc = 1000;
        std::string strSsrc = to_string(++ssrc);
        mediaServer->OpenRtpServer(strSsrc, device_id + "_" + channel_id, 3, 0, [strSsrc, device_id, channel_id, mediaServer, sipServer](int port){
            std::string mediaServerIp = mediaServer->getIP();
            // int mediaServerPort = mediaServer->getPort();
            sipServer->invite(device_id, channel_id, 0, mediaServerIp, port, strSsrc);
        });
        string streanId = device_id + "_" + channel_id;
        auto serverInfo = mediaServer->getInfo();
        value["data"]["app"] = "live";
        value["data"]["stream"] = streanId;
        value["data"]["ip"] = mediaServer->getIP();
        value["data"]["flv"] = "http://" + mediaServer->getIP() + ":" + to_string(serverInfo->HttpServerPort) + "/" + "live" + "/" + streanId + ".flv";
        value["data"]["ws_flv"] = "ws://" + mediaServer->getIP() + ":" + to_string(serverInfo->HttpServerPort) + "/" + "live" + "/" + streanId + ".flv";
        value["data"]["rtc"] = "http://" + mediaServer->getIP() + ":" + to_string(serverInfo->Port) + SMSServer::instance()->getWebrtcPlayApi() + "?appName=" + "live" + "&streamName=" + streanId;
        value["data"]["rtmp"] = "rtmp://" + mediaServer->getIP() + ":" + to_string(serverInfo->RtmpPort) + "/" + "live" + "/" + streanId;
        value["data"]["rtsp"] = "rtsp://" + mediaServer->getIP() + ":" + to_string(serverInfo->RtspPort) + "/" + "live" + "/" + streanId;
    } while (0);

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::DeviceList(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    // 调用信令的接口
    auto deviceList = DeviceManager::instance()->GetDeviceList();
    value["data"]["total"] = deviceList.size();
    for (auto device : deviceList) {
        auto deviceCtx = DeviceManager::instance()->GetDevice(device->GetDeviceID());
        json item;
        item["id"] = 1; //数据库key
        item["deviceId"] = device->GetDeviceID();
        item["name"] = device->GetName();
        item["manufacturer"] = device->GetManufacturer() == "" ? "未填写" : device->GetManufacturer();
        item["model"] = device->GetModel();
        item["firmware"] = "";
        item["transport"] = device->GetTransport();
        item["streamMode"] = "TCP-PASSIVE"; //数据流传输模式
        item["ip"] = device->GetIP();
        item["port"] = device->GetPort();
        item["hostAddress"] = "";
        item["channelCount"] = device->GetChannelCount();
        if (device->IsRegistered()) {
            item["onLine"] = device->IsRegistered();
            item["registerTime"] = to_string(device->GetRegistTime());
            item["updateTime"] = to_string(device->GetLastTime());
            item["keepaliveTime"] = to_string(device->GetLastTime());
        } else {
            item["onLine"] = false;
        }

        value["data"]["list"].push_back(item);
    }

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
void DownDeviceManagerApi::MessageTransparent(const HttpParser& parser, const UrlParser& urlParser,
    const function<void(HttpResponse& rsp)>& rspFunc)
{
    string res_content;
    auto device_id = parser._body["device_id"];
    auto req_xml = parser._body["xml"];


    HttpResponse rsp;
    rsp._status = 200;
    rsp.setContent(res_content);
    rspFunc(rsp);
}
void DownDeviceManagerApi::DeviceInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{


    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    string deviceId = parser._body["device_id"];
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);
    if (!device) {
        value["code"] = 404;
        value["msg"] = "device is not exist";
    } else {
        json item;
        item["id"] = 1; //数据库key
        item["deviceId"] = device->GetDeviceID();
        item["name"] = device->GetName();
        item["manufacturer"] = device->GetManufacturer() == "" ? "未填写" : device->GetManufacturer();
        item["model"] = device->GetModel();
        item["firmware"] = "固件版本";
        item["transport"] = device->GetTransport();
        item["streamMode"] = "TCP-PASSIVE"; //数据流传输模式
        item["ip"] = device->GetIP();
        item["port"] = device->GetPort();
        item["hostAddress"] = "wan地址";
        item["onLine"] = device->IsRegistered();
        item["channelCount"] = device->GetChannelCount();
        item["registerTime"] = device->GetRegistTime();
        item["updateTime"] = device->GetLastTime();
        item["keepaliveTime"] = device->GetLastTime();
        // item["sipTransactionInfo"]["callId"] = device->exosip_context->j_calls->c_id;
        // item["sipTransactionInfo"]["fromTag"] = device->GetLastTime();
        // item["sipTransactionInfo"]["toTag"] = device->GetLastTime();
        // item["sipTransactionInfo"]["viaBranch"] = device->GetLastTime();

        value["data"] = item;
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::DeviceSync(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    // TODO 判断是否有catalog任务在执行
    // 如果有，且出错了，返回错误原因
    // 如果有，且没有错，则返回进度
    // 如果没有，则发起任务

    string deviceId = parser._body["device_id"];
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::DeviceSyncStatus(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    // TODO 判断是否有catalog任务在执行
    // 如果有，且出错了，返回错误原因
    // 如果有，且没有错，则返回进度
    // 如果没有，则发起任务

    string deviceId = parser._body["device_id"];
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);

    if (!device) {
        value["code"] = 404;
        value["msg"] = "device is not exist";
    } else {
        // 先写死，后面需要根据实际的数值返回
        value["data"]["total"] = 1;
        value["data"]["current"] = 1;
        value["data"]["syncIng"] = 0;
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::ChannelList(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    string deviceId = parser._body["device_id"];
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);

    if (!device) {
        value["code"] = 404;
        value["msg"] = "device is not exist";
    } else {
        auto channels = device->GetAllChannels();
        value["data"]["total"] = channels.size();
        for (auto channel : channels) {
            auto channelCtx = device->GetChannel(channel->GetChannelID());
            json item;
            item["id"] = 1; //数据库key
            item["channelId"] = channel->GetChannelID();
            item["deviceId"] = device->GetDeviceID();
            item["streamId"] = device->GetDeviceID() + "_" + channel->GetChannelID();
            item["name"] = channel->GetName();
            item["manufacturer"] = channel->GetManufacturer() == "" ? "未填写" : channel->GetManufacturer();
            item["model"] = channel->GetModel();
            item["owner"] = channel->GetOwner();
            item["civilCode"] = channel->GetCivilCode();
            item["Address"] = channel->GetAddress();
            item["Parental"] = channel->GetParental();
            item["parentId"] = channel->GetParentID();
            item["registerWay"] = channel->GetRegisterWay();
            item["secrecy"] = channel->GetSecrety();
            item["ipAddress"] = channel->GetIpAddress();
            item["downloadSpeed"] = 1;
            item["subCount"] = channel->GetAllSubChannels().size();
            
            if (channelCtx) {
                item["status"] = channelCtx->GetStatus() == "ON" ? 1 : 0;
                item["streamIdentification"] = channelCtx->GetStreamNum();
            } else {
                item["status"] = 1;
                item["streamIdentification"] = 0;
            }
            
            value["data"]["list"].push_back(item);
        }
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::DevicePtz(const HttpParser& parser, const UrlParser& urlParser,
    const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id", "channel_id", "command", "speed" });
    auto device_id = parser._body["device_id"];
    auto channel_id = parser._body["channel_id"];
    auto command = parser._body["command"];
    auto speed = toInt(parser._body["speed"]);
    
    // 调用信令的接口

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::OpenSMSSender(const std::string& target_ip, const std::string& _stream_id,const std::string& _ssrc,
    int port, int pro_type, const std::function<void(int port)>& cb)
{

    int rtp_port = -1;
    // //1:tcp,2:udp,3:both
    auto server = SMSServer::instance()->getServerByCircly();
    if (!server) {
        return;
    }

    server->OpenRtpSender(_ssrc, target_ip, port, pro_type, 1, _stream_id, cb);
}

void DownDeviceManagerApi::ChannelSnap(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "device_id", "channel_id" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    // TODO 判断是否有catalog任务在执行
    // 如果有，且出错了，返回错误原因
    // 如果有，且没有错，则返回进度
    // 如果没有，则发起任务

    string deviceId = parser._body["device_id"];
    string channelId = parser._body["channel_id"];
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);

    do {
        if (!device) {
            value["code"] = 404;
            value["msg"] = "device is not exist";
            break;
        }
            
        auto channel = device->GetChannel(channelId);
        if (!channel) {
            value["code"] = 404;
            value["msg"] = "channel is not exist";
            break;
        }

        auto buffer = StreamBuffer::create();
        buffer->setCapacity(1024*1024);
        FILE* fp = fopen("testcover.png", "rb+");
        if (!fp) {
            value["code"] = 404;
            value["msg"] = "file is not exist";
            break;
        }
        int size = fread(buffer->data(), 1, 1024*1024, fp);
        fclose(fp);
        buffer->substr(0, size);

        rsp.setContent(string(buffer->data(), buffer->size()), HttpUtil::getMimeType("testcover.png"));
        rspFunc(rsp);
    } while (0);

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::onDeviceStatus(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "deviceId", "status", "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    string deviceId = parser._body["deviceId"];
    string serverId = parser._body["serverId"];
    string status = parser._body["status"];
    
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);

    if (status == "on") {
        if (device) {
            device->SetRegistered(true);
            device->SetDeviceID(deviceId);
            device->SetSignalServerId(serverId);
            DeviceManager::instance()->AddDevice(device);
        } else {
            device = std::make_shared<Device>();
            device->SetRegistered(true);
            device->SetDeviceID(deviceId);
            device->SetSignalServerId(serverId);
            DeviceManager::instance()->AddDevice(device);
        }
    } else if (status == "off") { 
        if (device) {
            device->SetRegistered(false);
        }
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::onKeepAlive(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "deviceId", "timestamp", "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    string deviceId = parser._body["deviceId"];
    uint64_t timestamp = parser._body["timestamp"];
    
    DeviceManager::instance()->UpdateDeviceLastTime(deviceId, timestamp);

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::onDeviceInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "deviceId", "serverId", "name", "channelCount", "manufacturer" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    string deviceId = parser._body["deviceId"];
    
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);
    if (device)
    {
        device->SetName(parser._body["name"]);
        device->SetChannelCount(parser._body["channelCount"]);
        device->SetManufacturer(parser._body["manufacturer"]);
        DeviceManager::instance()->AddDevice(device);
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownDeviceManagerApi::onDeviceChannel(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "deviceId", "serverId", "channelId", "paraentId", "name" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 0;
    value["msg"] = "success";

    string deviceId = parser._body["deviceId"];
    // json channles = parser._body["channles"];
    
    Device::Ptr device = DeviceManager::instance()->GetDevice(deviceId);
    if (device)
    {
        // for (auto &channel : channles) {
        //     checkArgs(channel, { "channelId", "name", "paraentId" });
            string channelId = parser._body["channelId"];
            auto channelCtx = device->GetChannel(channelId);
            if (channelCtx) {
                channelCtx->SetName(parser._body["name"]);
                channelCtx->SetParentID(parser._body["paraentId"]);
            } else {
                channelCtx = make_shared<Channel>();
                channelCtx->SetChannelID(channelId);
                channelCtx->SetName(parser._body["name"]);
                channelCtx->SetParentID(parser._body["paraentId"]);
                device->InsertChannel(parser._body["paraentId"], channelId, channelCtx);
            }
        // }
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}