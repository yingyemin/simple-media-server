#include <string>

#include "MediaHook.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Http/HttpClientApi.h"
#include "Util/String.h"

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
    
    shared_ptr<HttpClientApi> client;
    if (startWith(url, "https://")) {
        client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop(), true);
    } else {
        client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
    }
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
    if (client->sendHeader(url, timeout) != 0) {
        cb("connect to url: " + url + " failed", nullptr);
    }
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

// 使用类作为参数，好处是以后修改参数没那么麻烦，坏处是有地方漏了，编译的时候也发现不了
// 本项目使用类主要是图方便，谨慎用之
void MediaHook::onPublish(const PublishInfo& info, const function<void(const PublishResponse& rsp)>& cb)
{
    json value;
    value["protocol"] = info.protocol;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;
    value["params"] = info.params;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onPublish");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onPublish");

        reportByHttp(url, "GET", value.dump(), [cb](const string& err, const nlohmann::json& res){
            PublishResponse rsp;
            if (!err.empty()) {
                rsp.authResult = false;
                rsp.err = err;
                cb(rsp);

                return ;
            }

            if (res.find("authResult") == res.end()) {
                rsp.authResult = false;
                rsp.err = "authResult is empty";
                cb(rsp);

                return ;
            }
            
            rsp.authResult = res["authResult"];
            cb(rsp);
        });
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