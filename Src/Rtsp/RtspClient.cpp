#include "RtspClient.h"
#include "Common/Define.h"
#include "Util/String.h"
#include "Util/MD5.h"
#include "Hook/MediaHook.h"
#include "RtspPsMediaSource.h"

#include <arpa/inet.h>

RtspClient::RtspClient(MediaClientType type, const string& appName, const string& streamName)
    :TcpClient(EventLoop::getCurrentLoop())
    ,_type(type)
{
    _localUrlParser.path_ = "/" + appName + "/" + streamName;
    _localUrlParser.protocol_ = PROTOCOL_RTSP;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

RtspClient::~RtspClient()
{
    logInfo << "~RtspClient";
    auto rtspSrc = _source.lock();
    if (_type == MediaClientType_Pull && rtspSrc) {
        rtspSrc->delConnection(this);
        rtspSrc->release();
    } else if (rtspSrc) {
        rtspSrc->delConnection(this);
    }

    if (_playReader) {
        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTSP;
        info.status = "off";
        info.type = _localUrlParser.type_;
        info.uri = _localUrlParser.path_;
        info.vhost = _localUrlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void RtspClient::init()
{
    MediaClient::registerCreateClient("rtsp", [](MediaClientType type, const std::string &appName, const std::string &streamName){
        return make_shared<RtspClient>(type, appName, streamName);
    });
}

void RtspClient::setTransType(int type)
{
    _rtpType = (TransportType)type;
}

void RtspClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    _url = url;

    weak_ptr<RtspClient> wSelf = static_pointer_cast<RtspClient>(shared_from_this());
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

    logInfo << "localIp: " << localIp << ", localPort: " << localPort;
    if (TcpClient::create(localIp, localPort) < 0) {
        close();
        logInfo << "TcpClient::create failed: " << strerror(errno);
        return ;
    }

    _peerUrlParser.parse(url);
    logInfo << "_peerUrlParser.host_: " << _peerUrlParser.host_ << ", _peerUrlParser.port_: " << _peerUrlParser.port_
            << ", timeout: " << timeout;
    _peerUrlParser.port_ = _peerUrlParser.port_ == 0 ? 554 : _peerUrlParser.port_;
    if (TcpClient::connect(_peerUrlParser.host_, _peerUrlParser.port_, timeout) < 0) {
        close();
        logInfo << "TcpClient::connect, ip: " << _peerUrlParser.host_ << ", peerPort: " 
                << _peerUrlParser.port_ << ", failed: " << strerror(errno);
        return ;
    }
}

void RtspClient::stop()
{
    sendTeardown();
    close();
}

void RtspClient::pause()
{
    sendPause();
}

void RtspClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void RtspClient::addOnReady(void* key, const function<void()>& onReady)
{
    lock_guard<mutex> lck(_mtx);
    auto rtspSrc =_source.lock();
    if (rtspSrc) {
        rtspSrc->addOnReady(key, onReady);
        return ;
    }
    _mapOnReady.emplace(key, onReady);
}

void RtspClient::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buffer: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void RtspClient::onError(const string& err)
{
    logWarn << "get a err: " << err;
    close();
}

void RtspClient::close()
{
    if (_onClose) {
        _onClose();
    }

    TcpClient::close();
}

void RtspClient::onConnect()
{
    _socket = TcpClient::getSocket();
    _loop = _socket->getLoop();
    sendOption();
    _state = RTSP_SEND_DESCRIBE_ANNOUNCE;
}

void RtspClient::onRtspPacket()
{
    auto self = static_pointer_cast<RtspClient>(shared_from_this());
    static unordered_map<int, void (RtspClient::*)()> rtspHandle {
        {RTSP_SEND_DESCRIBE_ANNOUNCE, &RtspClient::sendDescribeOrAnnounce},
        {RTSP_SEND_SETUP, &RtspClient::sendSetup},
        {RTSP_SEND_PLAY_PUBLISH, &RtspClient::sendPlayOrPublish}
    };

    logInfo << "state: " << _state;

    auto it = rtspHandle.find(_state);
    if (it != rtspHandle.end()) {
        (this->*(it->second))();
    } else {
        // sendRtspResponse("403 Forbidden");
        // throw SockException(Err_shutdown, StrPrinter << "403 Forbidden:" << method);
    }
}

void RtspClient::sendMessage(const string& msg)
{
    // logInfo << msg << ", len : " << msg.size();
    send(std::make_shared<StringBuffer>(std::move(msg)));
}

void RtspClient::sendOption()
{
    logInfo << "RtspClient::sendOption";

    stringstream ss;
    ss << "OPTIONS " << _url << " RTSP/1.0" << CRLF
       << "CSeq: " << ++_seq << CRLF
       << "User-Agent: " << "SMS v0.1" << CRLF
       << CRLF;

    logInfo << "options: " << ss.str();
    _state = RTSP_SEND_DESCRIBE_ANNOUNCE;
    sendMessage(ss.str());
}

void RtspClient::sendDescribeOrAnnounce()
{
    logInfo << "_parser._version: " << _parser._version;

    if (_parser._version != "OK") {
        onError("option failed");
        return ;
    }

    if (_type == MediaClientType_Pull) {
        stringstream ss;
        ss << "DESCRIBE " << _url << " RTSP/1.0" << CRLF
           << "CSeq: " << ++_seq << CRLF
           << "User-Agent: " << "SMS v0.1" << CRLF
           << "Accept: application/sdp" << CRLF
           << CRLF;

        sendMessage(ss.str());
        _state = RTSP_SEND_SETUP;
    } else if (_type == MediaClientType_Push) {
        weak_ptr<RtspClient> wSelf = dynamic_pointer_cast<RtspClient>(shared_from_this());
        MediaSource::getOrCreateAsync(_localUrlParser.path_, _localUrlParser.vhost_, 
                                        _localUrlParser.protocol_, _localUrlParser.type_, 
        [wSelf](const MediaSource::Ptr &src){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            if (!src) {
                self->onError("source is empty");
                return ;
            }

            self->_loop->async([wSelf, src](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }

                auto rtspSource = dynamic_pointer_cast<RtspMediaSource>(src);
                self->_source = rtspSource;
                string sdp = rtspSource->getSdp();
                self->_sdpParser.parse(sdp);
                if (self->_sdpParser._vecSdpMedia.size() == 0) {
                    logInfo << "sdp media size: 0";
                    self->onError("sdp media size: 0");
                    return;
                }
                stringstream ss;
                ss << "ANNOUNCE " << self->_url << " RTSP/1.0" << CRLF
                   << "CSeq: " << ++self->_seq << CRLF
                   << "User-Agent: " << "SMS v0.1" << CRLF
                   << "Content-Type: application/sdp" << CRLF
                   << "Content-Length: " << sdp.size() << CRLF
                   << CRLF
                   << sdp;

                self->sendMessage(ss.str());
                self->_state = RTSP_SEND_SETUP;
            }, true);
        }, 
        [wSelf]() -> MediaSource::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            return make_shared<RtspMediaSource>(self->_localUrlParser, nullptr, true);
        }, this);
    } else {
        onError("media client type is error");
    }
}

