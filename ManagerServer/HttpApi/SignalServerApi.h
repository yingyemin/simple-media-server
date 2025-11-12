#ifndef Signal_Server_API_H_
#define Signal_Server_API_H_

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

class SignalServerApi
{
public:
    static void initApi();

private:
    static void onRegisterServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void onKeepAlive(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void list(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void query(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

    static void get(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
};

#endif //Signal_Server_API_H_