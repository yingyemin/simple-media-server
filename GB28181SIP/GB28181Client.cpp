/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Lixin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "GB28181Client.h"
#include "Log/Logger.h"
#include "Util/MD5.h"
#include "tinyxml.h"
#include "Common/Config.h"
#include "Common/Track.h"
#include "Util/String.h"
#include "Rtsp/RtspSdpParser.h"
#include "Http/HttpClientApi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>  
#include <map>
#include <unistd.h>

using namespace std;
using namespace tinyxml2;

GB28181Client::GB28181Client()
{
    _req.reset(new SipRequest());
    _req->peer_ip = Config::instance()->get("Sip", "serverIp");
    _req->peer_port = Config::instance()->get("Sip", "serverPort");
    _req->serial = Config::instance()->get("Sip", "serverId");
    _req->realm = Config::instance()->get("Sip", "serverRealm");
    _req->host = Config::instance()->get("Sip", "localIp");
    _req->host_port = Config::instance()->get("Sip", "localPort");
    _req->sip_auth_id = Config::instance()->get("Sip", "localId");
    _req->sip_auth_pwd = Config::instance()->get("Sip", "localPassword");
    _channelNum = Config::instance()->get("Sip", "channelNum");
    _channelStartId = Config::instance()->get("Sip", "channelStart");
    //_device.reset(new DevChannel("__defaultVhost__", "live", "test", 0, false, true, false, false));
}

GB28181Client::~GB28181Client()
{}

void GB28181Client::gbRegister(shared_ptr<SipRequest> req)
{
    if (!req) {
        req = _req;
    }
    std::stringstream ss;
    _sipStack.req_register(ss, req);

    sendMessage(ss.str());
}

void GB28181Client::keepalive()
{
    std::stringstream ss;
    _sipStack.req_keepalive(ss, _req);

    sendMessage(ss.str());
}

string GB28181Client::sendDevice(const string& callId, CatalogInfo& info)
{
    logTrace << "_channelNum is: " << info._channelNum << endl;
    if (info._channelNum <= 0) {
        logTrace << "_callid2Catalog size is: " << _callid2Catalog.size() << endl;
        return "";
    }
    int num = 0;
    logTrace << "_channelStartId: " << info._channelStartId << endl;
    logTrace << "_channelEndId: " << info._channelEndId << endl;
    int64_t startId = stol(info._channelEndId);
    if (info._channelNum > 5) {
        num = 5;
        info._channelNum -= 5;
        info._channelEndId = to_string(startId + 5);
    } else {
        num = info._channelNum;
        info._channelNum = 0;
    }

    std::stringstream ss;
    auto resCallId = _sipStack.resp_catalog(ss, _req, info._sn, info._total, num, startId, info._channelStartId);
    sendMessage(ss.str());

    return resCallId;
}

void GB28181Client::catalog(shared_ptr<SipRequest> req)
{
    // SipRequest req;
    // req.sip_auth_id = "34020000001320000003";
    // req.host = "192.168.30.173";
    // req.host_port = _localPort;

    // req.serial = "34021000002000000001";
    // req.realm = "3402100000";

    stringstream ss;
    _sipStack.resp_status(ss, req);
    sendMessage(ss.str());

    XMLDocument doc;
    if (doc.Parse(req->content.data()) != XML_SUCCESS)
    {
        logError << "parse xml error" << endl;
        return ;
    }

    XMLElement* root=doc.RootElement();
    //XMLElement* Query=root->FirstChildElement("Query");

    CatalogInfo info;
    info._sn = root->FirstChildElement("SN")->GetText();
    info._channelNum = _channelNum;
    info._total = _channelNum;
    info._channelStartId = _channelStartId;
    info._channelEndId = _channelEndId;

    auto callId = sendDevice("", info);
    _callid2Catalog[callId] = info;

    //int total = _channelNum;
    // _catalog2Timer = std::make_shared<Timer>(60.0 / 1000, [this, total, sn]() {
    //     if (_channelNum <= 0) {
    //         _channelNum = mINI::Instance()["sip.channelNum"];
    //         _channelEndId = "1310000001";
    //         return false;
    //     }
    //     sendDevice(total, sn);
    //     return true;
    // }, nullptr);
}

