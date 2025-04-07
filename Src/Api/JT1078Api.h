#ifndef JT1078Api_h
#define JT1078Api_h

#ifdef ENABLE_JT1078

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

class JT1078Api
{
public:
    static void initApi();
    static void create(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void deleteCreateInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void openServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void closeServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
                        
    static void startTalk(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
                        
    static void deleteTalkInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
                        
    static void stopTalk(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
                        
    static void startSend(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
                        
    static void stopSend(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
                        
    static void getPortInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
};

#endif
#endif //JT1078Api_h