void RtspClient::sendDescribeWithAuthInfo()
{
    string authStr = _parser._mapHeaders["www-authenticate"];
    if (authStr.empty()) {
        return ;
    }

    if (_username.empty() && !_peerUrlParser.username_.empty()) {
        _username = _peerUrlParser.username_;
        _pwd = _peerUrlParser.password_;
    }

    // logInfo << "rtspAuth authStr";
    //请求中包含认证信息
    auto pos = authStr.find_first_of(" ");
    if (pos == string::npos) {
        return ;
    }

    auto type = authStr.substr(0, pos);
    auto content = authStr.substr(pos + 1);

    auto authMap = split(content, ",", "=", " \"");
    string realm = authMap["realm"];
    string nonce = authMap["nonce"];
    string username = _username;

    string pwd = _pwd;
    string encPwd = MD5(username+ ":" + realm + ":" + pwd).hexdigest();
    auto encRes = MD5( encPwd + ":" + nonce + ":" + MD5(string("DESCRIBE") + ":" + _url).hexdigest()).hexdigest();

    stringstream ss;
    ss << "DESCRIBE " << _url << " RTSP/1.0" << CRLF
       << "CSeq: " << ++_seq << CRLF
       << "Authorization: Digest username=\"" << username << "\"" << ", realm=\"" << realm << "\", nonce=\""
            << nonce << "\", uri=\"" << _url << "\", response=\"" << encRes << CRLF
       << "User-Agent: " << "SMS v0.1" << CRLF
       << "Accept: application/sdp" << CRLF
       << CRLF;

    sendMessage(ss.str());

    _state = RTSP_SEND_SETUP;
}

