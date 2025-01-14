#include "RtpApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Rtp/RtpServer.h"
#include "Rtp/RtpManager.h"
#include "Rtp/RtpClientPull.h"
#include "Rtp/RtpClientPush.h"
#include "ApiUtil.h"
#include "Common/Define.h"

// #include <variant>

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RtpApi::initApi()
{
    g_mapApi.emplace("/api/v1/rtp/recv/create", RtpApi::createRtpReceiver);
    g_mapApi.emplace("/api/v1/rtp/send/create", RtpApi::createRtpSender);
    g_mapApi.emplace("/api/v1/rtp/recv/stop", RtpApi::stopRtpReceiver);
    g_mapApi.emplace("/api/v1/rtp/send/stop", RtpApi::stopRtpSender);
}

void RtpApi::createRtpReceiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "socketType", "streamName", "appName"});

    int active = toInt(parser._body["active"]);
    int ssrc = toInt(parser._body["ssrc"]);
    int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
    string streamName = parser._body["streamName"];
    string appName = parser._body["appName"];

    if (active) {
        checkArgs(parser._body, {"port", "ip"});
        int port = toInt(parser._body["port"]);
        string ip = parser._body["ip"];

        auto pull = make_shared<RtpClientPull>(ip, port, appName, streamName, ssrc, socketType);
        pull->create();
        pull->start();

        value["port"] = pull->getLocalPort();
        value["ip"] = pull->getLocalIp();
        value["ssrc"] = ssrc;
    } else {
        string uri = "/" + appName + "/" + streamName;

        int newServer = toInt(parser._body.value("newServer", "0"));
        if (newServer) {
            checkArgs(parser._body, {"port"});
            int port = toInt(parser._body["port"]);

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
            value["port"] = Config::instance()->get("Rtp", "Server", gbServerName, "port");
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

        auto push = make_shared<RtpClientPush>(ip, port, appName, streamName, ssrc, socketType);
        push->setPayloadType(payloadType);
        push->setOnlyTrack(onlyTrack);
        push->create();
        push->start();

        string key = ip + "_" + to_string(port) + "_" + to_string(ssrc);
        RtpClient::addClient(key, push);

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

void RtpApi::stopRtpReceiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void RtpApi::stopRtpSender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

