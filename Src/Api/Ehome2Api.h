#ifndef Ehome2Api_h
#define Ehome2Api_h

#ifdef ENABLE_EHOME2

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

class Ehome2Api
{
public:
    static void initApi();

private:
    static void createVodServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void stopVodServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
};

#endif
#endif //Ehome2Api_h
