#ifndef FfmpegApi_h
#define FfmpegApi_h

#ifdef ENABLE_FFMPEG

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

class FfmpegApi
{
public:
    static void initApi();
    static void addTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void delTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void reconfig(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void startVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void startCustomVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void stopVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

    static void resetVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
};

#endif

#endif //FfmpegApi_h