void RtspClient::sendSetup()
{
    logInfo << "_parser._version: " << _parser._version;
    logInfo << "_setupIndex: " << _setupIndex;
    if (_setupIndex == 0) {
        if (_parser._version == "Unauthorized") {
            if (_hasAuth) {
                onError("auth error");
                return;
            }
            _hasAuth = true;
            sendDescribeWithAuthInfo();
            return ;
        } else if (_parser._version != "OK") {
            onError("describe failed");
            return ;
        }

        if (_type == MediaClientType_Pull) {
            MediaSource::Ptr source;
            _sdpParser.parse(_parser._content);

            while (_sdpParser._vecSdpMedia.size() == 1) {
                auto iter = _sdpParser._vecSdpMedia.begin();
                if (toLower((*iter)->codec_) == "mp2p") {
                    _localUrlParser.type_ = "ps";
                    source = MediaSource::getOrCreate(_localUrlParser.path_, _localUrlParser.vhost_
                                , _localUrlParser.protocol_, _localUrlParser.type_
                                , [this](){
                                    return make_shared<RtspPsMediaSource>(_localUrlParser, _loop);
                                });
                } else {
                    break;
                }

                if (!source) {
                    onError("source is exists");
                    return ;
                }

                auto rtspSource = dynamic_pointer_cast<RtspPsMediaSource>(source);
                // _source = dynamic_pointer_cast<RtspMediaSource>(source);
                rtspSource->setSdp(_parser._content);
                rtspSource->setOrigin();

                int trackIndex = 0;
                for (auto& media : _sdpParser._vecSdpMedia) {
                    auto track = make_shared<RtspPsDecodeTrack>(trackIndex++, media);
                    // logInfo << "type: " << track->getTrackType();
                    logInfo << "index: " << track->getTrackIndex() << ", codec : " << media->codec_
                                << ", control: " << media->control_;
                    rtspSource->addTrack(track);
                    rtspSource->addControl2Index(media->control_, track->getTrackIndex());
                }

                _source = rtspSource;
                sendSetup(rtspSource);

                return ;
            }

            source = MediaSource::getOrCreate(_localUrlParser.path_, _localUrlParser.vhost_
                                , _localUrlParser.protocol_, _localUrlParser.type_
                                , [this](){
                                    return make_shared<RtspMediaSource>(_localUrlParser, _loop);
                                });

            if (!source) {
                onError("source is exists");
                return ;
            }

            auto rtspSource = dynamic_pointer_cast<RtspMediaSource>(source);
            // _source = dynamic_pointer_cast<RtspMediaSource>(source);
            rtspSource->setSdp(_parser._content);
            rtspSource->setOrigin();

            int trackIndex = 0;
            for (auto& media : _sdpParser._vecSdpMedia) {
                auto track = make_shared<RtspDecodeTrack>(trackIndex++, media);
                // logInfo << "type: " << track->getTrackType();
                logInfo << "index: " << track->getTrackIndex() << ", codec : " << media->codec_
                            << ", control: " << media->control_;
                rtspSource->addTrack(track);
                rtspSource->addControl2Index(media->control_, track->getTrackIndex());
            }
            rtspSource->onReady();

            _source = rtspSource;
            sendSetup(rtspSource);

            lock_guard<mutex> lck(_mtx);
            for (auto &iter : _mapOnReady) {
                rtspSource->addOnReady(iter.first, iter.second);
            }
            _mapOnReady.clear();
        } else {
            sendSetup(_source.lock());
        }
    } else {
        if (_rtpType == Transport_UDP) {
            bindRtxpSocket();
        }
        sendSetup(_source.lock());
    }    
}