void GB28181Client::sendDeviceInfo(shared_ptr<SipRequest> req)
{
    stringstream ss;
    _sipStack.resp_status(ss, req);
    sendMessage(ss.str());

    XMLDocument doc;
    if (doc.Parse(req->content.data()) != XML_SUCCESS)
    {
        logError << "parse xml error" << endl;
        return ;
    }

    XMLElement* root=doc.RootElement();
    //XMLElement* Query=root->FirstChildElement("Query");

    string sn = root->FirstChildElement("SN")->GetText();

    std::stringstream ss1;
    _sipStack.resp_deviceinfo(ss1, _req, sn, _channelNum);
    sendMessage(ss1.str());
}

void GB28181Client::sendDeviceStatus(shared_ptr<SipRequest> req)
{
    stringstream ss;
    _sipStack.resp_status(ss, req);
    sendMessage(ss.str());

    XMLDocument doc;
    if (doc.Parse(req->content.data()) != XML_SUCCESS)
    {
        logError << "parse xml error" << endl;
        return ;
    }

    XMLElement* root=doc.RootElement();
    //XMLElement* Query=root->FirstChildElement("Query");

    string sn = root->FirstChildElement("SN")->GetText();

    std::stringstream ss1;
    _sipStack.resp_devicestatus(ss1, _req, sn);
    sendMessage(ss1.str());
}

