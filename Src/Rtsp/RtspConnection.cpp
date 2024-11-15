#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "RtspConnection.h"
#include "Logger.h"
#include "Util/String.h"
#include "RtspAuth.h"
#include "Common/Define.h"
#include "RtspPsMediaSource.h"
#include "Common/Config.h"
#include "Hook/MediaHook.h"

#include <sstream>

using namespace std;

RtspConnection::RtspConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "RtspConnection";
}

RtspConnection::~RtspConnection()
{
    logInfo << "~RtspConnection, _isPublish: " << _isPublish;
    auto rtspSrc = _source.lock();
    if (_isPublish && rtspSrc) {
        rtspSrc->release();
        // rtspSrc->delOnDetach(this);
    } else if (rtspSrc) {
        rtspSrc->delConnection(this);
        // rtspSrc->delOnDetach(this);
    }

    if (_playReader) {
        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTSP;
        info.status = "off";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void RtspConnection::init()
{
    weak_ptr<RtspConnection> wSelf = static_pointer_cast<RtspConnection>(shared_from_this());
    _parser.setOnRtspPacket([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->onRtspPacket();
    });

    _parser.setOnRtpPacket([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        // self->onRtpPacket();
        auto interleaved = data[1];
        auto rtpTrans = self->_mapRtpTransport.find(interleaved);
        if (rtpTrans != self->_mapRtpTransport.end()) {
            auto buffer = StreamBuffer::create();
            buffer->move((char*)data + 4, len - 4, false);
            rtpTrans->second->onRtpPacket(buffer);
        }
    });
}

void RtspConnection::close()
{
    TcpConnection::close();
}

void RtspConnection::onManager()
{
    // logInfo << "manager";
}

void RtspConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void RtspConnection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t RtspConnection::send(Buffer::Ptr pkt)
{
    // logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void RtspConnection::onRtspPacket()
{
    if (_baseUrl.empty()) {
        _baseUrl = _parser._url;
    }
    auto self = static_pointer_cast<RtspConnection>(shared_from_this());
    static unordered_map<string, void (RtspConnection::*)()> rtspHandle {
        {"OPTIONS", &RtspConnection::handleOption},
        {"DESCRIBE", &RtspConnection::handleDescribe},
        {"ANNOUNCE", &RtspConnection::handleAnnounce},
        {"RECORD", &RtspConnection::handleRecord},
        {"SETUP", &RtspConnection::handleSetup},
        {"PLAY", &RtspConnection::handlePlay},
        {"PAUSE", &RtspConnection::handlePause},
        {"TEARDOWN", &RtspConnection::handleTeardown},
        {"GET", &RtspConnection::handleGet},
        {"POST", &RtspConnection::handlePost},
        {"SET_PARAMETER", &RtspConnection::handleSetParam},
        {"GET_PARAMETER", &RtspConnection::handleGetParam}
    };

    logInfo << _parser._method;
    auto it = rtspHandle.find(_parser._method);
    if (it != rtspHandle.end()) {
        (this->*(it->second))();
    } else {
        // sendRtspResponse("403 Forbidden");
        // throw SockException(Err_shutdown, StrPrinter << "403 Forbidden:" << method);
        logWarn << _parser._content;
    }
}

void RtspConnection::sendMessage(const string& msg)
{
    // logInfo << msg << ", len : " << msg.size();
    send(std::make_shared<StringBuffer>(std::move(msg)));
}

void RtspConnection::handleOption()
{
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::responseDescribe(const MediaSource::Ptr &src)
{
    if (!src) {
        sendStreamNotFound();
        return;
    }

    auto rtspSrc = dynamic_pointer_cast<RtspMediaSource>(src);
    if (!rtspSrc) {
        sendStreamNotFound();
        return;
    }

    string sdp = rtspSrc->getSdp();
    _sdpParser.parse(sdp);
    if (_sdpParser._vecSdpMedia.size() == 0) {
        logInfo << "sdp media size: 0";
        sendStreamNotFound();
        return;
    }

    _sessionId = randomStr(12);
    _source = rtspSrc;
    // weak_ptr<RtspConnection> wSelf = dynamic_pointer_cast<RtspConnection>(shared_from_this());
    // self->_source->addOnDetach(self.get(), [wSelf](){
    //     auto self = wSelf.lock();
    //     if (!self) {
    //         return ;
    //     }
    //     self->close();
    // });

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
        << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
        << "Content-Length: " << sdp.size() << "\r\n"
        << "Content-Type: application/sdp\r\n"
        << "\r\n"
        << sdp;

    sendMessage(ss.str());
}

void RtspConnection::handleDescribe_l()
{
    weak_ptr<RtspConnection> wSelf = dynamic_pointer_cast<RtspConnection>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->_loop->async([wSelf, src](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->responseDescribe(src);
        }, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        if (self->_urlParser.type_ == "ps") {
            return make_shared<RtspPsMediaSource>(self->_urlParser, nullptr, true);
        } else {
            return make_shared<RtspMediaSource>(self->_urlParser, nullptr, true);
        }
    }, this);
}

void RtspConnection::handleDescribe()
{
    // 鉴权
    if (_authNonce.empty()) {
        _authNonce = randomStr(32);
    }
    // _baseUrl = _parser._url;
    _urlParser.parse(_baseUrl);
    // 配置读取
    static int needAuth = Config::instance()->getAndListen([](const json &config){
        needAuth = Config::instance()->get("Rtsp", "Server", "Server1", "rtspAuth");
    }, "Rtsp", "Server", "Server1", "rtspAuth");

    if (needAuth) {
        if (RtspAuth::rtspAuth(_authNonce, _parser)) {
            handleDescribe_l();
        } else {
            authFailed();
        }
    } else {
        PlayInfo info;
        info.protocol = _urlParser.protocol_;
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;
        info.params = _urlParser.param_;

        weak_ptr<RtspConnection> wSelf = dynamic_pointer_cast<RtspConnection>(shared_from_this());
        MediaHook::instance()->onPlay(info, [wSelf](const PlayResponse &rsp){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            if (!rsp.authResult) {
                self->sendBadRequst("on publish failed" + rsp.err);
                return ;
            }

            self->handleDescribe_l();
        });

        // handleDescribe_l();
    }

    // 查找源，获取sdp
    // string sdp = "";

    // std::stringstream ss;
    // ss << "RTSP/1.0 200 OK\r\n"
    //    << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
    //    << "Content-Length: " << sdp.size() << "\r\n"
    //    << "Content-Type: application/sdp\r\n"
    //    << "\r\n"
    //    << sdp;

    // sendMessage(ss.str());
}

void RtspConnection::authFailed()
{
    bool isBasic = false;
    string authHeader = "";
    if (!isBasic) {
        authHeader = "WWW-Authenticate: Digest realm=\"SimpleMediaServer\", nonce=\"" + _authNonce + "\"\r\n";
    } else {
        authHeader = "WWW-Authenticate: Basic realm=\"SimpleMediaServer\"\r\n";
    }

    std::stringstream ss;
    ss << "RTSP/1.0 401 Unauthorized\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << authHeader
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::handleAnnounce_l() {
    if (_source.lock()) {
        string err = "publish repeat";
        std::stringstream ss;
        ss << "RTSP/1.0 406 Not Acceptable\r\n"
        << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
        << "Content-Length: " << err.size() << "\r\n"
        << "Content-Type: application/text\r\n"
        << "\r\n"
        << err;

        sendMessage(ss.str());
        close();
        return ;
    }
    auto source = MediaSource::getOrCreate(_urlParser.path_, _urlParser.vhost_
                        , _urlParser.protocol_, _urlParser.type_
                        , [this](){
                            if (_urlParser.type_ == "ps") {
                                return dynamic_pointer_cast<RtspMediaSource>(make_shared<RtspPsMediaSource>(_urlParser, _loop));
                            } else {
                                return make_shared<RtspMediaSource>(_urlParser, _loop);
                            }
                        });
    if (!source) {
        string err = "publish repeat";
        std::stringstream ss;
        ss << "RTSP/1.0 406 Not Acceptable\r\n"
        << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
        << "Content-Length: " << err.size() << "\r\n"
        << "Content-Type: application/text\r\n"
        << "\r\n"
        << err;

        sendMessage(ss.str());
        close();
        return ;
    }

    auto rtspSource = dynamic_pointer_cast<RtspMediaSource>(source);
    // _source = dynamic_pointer_cast<RtspMediaSource>(source);
    rtspSource->setSdp(_parser._content);
    rtspSource->setOrigin();
    rtspSource->setOriginSocket(_socket);
    weak_ptr<RtspConnection> wSelf = dynamic_pointer_cast<RtspConnection>(shared_from_this());
    rtspSource->addOnDetach(this, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->close();
    });

    _sdpParser.parse(_parser._content);
    int trackIndex = 0;
    for (auto& media : _sdpParser._vecSdpMedia) {
        if (media->trackType_ == "audio" && media->codec_.empty()) {
            if (media->payloadType_ == 8) {
                media->codec_ = "pcma";
                media->channel_ = 1;
                media->samplerate_ = 8000;
            } else if (media->payloadType_ == 0) {
                media->codec_ = "pcmu";
                media->channel_ = 1;
                media->samplerate_ = 8000;
            } 
        }
        RtspTrack::Ptr track;
        if (_payloadType == "ps") {
            track = make_shared<RtspPsDecodeTrack>(trackIndex++, media);
        } else {
            track = make_shared<RtspDecodeTrack>(trackIndex++, media);
        }

        if (!track->getTrackInfo() || track->getTrackInfo()->codec_.empty()) {
            sendBadRequst("no track available");
            return ;
        }
        // logInfo << "type: " << track->getTrackType();
        logInfo << "index: " << track->getTrackIndex() << ", codec : " << media->codec_
                    << ", control: " << media->control_;
        rtspSource->addTrack(track);
        rtspSource->addControl2Index(media->control_, track->getTrackIndex());
    }
    rtspSource->onReady();

    _source = rtspSource;

    _sessionId = randomStr(12);
    _isPublish = true;

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::handleAnnounce()
{
    _authNonce = randomStr(32);
    // _baseUrl = _parser._url;
    _urlParser.parse(_baseUrl);
    // 配置读取
    static int needAuth = Config::instance()->getAndListen([](const json &config){
        needAuth = Config::instance()->get("Rtsp", "Server", "Server1", "rtspAuth");
    }, "Rtsp", "Server", "Server1", "rtspAuth");

    if (needAuth) {
        if (RtspAuth::rtspAuth(_authNonce, _parser)) {
            handleAnnounce_l();
        } else {
            authFailed();
        }
    } else {
        PublishInfo info;
        info.protocol = _urlParser.protocol_;
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;
        info.params = _urlParser.param_;

        weak_ptr<RtspConnection> wSelf = dynamic_pointer_cast<RtspConnection>(shared_from_this());
        MediaHook::instance()->onPublish(info, [wSelf](const PublishResponse &rsp){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            if (!rsp.authResult) {
                self->sendBadRequst("on publish failed" + rsp.err);
                return ;
            }

            self->handleAnnounce_l();
        });
    }
}

void RtspConnection::handleRecord()
{
    if (_mapRtpTransport.empty() || _sessionId.empty()) {
        sendSessionNotFound();
        return ;
    }

    // 推流交互完成，设置源可用
    // _source->setStatus(SourceStatus::AVAILABLE);

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Session: " << _sessionId << "\r\n"
       << "Rtp-Info: url=" << _baseUrl << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::sendBadRequst(const string& err)
{
    std::stringstream ss;
    ss << "RTSP/1.0 400 Bad Request\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Content-Length: " << err.size() << "\r\n"
       << "Content-Type: application/text\r\n"
       << "\r\n"
       << err;

    sendMessage(ss.str());
    close();
}

void RtspConnection::sendNotAcceptable()
{
    std::stringstream ss;
    ss << "RTSP/1.0 406 Not Acceptable\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Connection: Close\r\n"
       << "\r\n";

    sendMessage(ss.str());
    close();
}

void RtspConnection::sendSessionNotFound()
{
    std::stringstream ss;
    ss << "RTSP/1.0 454 Session Not Found\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Connection: Close\r\n"
       << "\r\n";

    sendMessage(ss.str());
    close();
}

void RtspConnection::sendStreamNotFound()
{
    std::stringstream ss;
    ss << "RTSP/1.0 404 Stream Not Found\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Connection: Close\r\n"
       << "\r\n";

    sendMessage(ss.str());
    close();
}

void RtspConnection::sendUnsupportedTransport()
{
    std::stringstream ss;
    ss << "RTSP/1.0 461 Unsupported transport\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::handleSetup()
{
    logInfo << "url: " << _parser._url;
    auto rtspSrc = _source.lock();
    if (!rtspSrc) {
        sendBadRequst("source is offline");
    }
    int index = -1;
    string codec = "unknwon";
    string control = "unknown";
    for (auto& media : _sdpParser._vecSdpMedia) {
        if (media->control_.find("://") != string::npos) {
            // logInfo << "control_: " << media->control_;
            if (_parser._url == media->control_) {
                index = rtspSrc->getIndexByControl(media->control_);
                codec = media->codec_;
                control = media->control_;
                // index = media->index_;
                break;
            }
        } else {
            auto basecontrol = _baseUrl + "/" + media->control_;
            // logInfo << "control_: " << basecontrol;
            if (_parser._url == basecontrol) {
                // index = media->index_;
                index = rtspSrc->getIndexByControl(media->control_);
                codec = media->codec_;
                control = media->control_;
                break;
            }
        }
    }
    if (index == -1) {
        sendBadRequst("can not find the control url");
        return ;
    }

    logInfo << "index: " << index << ", codec: " << codec << ", control: " << control;

    auto track = rtspSrc->getTrack(index);
    if (!track) {
        sendBadRequst("can not find track");
        return ;
    } else if (track->hasSetup()) {
        sendBadRequst("track have setuped");
        return ;
    }

    // 此处可以通过配置，指定只支持tcp或udp
    auto trans = _parser._mapHeaders["transport"];
    string serverPortStr;
    // logInfo << "trans: " << trans;
    if (trans.find("TCP") != string::npos) {
        auto rtpTrans = make_shared<RtspRtpTransport>(Transport_TCP, TransportData_Media, track, _socket);
        auto rtcpTrans = make_shared<RtspRtcpTransport>(Transport_TCP, TransportData_Data, track, _socket);

        auto vecTrans = split(_parser._mapHeaders["transport"], ";", "=");
        int interleavedRtp = 0; 
        int interleavedRtcp = 0;
        if (2 != sscanf(vecTrans["interleaved"].data(), "%d-%d", &interleavedRtp, &interleavedRtcp)) {
            sendBadRequst("parse interleaved error");
            return ;
        }
        if (_isPublish) {
            _mapRtpTransport.emplace(interleavedRtp, rtpTrans);
            _mapRtcpTransport.emplace(interleavedRtcp, rtcpTrans);
            if (vecTrans.find("ssrc") != vecTrans.end()) {
                track->setSsrc(strtol(vecTrans["ssrc"].data(), 0, 16));
            } else {
                track->setSsrc(track->getTrackIndex() + 1000);
            }
            // track->setInterleavedRtp(interleavedRtp);
        } else {
            interleavedRtp = index * 2;
            trans = "RTP/AVP/TCP;unicast;interleaved=" + to_string(interleavedRtp) + "-" + to_string(interleavedRtp + 1) + ";";
            // track->setInterleavedRtp(0/*interleavedRtp*/);
            _mapRtpTransport.emplace(index * 2, rtpTrans);
            _mapRtcpTransport.emplace(index * 2 + 1, rtcpTrans);
        }
    } else if (trans.find("multicast") != string::npos) {

    } else {
        auto socketRtp = make_shared<Socket>(_loop);
        auto socketRtcp = make_shared<Socket>(_loop);

        socketRtp->createSocket(SOCKET_UDP);
        // logInfo << "bind rtp socket: ";
        if (socketRtp->bind(0, "0.0.0.0") == -1) {
            sendNotAcceptable();
            return ;
        }
        auto rtpPort = socketRtp->getLocalPort();
        logInfo << "getLocalPort: " << rtpPort;
        if (rtpPort < 0) {
            sendNotAcceptable();
            return ;
        }
        int rtcpPort = rtpPort + 1;
        if (rtpPort % 2 != 0) {
            rtcpPort = rtpPort - 1;
        }
        socketRtcp->createSocket(SOCKET_UDP);
        // logInfo << "bind rtcp socket: ";
        if (socketRtcp->bind(rtcpPort, "0.0.0.0") == -1) {
            sendNotAcceptable();
            return ;
        }
        if (rtpPort % 2 != 0) {
            auto tmpSocket = socketRtp;
            socketRtp = socketRtcp;
            socketRtcp = tmpSocket;
        }

        //设置客户端内网端口信息
        string strClientPort = findSubStr(trans, "client_port=", "");
        uint16_t ui16RtpPort = atoi(findSubStr(strClientPort, "", "-").data());
        uint16_t ui16RtcpPort = atoi(findSubStr(strClientPort, "-", "").data());

        // logInfo << "client port: " << strClientPort;
        // logInfo << "client port: " << findSubStr(strClientPort, "", "-");
        // logInfo << "client port: " << findSubStr(strClientPort, "-", "");

        struct sockaddr_in peerAddr;
        //设置rtp发送目标地址
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(ui16RtpPort);
        peerAddr.sin_addr.s_addr = inet_addr(_socket->getPeerIp().data());
        bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
        socketRtp->bindPeerAddr((struct sockaddr *) (&peerAddr));

        //设置rtcp发送目标地址
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(ui16RtcpPort);
        peerAddr.sin_addr.s_addr = inet_addr(_socket->getPeerIp().data());
        bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
        socketRtcp->bindPeerAddr((struct sockaddr *) (&peerAddr));

        socketRtp->addToEpoll();
        socketRtcp->addToEpoll();
        auto rtpTrans = make_shared<RtspRtpTransport>(Transport_UDP, TransportData_Media, track, socketRtp);
        auto rtcpTrans = make_shared<RtspRtcpTransport>(Transport_UDP, TransportData_Data, track, socketRtcp);
        _mapRtpTransport.emplace(index * 2, rtpTrans);
        _mapRtcpTransport.emplace(index * 2 + 1, rtcpTrans);
        rtpTrans->start();
        rtcpTrans->start();
        serverPortStr = ";server_port=" + to_string(socketRtp->getLocalPort()) + "-" + to_string(socketRtcp->getLocalPort());
    }

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Session: " << _sessionId << "\r\n"
       << "Transport: " << trans << serverPortStr << ";ssrc=" << track->getSsrc() << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::handlePlay()
{
    auto rtspSrc = _source.lock();
    if (_sessionId.empty() || 
        _sessionId != _parser._mapHeaders["session"] || 
        _sdpParser._vecSdpMedia.empty() ||
        !rtspSrc)
    {
        sendStreamNotFound();
        return ;
    }
    
    auto it = _parser._mapHeaders.find("scale");
    if (it != _parser._mapHeaders.end()) {
        float scale = stof(it->second);
        // _source->scale();
    }
    
    it = _parser._mapHeaders.find("range");
    string range;
    if (it != _parser._mapHeaders.end()) {
        range = it->second;
        auto mapRange = split(range, "npt=", "-");
        auto startTime = mapRange.begin()->first;
        auto endTime = mapRange.begin()->second;

        if (startTime == "now" || startTime == "") {
            ;
        } else {
            // _source->seek(stof(startTime));
        }
    }

    // auto tracks = rtspSrc->getTrack();
    string rtpInfo;
    string controlUrl;
    for (auto& media : _sdpParser._vecSdpMedia) {
        if (media->control_.find("://") != string::npos) {
            controlUrl = media->control_;
        } else {
            controlUrl = _baseUrl + "/" + media->control_;
        }
        int index = rtspSrc->getIndexByControl(media->control_);
        auto track = rtspSrc->getTrack(index);
        if (!track) {
            logError << "track is empty, server invalid";
            sendBadRequst("track is empty, server invalid");
            return ;
        }
        rtpInfo += "url=" + controlUrl + ";" +
                   "seq=" + to_string(track->getSeq() + 1) + ";" +
                   "rtptime=" + to_string(track->getTimestamp()) + ",";
    }
    rtpInfo.pop_back();

    // 恢复播放
    // _source->pause();

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "Session:" << _sessionId << "\r\n"
       << "Range: " << (range.empty() ? "npt=0.000000-" : range) << "\r\n"
       << "RTP-Info" << rtpInfo << "\r\n"
       << "\r\n";

    sendMessage(ss.str());

    if (!_playReader/* && _rtp_type != Rtsp::RTP_MULTICAST*/) {
        weak_ptr<RtspConnection> weak_self = static_pointer_cast<RtspConnection>(shared_from_this());
        // if (!_sendTimer) {
        //     _sendTimer = make_shared<Timer>(40 / 1000.0, [weak_self] {
        //         auto strongSelf = weak_self.lock();
        //         if (!strongSelf) {
        //             return false;
        //         }
        //         while (strongSelf->_rtpList.size() > 0) {
        //             RtspMediaSource::RingDataType pack = strongSelf->_rtpList.front();
        //             strongSelf->_rtpList.pop_front();
        //             if (pack && pack.get()) {
        //                 strongSelf->sendRtpPacket(pack);
        //             }
        //         }

        //         return true;
        //     }, getPoller());
        // }
        static int interval = Config::instance()->getAndListen([](const json &config){
            interval = Config::instance()->get("Rtsp", "Server", "Server1", "interval");
            if (interval == 0) {
                interval = 5000;
            }
        }, "Rtsp", "Server", "Server1", "interval");

        if (interval == 0) {
            interval = 5000;
        }
        weak_ptr<RtspConnection> wSelf = static_pointer_cast<RtspConnection>(shared_from_this());
        _loop->addTimerTask(interval, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            self->_lastBitrate = self->_intervalSendBytes / (interval / 1000.0);
            self->_intervalSendBytes = 0;

            return interval;
        }, nullptr);
        
        _playReader = rtspSrc->getRing()->attach(_loop, true);
        _playReader->setGetInfoCB([weak_self]() {
            auto self = weak_self.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->_socket->getPeerIp();
            ret.port_ = self->_socket->getPeerPort();
            ret.protocol_ = PROTOCOL_RTSP;
            ret.close_ = [weak_self](){
                auto self = weak_self.lock();
                if (self) {
                    self->onError();
                }
            };
            ret.bitrate_ = self->_lastBitrate;
            return ret;
        });
        _playReader->setDetachCB([weak_self]() {
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return;
            }
            // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
            strong_self->close();
        });
        _playReader->setReadCB([weak_self](const RtspMediaSource::DataType &pack) {
            auto strong_self = weak_self.lock();
            if (!strong_self/* || pack->empty()*/) {
                return;
            }
            auto rtp = pack->front();
            int index = rtp->trackIndex_;
            // logInfo << "rtp index: " << index;
            auto transport = strong_self->_mapRtpTransport[pack->front()->trackIndex_ * 2];
            // for (auto rtptrans : strong_self->_mapRtpTransport) {
                // logInfo << "index: " << rtptrans.first;
            // }
            // logInfo << "index: " << index;
            if (transport) {
                // logInfo << "sendRtpPacket: " << index;
                int bytes = transport->sendRtpPacket(pack);
                strong_self->_intervalSendBytes += bytes;
                strong_self->_totalSendBytes += bytes;
            }
            // strong_self->_rtpList.push_back(pack);
        });

        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTSP;
        info.status = "on";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void RtspConnection::handlePause()
{
    if (_mapRtpTransport.empty() || _sessionId.empty()) {
        sendSessionNotFound();
        return ;
    }
    if (!_isPublish && _source.lock()) {
        // _source->pause();
    }
    
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::handleTeardown()
{
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "\r\n";

    sendMessage(ss.str());

    close();
}

void RtspConnection::handleGet()
{

}

void RtspConnection::handlePost()
{

}

void RtspConnection::handleSetParam()
{
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}

void RtspConnection::handleGetParam()
{
    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n"
       << "CSeq: " << _parser._mapHeaders["cseq"] << "\r\n"
       << "\r\n";

    sendMessage(ss.str());
}
