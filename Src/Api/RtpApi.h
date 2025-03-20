#ifndef RtpApi_h
#define RtpApi_h

#ifdef ENABLE_RTP

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

class RtpApi
{
public:
    static void initApi();
    static void createRtpReceiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void createRtpSender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void stopRtpReceiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void stopRtpSender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
};

#endif
#endif //RtpApi_h