void RtspClient::sendSetup(const RtspMediaSource::Ptr& rtspSrc)
{
    // auto rtspSrc = _source.lock();

    auto media = _sdpParser._vecSdpMedia[_setupIndex];

    stringstream ss;
    ss << "SETUP " << _url << "/" << media->control_ << " RTSP/1.0" << CRLF
       << "CSeq: " << ++_seq << CRLF
       << "User-Agent: " << "SMS v0.1" << CRLF;
    
    if (_rtpType == Transport_TCP) {
        ss << "Transport: RTP/AVP/TCP;unicast;interleaved=" << (2*_setupIndex) << "-" << (2*_setupIndex + 1) << CRLF;
        auto track = rtspSrc->getTrack(_setupIndex);
        auto rtpTrans = make_shared<RtspRtpTransport>(Transport_TCP, TransportData_Media, track, _socket);
        auto rtcpTrans = make_shared<RtspRtcpTransport>(Transport_TCP, TransportData_Data, track, _socket);
        if (_type == MediaClientType_Pull) {
            _mapRtpTransport.emplace(2*_setupIndex, rtpTrans);
            _mapRtcpTransport.emplace(2*_setupIndex + 1, rtcpTrans);
            track->setInterleavedRtp(2*_setupIndex);
        } else {
            track->setInterleavedRtp(2*_setupIndex);
            _mapRtpTransport.emplace(2*_setupIndex, rtpTrans);
            _mapRtcpTransport.emplace(2*_setupIndex + 1, rtcpTrans);
        }
    } else {
        // create socket
        auto track = rtspSrc->getTrack(_setupIndex);
        auto socketRtp = make_shared<Socket>(_loop);
        auto socketRtcp = make_shared<Socket>(_loop);

        socketRtp->createSocket(SOCKET_UDP);
        // logInfo << "bind rtp socket: ";
        if (socketRtp->bind(0, "0.0.0.0") == -1) {
            onError("create rtp/udp socket failed");
            return ;
        }
        auto rtpPort = socketRtp->getLocalPort();
        logInfo << "getLocalPort: " << rtpPort;
        if (rtpPort < 0) {
            onError("get rtp/udp port failed");
            return ;
        }
        socketRtcp->createSocket(SOCKET_UDP);
        // logInfo << "bind rtcp socket: ";
        if (socketRtcp->bind(rtpPort + 1, "0.0.0.0") == -1) {
            onError("create rtcp/udp socket failed");
            return ;
        }
        if (rtpPort % 2 != 0) {
            auto tmpSocket = socketRtp;
            socketRtp = socketRtcp;
            socketRtcp = tmpSocket;
        }

        //设置客户端内网端口信息
        // string strClientPort = findSubStr(trans, "client_port=", "");
        // uint16_t ui16RtpPort = atoi(findSubStr(strClientPort, "", "-").data());
        // uint16_t ui16RtcpPort = atoi(findSubStr(strClientPort, "-", "").data());

        // logInfo << "client port: " << strClientPort;
        // logInfo << "client port: " << findSubStr(strClientPort, "", "-");
        // logInfo << "client port: " << findSubStr(strClientPort, "-", "");

        // struct sockaddr_in peerAddr;
        // //设置rtp发送目标地址
        // peerAddr.sin_family = AF_INET;
        // peerAddr.sin_port = htons(ui16RtpPort);
        // peerAddr.sin_addr.s_addr = inet_addr(_socket->getPeerIp().data());
        // bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
        // socketRtp->bindPeerAddr((struct sockaddr *) (&peerAddr));

        // //设置rtcp发送目标地址
        // peerAddr.sin_family = AF_INET;
        // peerAddr.sin_port = htons(ui16RtcpPort);
        // peerAddr.sin_addr.s_addr = inet_addr(_socket->getPeerIp().data());
        // bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
        // socketRtcp->bindPeerAddr((struct sockaddr *) (&peerAddr));

        socketRtp->addToEpoll();
        socketRtcp->addToEpoll();
        auto rtpTrans = make_shared<RtspRtpTransport>(Transport_UDP, TransportData_Media, track, socketRtp);
        auto rtcpTrans = make_shared<RtspRtcpTransport>(Transport_UDP, TransportData_Data, track, socketRtcp);
        _mapRtpTransport.emplace(_setupIndex * 2, rtpTrans);
        _mapRtcpTransport.emplace(_setupIndex * 2 + 1, rtcpTrans);
        rtpTrans->start();
        rtcpTrans->start();
        auto clientPortStr = ";client_port=" + to_string(socketRtp->getLocalPort()) + "-" + to_string(socketRtcp->getLocalPort());
        
        ss << "Transport: RTP/AVP;unicast;client_port=" << clientPortStr << CRLF;
    }
    ss << CRLF;

    ++_setupIndex;

    if (_setupIndex == _sdpParser._vecSdpMedia.size()) {
        _state = RTSP_SEND_PLAY_PUBLISH;
    }

    sendMessage(ss.str());
}

