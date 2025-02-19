#ifndef SrtApi_h
#define SrtApi_h

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

#ifdef ENABLE_SRT

class SrtApi
{
public:
    static void initApi();
    static void createPullClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void stopPullClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void listPullClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void createPushClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void stopPushClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    static void listPushClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
};

#endif

#endif