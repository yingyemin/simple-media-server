#ifndef RtspApi_h
#define RtspApi_h

#ifdef ENABLE_RTSP

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

// using namespace std;  // 去掉这行

class RtspApi
{
public:
    static void initApi();
    static void createRtspStream(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void startRtspPlay(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void stopRtspPlay(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void listRtspPlayInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void startRtspPublish(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void stopRtspPublish(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void listRtspPublishInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void createRtspServer(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void stopRtspServer(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    static void listRtspServers(const HttpParser& parser, const UrlParser& urlParser,
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
};

#endif
#endif //RtSpApi_h