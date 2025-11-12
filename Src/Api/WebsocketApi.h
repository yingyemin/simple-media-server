#ifndef WebsocketApi_h
#define WebsocketApi_h

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

// using namespace std;  // 去掉这行

class WebsocketApi
{
public:
    static void initApi();

    static void startPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
};

#endif //WebsocketApi_h