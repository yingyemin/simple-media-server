#include <string>

#include "MediaHook.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Http/HttpClientApi.h"

using namespace std;

MediaHook::Ptr MediaHook::instance() 
{ 
    static MediaHook::Ptr hook(new MediaHook()); 
    return hook; 
}

void MediaHook::init()
{
    weak_ptr<MediaHook> wSelf = shared_from_this();
    _type = Config::instance()->getAndListen([wSelf](const json& config){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->_type = Config::instance()->get("Hook", "Type");
        logInfo << "Hook type: " << self->_type;
    }, "Hook", "Type");
}

void MediaHook::reportByHttp(const string& url, const string&method, const string& msg, const function<void(const string& err, const json& res)>& cb)
{
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("Hook", "Http", "timeout");
    }, "Hook", "Http", "timeout");
    
    shared_ptr<HttpClientApi> client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
    client->addHeader("Content-Type", "application/json;charset=UTF-8");
    client->setMethod(method);
    client->setContent(msg);
    client->setOnHttpResponce([client, cb](const HttpParser &parser){
        // logInfo << "uri: " << parser._url;
        // logInfo << "status: " << parser._version;
        // logInfo << "method: " << parser._method;
        // logInfo << "_content: " << parser._content;
        if (parser._url != "200") {
            cb("http error", "");
        }
        try {
            json value = json::parse(parser._content);
            cb("", value);
        } catch (exception& ex) {
            logInfo << "json parse failed: " << ex.what();
            cb(ex.what(), nullptr);
        }

        const_cast<shared_ptr<HttpClientApi> &>(client).reset();
    });
    logInfo << "connect to utl: " << url;
    client->sendHeader(url, timeout);
}

void MediaHook::onStreamStatus(const StreamStatusInfo& info)
{
    json value;
    value["protocol"] = info.protocol;
    value["errorCode"] = info.errorCode;
    value["status"] = info.status;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onStreamStatus");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onStreamStatus");

        reportByHttp(url, "GET", value.dump());
    }
}

void MediaHook::onNonePlayer(const string& protocol, const string& uri, 
            const string& vhost, const string& type)
{
    json value;
    value["protocol"] = protocol;
    value["type"] = type;
    value["uri"] = uri;
    value["vhost"] = vhost;

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onNonePlayer");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onNonePlayer");

        reportByHttp(url, "GET", value.dump());
    }
}