void GB28181Client::onWholeSipPacket(shared_ptr<SipRequest> req)
{
    weak_ptr<GB28181Client> wSelf = shared_from_this();
    std::string session_id = req->sip_auth_id;

    if (req->is_register()) {
        logInfo << "get a register response, status is: " << req->status << endl;
        if (req->status == "200") {
            logInfo << "register success" << endl;
            _registerStatus = 1;
            _loop->addTimerTask(5000, [wSelf](){
                auto self = wSelf.lock();
                if (!self) {
                    return 0;
                }
                if (self->_aliveStatus != 0) {
                    self->_aliveStatus = 0;
                    self->gbRegister(self->_req);
                    return 0;
                }
                self->_aliveStatus = 1;
                self->keepalive();
                return 5000;
            }, nullptr);
        } else if (req->status == "401") {
            logInfo << "www_authenticate is: " << req->www_authenticate << endl;
            auto space = split(req->www_authenticate, ",");
            auto kongge = split(space[0], " ");
            string secretType = kongge[0];
            logTrace << "secretType is: " << secretType << endl;

            space[0] = kongge[1];
            //auto douhao = string_split(space[1], ",");
            unordered_map<string, string> auths;
            for (auto& tmp : space) {
                auto auth = split(tmp, "=");
                string key = trim(auth[0], " ");
                auths[key] = auth[1];
            }
            string realm = trim(auths["realm"], "\"");
            string nonce = trim(auths["nonce"], "\"");
            string username = _req->sip_auth_id;
            string password = _req->sip_auth_pwd;
            string uri = _req->serial + "@" + _req->realm;

            stringstream authenticate;
            if (auths.find("qop") != auths.end()) {
                string qop = trim(auths["qop"], "\"");
                string nc = "00000001";
                string cnonce = "0a4f113b";

                string ha1 = MD5(username + ":" + realm + ":" + password).hexdigest();
                logInfo << "ha1 is: " << ha1 << endl;
                string ha2 = MD5("REGISTER:" + uri).hexdigest();
                logInfo << "ha2 is: " << ha2 << endl;
                string strmd5 = ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2;
                logInfo << "strmd5 is: " << strmd5 << endl;
                string response = MD5(strmd5).hexdigest();
                authenticate << "Digest username=" << username 
                            << ",realm=" << realm 
                            << ",nonce=" << nonce 
                            << ",uri=" << uri 
                            << ",response=" << response
                            << ",algorithm=MD5"
                            << ",qop=" << qop
                            << ",nc=" << nc
                            << ",cnonce=" << cnonce;
            } else {
                // string ha1 = MD5(username + ":" + realm + ":" + password).hexdigest();
                // string ha2 = MD5("REGISTER:" + uri).hexdigest();
                string ha1 = MD5(username + ":" + realm + ":" + password).hexdigest();
                logInfo << "ha1 is: " << ha1 << endl;
                string ha2 = MD5("REGISTER:sip:" + uri).hexdigest();
                logInfo << "ha2 is: " << ha2 << endl;
                string strmd5 = ha1 + ":" + nonce + ":" + ha2;
                logInfo << "strmd5 is: " << strmd5 << endl;
                string respense = MD5(strmd5).hexdigest();
                authenticate << "Digest username=\"" << username
                            << "\",realm=\"" << realm
                            << "\",nonce=\"" << nonce 
                            << "\",uri=\"sip:" << uri 
                            << "\",response=\"" << respense
                            << "\",algorithm=MD5";
            }
            req->host = _req->host;
            req->host_port = _req->host_port;
            req->peer_ip = _req->peer_ip;
            req->peer_port = _req->peer_port;
            req->realm = _req->realm;
            req->serial = _req->serial;
            req->authorization = authenticate.str();
            usleep(1000 * 100);
            gbRegister(req);
        }
    }else if (req->is_message()) {
        if (req->cmdtype == SipCmdRespone) {
            logInfo << "get a message response" << endl;
            if (req->call_id.compare(0, 5, SIP_CALLID_KEEPALIVE) == 0) {
                logTrace << "get a keepalive response" << endl;
                _aliveStatus = 0;
            } else if (req->call_id.compare(0, 5, SIP_CALLID_CATALOG) == 0) {
                logTrace << "get a catalog response: " << req->call_id << endl;
                usleep(1000 * 10);
                auto info = _callid2Catalog[req->call_id];
                if (info._channelNum == 0) {
                    return ;
                }
                string callId = sendDevice(req->call_id, info);
                logTrace << "get a new catalog: " << callId << endl;
                //if (info._channelNum >= 0) {
                    //logTrace << "save a new catalog: " << callId << endl;
                    logTrace << "info._channelNum: " << info._channelNum << endl;
                    _callid2Catalog[callId] = info;
                //}
                if (callId != req->call_id) {
                    _callid2Catalog.erase(req->call_id);
                }
            } else if (req->call_id.compare(0, 5, SIP_CALLID_RECORDINFO) == 0) {
                logTrace << "get a recordinfo response" << endl;
            }
        } else if (req->content.find("Catalog") != string::npos) {
            logInfo << "get a catalog request" << endl;
            catalog(req);
        } else if (req->content.find("RecordInfo") != string::npos) {
            logInfo << "get a RecordInfo request" << endl;
        } else if (req->content.find("DeviceInfo") != string::npos) {
            logInfo << "get a DeviceInfo request" << endl;
            sendDeviceInfo(req);
        } else if (req->content.find("DeviceStatus") != string::npos) {
            logInfo << "get a DeviceStatus request" << endl;
            sendDeviceStatus(req);
        }
    }else if (req->is_invite()) {
        logInfo << "get a invite request; sip_auth_id is " << req->sip_auth_id << "; srerial is " << req->serial << endl;
        RtspSdpParser sdp;
        sdp.parse(req->content);
        auto track = sdp._vecSdpMedia[VideoTrackType];
        string ssrc = track->mapmedia_['y'];
        auto cVec = split(sdp._title->mapTitle_['c'], " ");
        string ip = cVec[2];
        int port = track->port_;
        bool isUdp = true;

        if (track->mapmedia_['m'].find("TCP") != string::npos)
        {
            isUdp = false;
        }

        logInfo << "ssrc is: " << ssrc << endl;

        // // function<void(const SockException &ex)> cb = [](const SockException &ex){
        // //     WarnL << "error is: " << ex.what() << endl;
        // // };
        req->host = _req->host;
        req->host_port = _req->host_port;
        req->peer_ip = _req->peer_ip;
        req->peer_port = _req->peer_port;
        req->sip_channel_id = req->sip_auth_id;
        // req->sdp = sdp;
        logInfo << "save sip_channel_id: " << req->sip_channel_id << endl;
        // // req->_device.reset(new DevChannel("__defaultVhost__", "live", req->sip_channel_id, 0, false, true, false, false));
        // // req->_device->addTrack(std::make_shared<H264Track>());
        // // req->_device->addTrackCompleted();
        logInfo << "ip: " << ip << "; port: " << port << endl;
        // // req->_device->startSendRtp(*((MediaSource*)this), ip, port, ssrc, isUdp, cb);
        _channel2Req[req->sip_channel_id][req->call_id] = req;
        std::stringstream ss;
        _sipStack.resp_invite(ss, req, ssrc);

        sendMessage(ss.str());

        static int timeout = Config::instance()->getAndListen([](const json& config){
            timeout = Config::instance()->get("Hook", "Http", "timeout");
        }, "Hook", "Http", "timeout");

        if (!timeout) {
            timeout = 5;
        }
        
        shared_ptr<HttpClientApi> client;
        string url = "http://127.0.0.1/api/v1/gb28181/send/create";
        json body;
        body["active"] = true;
        body["ssrc"] = stoi(ssrc);
        body["port"] = track->port_;
        body["socketType"] = isUdp ? 2 : 1;
        body["streamName"] = "test";
        body["appName"] = "live";
        body["ip"] = ip;
        client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
        client->addHeader("Content-Type", "application/json;charset=UTF-8");
        client->setMethod("GET");
        client->setContent(body.dump());
        client->setOnHttpResponce([client](const HttpParser &parser){
            // logInfo << "uri: " << parser._url;
            // logInfo << "status: " << parser._version;
            // logInfo << "method: " << parser._method;
            // logInfo << "_content: " << parser._content;
            if (parser._url != "200") {
                // cb("http error", "");
                logInfo << "http error";
                const_cast<shared_ptr<HttpClientApi> &>(client).reset();
                return ;
            }
            try {
                json value = json::parse(parser._content);
                // cb("", value);
            } catch (exception& ex) {
                logInfo << "json parse failed: " << ex.what();
                // cb(ex.what(), nullptr);
            }

            const_cast<shared_ptr<HttpClientApi> &>(client).reset();
        });
        logInfo << "connect to utl: " << url;
        if (client->sendHeader(url, timeout) != 0) {
            logInfo << "connect to url: " << url << " failed";
        }
    }else if (req->is_bye()) {
        stringstream ss;
        _sipStack.resp_status(ss, req);
        sendMessage(ss.str());
        auto creq = _channel2Req[req->sip_auth_id][req->call_id];
        // creq->_device->stopSendRtp(*((MediaSource*)this));
        creq->_start = false;
    }else if (req->is_ack()) {
        FILE* fp = fopen("test.h264", "rb");
        //int size = getNextNalu(fp, (unsigned char*)_buf);
        //cout << "get a frame. size is: " << size << endl;
        // if (size <= 0) {
        //     logError << "read file error" << endl;
        //     return;
        // }
        if (_channel2Req.find(req->sip_auth_id) == _channel2Req.end()){
            logInfo << "find channel req error: " << req->sip_auth_id << endl;
            return ;
        } else {
            auto channel = _channel2Req[req->sip_auth_id];
            if (channel.find(req->call_id) == channel.end()) {
                logInfo << "find call id req error: " << req->call_id << endl;
                return ;
            }
        }
        auto creq = _channel2Req[req->sip_auth_id][req->call_id];
        
        
        // auto track = creq->sdp.getTrack(VideoTrackType);
        // string ssrc = track->_other['y'];
        // string ip = track->_ip;
        // int port = track->_port;
        // int64_t dts = 0;
        // creq->_start = true;
        // // function<void(const SockException &ex)> cb = [](const SockException &ex){};
        // _channel2Timer[req->sip_auth_id][req->call_id] = std::make_shared<Timer>(67.0 / 1000,[this, dts, fp, creq]() {
        //     if (!creq->_start) {
        //         _channel2Req[creq->sip_channel_id][creq->call_id].reset();
        //         _channel2Req[creq->sip_channel_id].erase(creq->call_id);
        //         _channel2Timer[creq->sip_channel_id].erase(creq->call_id);
        //         return false;
        //     }
        //     int size = getNextNalu(fp, (unsigned char*)_buf);
        //     //cout << "get a frame. size is: " << size << endl;
        //     if (size <= 0) {
        //         fseek(fp, 0, SEEK_SET);
        //         size = getNextNalu(fp, (unsigned char*)_buf);
        //     }
        //     // creq->_device->inputH264(_buf, size, dts);
        //     //logInfo << creq->sip_channel_id << " ====> send a frame" << endl;
        //     return true;
        // }, nullptr);
    }else{
        //srs_trace("gb28181: ingor request method=%s", req->method.c_str());
        logTrace << "gb28181: ingor request method=" << req->method.c_str() << endl;
    }
}