void RtspClient::sendPlayOrPublish()
{
    if (_parser._version != "OK") {
        onError("setup failed");
        return ;
    }

    _sessionId = _parser._mapHeaders["session"];

    stringstream ss;
    if (_type == MediaClientType_Pull) {
        if (_rtpType == Transport_UDP) {
            bindRtxpSocket();
        }

        ss << "PLAY " << _url << " RTSP/1.0" << CRLF
           << "CSeq: " << ++_seq << CRLF
           << "User-Agent: " << "SMS v0.1" << CRLF
           << "Session: " << _sessionId << CRLF
           << "Range: npt=0.000-" << CRLF
           << CRLF;
    } else if (_type == MediaClientType_Push) {
        if (_rtpType == Transport_UDP) {
            bindRtxpSocket();
        }
        ss << "RECORD " << _url << " RTSP/1.0" << CRLF
           << "CSeq: " << ++_seq << CRLF
           << "User-Agent: " << "SMS v0.1" << CRLF
           << "Session: " << _sessionId << CRLF
           << CRLF;

        if (!_playReader/* && _rtp_type != Rtsp::RTP_MULTICAST*/) {
            weak_ptr<RtspClient> weak_self = static_pointer_cast<RtspClient>(shared_from_this());
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
            auto rtspSrc = _source.lock();

            _playReader = rtspSrc->getRing()->attach(_loop, true);
            _playReader->setGetInfoCB([weak_self]() {
                auto self = weak_self.lock();
                ClientInfo ret;
                if (!self) {
                    return ret;
                }
                ret.ip_ = self->_socket->getLocalIp();
                ret.port_ = self->_socket->getLocalPort();
                ret.protocol_ = PROTOCOL_RTSP;
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
                // auto rtp = pack->front();
                // int index = rtp->trackIndex_;
                // logInfo << "rtp index: " << index;
                auto transport = strong_self->_mapRtpTransport[pack->front()->trackIndex_ * 2];
                // for (auto rtptrans : strong_self->_mapRtpTransport) {
                    // logInfo << "index: " << rtptrans.first;
                // }
                // logInfo << "index: " << index;
                if (transport) {
                    // logInfo << "sendRtpPacket: " << index;
                    transport->sendRtpPacket(pack);
                }
                // strong_self->_rtpList.push_back(pack);
            });

            PlayerInfo info;
            info.ip = _socket->getPeerIp();
            info.port = _socket->getPeerPort();
            info.protocol = PROTOCOL_RTSP;
            info.status = "on";
            info.type = _localUrlParser.type_;
            info.uri = _localUrlParser.path_;
            info.vhost = _localUrlParser.vhost_;

            MediaHook::instance()->onPlayer(info);
        }
    }

    sendMessage(ss.str());

    _state = RTSP_FINISH_PLAY_PUBLISH;
}

void RtspClient::sendPause(int seekMs)
{
    stringstream ss;
    ss << "PAUSE " << _url << " RTSP/1.0" << CRLF
       << "CSeq: " << ++_seq << CRLF
       << "User-Agent: " << "SMS v0.1" << CRLF
       << "Session: " << _sessionId << CRLF;
    if (seekMs) {
        ss << "Range: npt=" << (seekMs / 1000.0) << "-" << CRLF;
    }
    
    ss << CRLF;

    sendMessage(ss.str());
}

void RtspClient::sendTeardown()
{
    stringstream ss;
    ss << "TEARDOWN " << _url << " RTSP/1.0" << CRLF
       << "CSeq: " << ++_seq << CRLF
       << "User-Agent: " << "SMS v0.1" << CRLF
       << "Session: " << _sessionId << CRLF
       << CRLF;
    
    sendMessage(ss.str());
}

void RtspClient::bindRtxpSocket()
{
    auto& trans = _parser._mapHeaders["transport"];
    string strServerPort = findSubStr(trans, "server_port=", "");
    uint16_t ui16RtpPort = atoi(findSubStr(strServerPort, "", "-").data());
    uint16_t ui16RtcpPort = atoi(findSubStr(strServerPort, "-", "").data());

    struct sockaddr_in peerAddr;
    //设置rtp发送目标地址
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(ui16RtpPort);
    peerAddr.sin_addr.s_addr = inet_addr(_socket->getPeerIp().data());
    bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
    _mapRtpTransport[2*(_setupIndex - 1)]->getSocket()->bindPeerAddr((struct sockaddr *) (&peerAddr));
    _mapRtpTransport[2*(_setupIndex - 1)]->getSocket()->send("\xce\xfa\xed\xfe", 4, 1);

    //设置rtcp发送目标地址
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(ui16RtcpPort);
    peerAddr.sin_addr.s_addr = inet_addr(_socket->getPeerIp().data());
    bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
    _mapRtcpTransport[2*(_setupIndex - 1) + 1]->getSocket()->bindPeerAddr((struct sockaddr *) (&peerAddr));
    _mapRtcpTransport[2*(_setupIndex - 1) + 1]->getSocket()->send("\xce\xfa\xed\xfe", 4, 1);
}