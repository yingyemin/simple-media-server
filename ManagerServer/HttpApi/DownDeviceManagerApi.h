#ifndef DOWN_DEVICE_MANAGER_API_H_
#define DOWN_DEVICE_MANAGER_API_H_

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

// using namespace std;


class DownDeviceManagerApi
{
public:
    static void initApi();

private:
    static void StreamStatus(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void Stop(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void Play(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc); 

    static void PlaySipClient(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void startSipClient(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void DeviceList(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void DeviceInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void DevicePtz(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void DeviceSync(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void DeviceSyncStatus(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void ChannelList(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void ChannelSnap(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void onDeviceStatus(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void onDeviceChannel(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void onDeviceInfo(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void onKeepAlive(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);

    // 增加xml 透传接口
    static void MessageTransparent(const HttpParser& parser, const UrlParser& urlParser,
        const std::function<void(HttpResponse& rsp)>& rspFunc);
private:
    template<typename Type>
    static std::string _mk_response(int status, Type t, std::string msg);
    static void private_play(const std::string& device_id, const std::string& channel_id,int pro_type, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void OpenSMSSender(const std::string& target_ip, const std::string& _stream_id, const std::string& _ssrc,
        int port, int pro_type, const std::function<void(int port)>& cb);

    static void private_play_sipclient(const std::string& device_id, const std::string& channel_id, int pro_type,
        const std::string& target_ip,int target_port, const std::string& _ssrc, const std::function<void(HttpResponse& rsp)>& rspFunc);
};

#endif //HookApi_h