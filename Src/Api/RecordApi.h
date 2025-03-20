#ifndef RecordApi_h
#define RecordApi_h

#ifdef ENABLE_RECORD

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

class RecordApi
{
public:
    static void initApi();
    static void startRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void stopRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
};

#endif
#endif //RecordApi_h