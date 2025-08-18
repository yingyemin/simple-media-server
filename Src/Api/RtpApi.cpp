﻿#include "RtpApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Rtp/RtpServer.h"
#include "Rtp/RtpManager.h"
#include "Rtp/RtpClientPull.h"
#include "Rtp/RtpClientPush.h"
#include "Http/ApiUtil.h"
#include "Common/Define.h"

// #include <variant>

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RtpApi::initApi()
{
    g_mapApi.emplace("/api/v1/rtp/recv/create", RtpApi::createRtpReceiver);
    g_mapApi.emplace("/api/v1/rtp/send/create", RtpApi::createRtpSender);
    g_mapApi.emplace("/api/v1/rtp/send/start", RtpApi::startRtpSender);
    g_mapApi.emplace("/api/v1/rtp/recv/stop", RtpApi::stopRtpReceiver);
    g_mapApi.emplace("/api/v1/rtp/send/stop", RtpApi::stopRtpSender);
}

void RtpApi::createRtpReceiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "streamName", "appName"});

    int active = toInt(parser._body["active"]);
    int ssrc = toInt(parser._body["ssrc"]);
    string streamName = parser._body["streamName"];
    string appName = parser._body["appName"];

    if (active) {
        checkArgs(parser._body, {"socketType", "port", "ip"});
        int port = toInt(parser._body["port"]);
        int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
        string ip = parser._body["ip"];

        static int timeout = Config::instance()->getAndListen([](const json &config){
            timeout = Config::instance()->get("GB28181", "Server", "timeout");
        }, "GB28181", "Server", "timeout", "", "5000");

        auto pull = make_shared<RtpClientPull>(appName, streamName, ssrc, socketType);
        pull->create("0.0.0.0", 0, "rtp://" + ip + ":" + to_string(port));
        pull->start("0.0.0.0", 0, "rtp://" + ip + ":" + to_string(port), timeout);

        string key = "/" + appName + "/" + streamName;
        MediaClient::addMediaClient(key, pull);

        pull->setOnClose([key](){
            MediaClient::delMediaClient(key);
        });

        value["port"] = pull->getLocalPort();
        value["ip"] = pull->getLocalIp();
        value["ssrc"] = ssrc;
    } else {
        string uri = "/" + appName + "/" + streamName;

        int newServer = toInt(parser._body.value("newServer", "0"));
        if (newServer) {
            checkArgs(parser._body, {"socketType", "port"});
            int port = toInt(parser._body["port"]);
            int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both

            if (ssrc) {
                RtpServer::instance()->startReceive("0.0.0.0", port, socketType);
                // add ssrc map streamname
                auto rtpContext = make_shared<RtpContext>(nullptr, uri, DEFAULT_VHOST, PROTOCOL_RTP, DEFAULT_TYPE);

                string payloadType = parser._body.value("payloadType", "ps");
                if (payloadType == "raw") {
                    checkArgs(parser._body, {"videoCodec", "audioCodec"});
                    string videoCodec = parser._body["videoCodec"];
                    string audioCodec = parser._body["audioCodec"];
                    
                    if (videoCodec != "none") {
                        rtpContext->createVideoTrack(videoCodec);
                    }

                    if (audioCodec != "none") {
                        int channel = toInt(parser._body.value("channel", "1"));
                        int bitPerSample = toInt(parser._body.value("bitPerSample", "8"));
                        int samplerate = toInt(parser._body.value("samplerate", "8000"));
                        rtpContext->createAudioTrack(audioCodec, channel, bitPerSample, samplerate);
                    }
                }
                rtpContext->setPayloadType(payloadType);
                RtpManager::instance()->addContext(ssrc, rtpContext);
            } else {
                RtpServer::instance()->startReceiveNoSsrc("0.0.0.0", port, socketType, parser._body);
            }
            value["port"] = port;
            value["ip"] = Config::instance()->get("LocalIp");
            value["ssrc"] = ssrc;
        } else {
            // add ssrc map streamname
            string gbServerName = parser._body.value("gbServerName", "Server1");
            value["port"] = (int)Config::instance()->get("Rtp", "Server", gbServerName, "port");
            value["ip"] = Config::instance()->get("LocalIp");
            value["ssrc"] = ssrc;
            
            auto rtpContext = make_shared<RtpContext>(nullptr, uri, DEFAULT_VHOST, PROTOCOL_RTP, DEFAULT_TYPE);

            string payloadType = parser._body.value("payloadType", "ps");
            if (payloadType == "raw") {
                checkArgs(parser._body, {"videoCodec", "audioCodec"});
                string videoCodec = parser._body["videoCodec"];
                string audioCodec = parser._body["audioCodec"];
                    
                if (videoCodec != "none") {
                    rtpContext->createVideoTrack(videoCodec);
                }

                if (audioCodec != "none") {
                    int channel = toInt(parser._body.value("channel", "1"));
                    int bitPerSample = toInt(parser._body.value("bitPerSample", "8"));
                    int samplerate = toInt(parser._body.value("samplerate", "8000"));
                    rtpContext->createAudioTrack(audioCodec, channel, bitPerSample, samplerate);
                }
            }
            rtpContext->setPayloadType(payloadType);
            RtpManager::instance()->addContext(ssrc, rtpContext);
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtpApi::stopRtpReceiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "streamName", "appName"});

    int active = toInt(parser._body["active"]);
    int ssrc = toInt(parser._body["ssrc"]);
    string streamName = parser._body["streamName"];
    string appName = parser._body["appName"];

    if (active) {
        string key = "/" + appName + "/" + streamName;
        MediaClient::delMediaClient(key);
    } else {
        string uri = "/" + appName + "/" + streamName;

        int newServer = toInt(parser._body.value("newServer", "0"));
        if (newServer) {
            checkArgs(parser._body, {"socketType", "port"});
            int port = toInt(parser._body["port"]);
            int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
            RtpServer::instance()->stopByPort(port, 0, socketType);
        } else {
            value["ssrc"] = ssrc;

            RtpManager::instance()->delContext(ssrc);
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtpApi::createRtpSender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "port", "socketType", "streamName", "appName", "ip"});

    int active = toInt(parser._body["active"]);
    // logInfo << "json active: " << parser._body["active"];
    // logInfo << "active: " << active;
    if (active) {
        int ssrc = toInt(parser._body["ssrc"]);
        int port = toInt(parser._body["port"]);
        int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
        string streamName = parser._body["streamName"];
        string appName = parser._body["appName"];
        string ip = parser._body["ip"];
        string payloadType = parser._body.value("payloadType", "ps");
        string onlyTrack = parser._body.value("onlyTrack", "all");

        // static int timeout = Config::instance()->getAndListen([](const json &config){
        //     timeout = Config::instance()->get("GB28181", "Server", "timeout");
        // }, "GB28181", "Server", "timeout", "", "5000");

        auto push = make_shared<RtpClientPush>(appName, streamName, ssrc, socketType);
        push->create("0.0.0.0", 0, "rtp://" + ip + ":" + to_string(port));
        // push->start("0.0.0.0", 0, "rtp://" + ip + ":" + to_string(port), timeout);

        string key = ip + "_" + to_string(port) + "_" + to_string(ssrc);
        MediaClient::addMediaClient(key, push);

        push->setOnClose([key](){
            MediaClient::delMediaClient(key);
        });

        value["port"] = push->getLocalPort();
        value["ip"] = push->getLocalIp();
        value["ssrc"] = ssrc;
    } else {
        int ssrc = toInt(parser._body["ssrc"]);
        int port = toInt(parser._body["port"]);
        int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
        string streamName = parser._body["streamName"];
        string appName = parser._body["appName"];

        RtpServer::instance()->startSend("0.0.0.0", port, socketType, appName, streamName, ssrc);

        value["port"] = port;
        value["ip"] = Config::instance()->get("LocalIp");
        value["ssrc"] = ssrc;
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtpApi::startRtpSender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";

    checkArgs(parser._body, {"ssrc", "port",  "ip"});

    
    int ssrc = toInt(parser._body["ssrc"]);
    int port = toInt(parser._body["port"]);
    string ip = parser._body["ip"];

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("GB28181", "Server", "timeout");
    }, "GB28181", "Server", "timeout", "", "5000");

    string key = ip + "_" + to_string(port) + "_" + to_string(ssrc);
    auto push = MediaClient::getMediaClient(key);

    if (push) {
        push->start("0.0.0.0", 0, "rtp://" + ip + ":" + to_string(port), timeout);
    } else {
        value["code"] = "404";
        value["msg"] = "rtp push client is not exist";
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtpApi::stopRtpSender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "port", "socketType", "streamName", "appName", "ip"});

    int active = toInt(parser._body["active"]);
    // logInfo << "json active: " << parser._body["active"];
    // logInfo << "active: " << active;
    if (active) {
        int ssrc = toInt(parser._body["ssrc"]);
        int port = toInt(parser._body["port"]);
        string ip = parser._body["ip"];

        string key = ip + "_" + to_string(port) + "_" + to_string(ssrc);
        MediaClient::delMediaClient(key);
    } else {
        int port = toInt(parser._body["port"]);
        int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both

        RtpServer::instance()->stopSendByPort(port, socketType);
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

