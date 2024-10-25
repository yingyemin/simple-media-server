#include "WebrtcContext.h"
#include "Log/Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/Config.h"
#include "Common/MediaSource.h"
#include "WebrtcSdpParser.h"
#include "WebrtcTrack.h"
#include "WebrtcRtcpPacket.h"
#include "WebrtcContextManager.h"
#include "Codec/H264Track.h"
#include "Hook/MediaHook.h"
#include "Webrtc.h"

using namespace std;

static shared_ptr<DtlsCertificate> g_dtlsCertificate;

void cloneTrack(const shared_ptr<WebrtcTrackInfo>& dstInfo, const shared_ptr<TrackInfo>& srcInfo)
{
    dstInfo->codec_ = srcInfo->codec_;
    dstInfo->trackType_ = srcInfo->trackType_;
}

WebrtcContext::WebrtcContext()
{
    _dtlsSession.reset(new DtlsSession("server"));
	if (!_dtlsSession->init(g_dtlsCertificate)) {
		logError << "dtls session init failed";
    }

    _timeClock.start();
    _lastPktClock.start();
}

WebrtcContext::~WebrtcContext()
{
    // WebrtcContextManager::instance()->delContext(_username);
    auto rtcSrc = _source.lock();
    
    if (!_isPlayer && rtcSrc) {
        rtcSrc->release();
        rtcSrc->delConnection(this);
        // _source->delOnDetach(this);
    } else if (rtcSrc) {
        rtcSrc->delConnection(this);
        // _source->delOnDetach(this);
    }

    if (_playReader) {
        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_WEBRTC;
        info.status = "off";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void WebrtcContext::initDtlsCert()
{
    if (!g_dtlsCertificate) {
        g_dtlsCertificate = make_shared<DtlsCertificate>();
        g_dtlsCertificate->init();
    }
}

string WebrtcContext::getFingerprint()
{
    return g_dtlsCertificate->getFingerprint();
}

shared_ptr<DtlsCertificate> WebrtcContext::getDtlsCertificate()
{
    return g_dtlsCertificate;
}

void WebrtcContext::initPlayer(const string& appName, const string& streamName, const string& sdp)
{
    _path = "/" + appName + "/" + streamName;
    auto source = MediaSource::get(_path, DEFAULT_VHOST);
    if (!source) {
        throw runtime_error("source is not exist");
    }

    auto mapTrackInfo = source->getTrackInfo();
    shared_ptr<TrackInfo> videoInfo;
    shared_ptr<TrackInfo> audioInfo;
    int trackNum = 0;
    for (auto iter : mapTrackInfo) {
        ++trackNum;
        if (iter.second->trackType_ == "video") {
            videoInfo = iter.second;
        } else {
            audioInfo = iter.second;
        }
    }

    if (!videoInfo || videoInfo->codec_ != "h264") {
        throw runtime_error("only surpport h264 now");
    }

    _remoteSdp = make_shared<WebrtcSdp>();
    _remoteSdp->parse(sdp);

    initPlayerLocalSdp(trackNum);

    negotiatePlayValid(videoInfo, audioInfo);

    if (_enbaleDtls) {
        weak_ptr<WebrtcContext> wSelf = shared_from_this();
        _dtlsSession->setOnHandshakeDone([wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->startPlay();    
            }
        });

        _dtlsSession->setOnRecvApplicationData([wSelf](const char* data, int len){
            auto self = wSelf.lock();
            if (self) {
                self->onRecvDtlsApplicationData(data, len);    
            }
        });
    }
}

void WebrtcContext::initPublisher(const string& appName, const string& streamName, const string& sdp)
{
    _path = "/" + appName + "/" + streamName;
    _urlParser.path_ = _path;
    _urlParser.vhost_ = DEFAULT_VHOST;
    _urlParser.protocol_ = PROTOCOL_WEBRTC;
    _urlParser.type_ = DEFAULT_TYPE;
    auto source = MediaSource::getOrCreate(_path, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [this](){
        return make_shared<WebrtcMediaSource>(_urlParser, _loop);
    });

    if (!source) {
        throw runtime_error("source is exist");
    }

    auto rtcSource = dynamic_pointer_cast<WebrtcMediaSource>(source);
    if (!rtcSource) {
        throw runtime_error("source is exist");
    }
    _source = rtcSource;
    _isPlayer = false;
    rtcSource->setOrigin();

    // read config
    shared_ptr<TrackInfo> videoInfo = make_shared<H264Track>();
    shared_ptr<TrackInfo> audioInfo = make_shared<TrackInfo>();
    
    videoInfo->codec_ = "h264";
    audioInfo->codec_ = "opus";

    _remoteSdp = make_shared<WebrtcSdp>();
    _remoteSdp->parse(sdp);

    initPlayerLocalSdp(0);

    negotiatePlayValid(videoInfo, audioInfo);

    weak_ptr<WebrtcContext> wSelf = shared_from_this();
    _dtlsSession->setOnHandshakeDone([wSelf](){
        logInfo << "start to publish";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (self->_enbaleSrtp && !self->_srtpSession) {
            self->_srtpSession.reset(new SrtpSession());
            std::string recv_key, send_key;
            if (0 != self->_dtlsSession->getSrtpKey(recv_key, send_key)) {
                logError << "dtls get srtp key failed";
            throw runtime_error("dtls get srtp key failed");
            }

            if (!self->_srtpSession->init(recv_key, send_key)) {
                logError << "srtp session init failed";
            throw runtime_error("srtp session init failed");
            }

            self->_hasInitSrtp = true;
        }
    });
}

void WebrtcContext::initPlayerLocalSdp(int trackNum)
{
    _localSdp = make_shared<WebrtcSdp>();
    _localSdp->_title = make_shared<WebrtcSdpTitle>();
    // v
    _localSdp->_title->version_ = _remoteSdp->_title->version_;

    // o
    _localSdp->_title->username_ = "SimpleMediaServer";
    _localSdp->_title->sessionId_ = "3259346588694";
    _localSdp->_title->sessionVersion_ = _remoteSdp->_title->sessionVersion_;
    _localSdp->_title->netType_ = _remoteSdp->_title->netType_;
    _localSdp->_title->addrType_ = _remoteSdp->_title->addrType_;
    _localSdp->_title->addr_ = _remoteSdp->_title->addr_;

    // s
    _localSdp->_title->sessionName_ = "play_connection";

    // t
    _localSdp->_title->startTime_ = _remoteSdp->_title->startTime_;
    _localSdp->_title->endTime_ = _remoteSdp->_title->endTime_;

    // a
    _localSdp->_title->groupPolicy_ = _remoteSdp->_title->groupPolicy_;
    _localSdp->_title->msidSemantic_ = _remoteSdp->_title->msidSemantic_;
    _localSdp->_title->msids_.emplace_back(_path);

    // a ice
    // _localSdp->_title->fingerprintAlg_ = "sha-256";
    // _localSdp->_title->fingerprint_ = g_dtlsCertificate->getFingerprint();
    // _localSdp->_title->iceOptions_ = _remoteSdp->_title->iceOptions_.empty() ? "trickle" : _remoteSdp->_title->iceOptions_;
    // _localSdp->_title->iceUfrag_ = randomString(8);
    // _localSdp->_title->icePwd_ = randomString(8);
    // _localSdp->_title->setup_ = "passive";
    _localSdp->_title->iceRole_ = "ice-lite";

    _iceUfrag = randomString(8);
    _icePwd = randomString(32);

    string remoteIceUfrag;

    if (_remoteSdp->_title->iceUfrag_.empty()) {
        auto iter = _remoteSdp->_vecSdpMedia.begin();
        if (iter != _remoteSdp->_vecSdpMedia.end()) {
            remoteIceUfrag = (*iter)->iceUfrag_;
        }
    } else {
        remoteIceUfrag = _remoteSdp->_title->iceUfrag_;
    }

    _username = _iceUfrag + ":" + remoteIceUfrag;

    WebrtcContextManager::instance()->addContext(_username, shared_from_this());
}

void WebrtcContext::negotiatePlayValid(const shared_ptr<TrackInfo>& videoInfo, const shared_ptr<TrackInfo>& audioInfo)
{
    int videoTrackNum = 0;
    WebrtcPtInfo::Ptr remotePtInfo;

    static bool enableNack = Config::instance()->getAndListen([](const json& config){
        enableNack = Config::instance()->get("Webrtc", "Server", "Server1", "enableNack");
    }, "Webrtc", "Server", "Server1", "enableNack");

    static bool enableTwcc = Config::instance()->getAndListen([](const json& config){
        enableTwcc = Config::instance()->get("Webrtc", "Server", "Server1", "enableTwcc");
    }, "Webrtc", "Server", "Server1", "enableTwcc");

    static bool enableRtx = Config::instance()->getAndListen([](const json& config){
        enableRtx = Config::instance()->get("Webrtc", "Server", "Server1", "enableRtx");
    }, "Webrtc", "Server", "Server1", "enableRtx");

    static bool enableRed = Config::instance()->getAndListen([](const json& config){
        enableRed = Config::instance()->get("Webrtc", "Server", "Server1", "enableRed");
    }, "Webrtc", "Server", "Server1", "enableRed");

    static bool enableUlpfec = Config::instance()->getAndListen([](const json& config){
        enableUlpfec = Config::instance()->get("Webrtc", "Server", "Server1", "enableUlpfec");
    }, "Webrtc", "Server", "Server1", "enableUlpfec");
    
    for (auto sdpMedia : _remoteSdp->_vecSdpMedia) {
        shared_ptr<WebrtcSdpMedia> localSdpMedia = make_shared<WebrtcSdpMedia>();
        shared_ptr<WebrtcTrackInfo> trackInfo = make_shared<WebrtcTrackInfo>();

        int remoteTwccId = 0;
        auto extIter = sdpMedia->mapExtmap_.find(TWCCUrl);
        if (extIter != sdpMedia->mapExtmap_.end()) {
            remoteTwccId = extIter->second;
        }

        if (sdpMedia->media_ == "video") {
            ++videoTrackNum;
            bool first = true;
            bool isH264 = true;
            if (videoInfo) {
                isH264 = videoInfo->codec_ == "h264";
            }

            // cloneTrack(trackInfo, videoInfo);

            for (auto ptIter : sdpMedia->mapPtInfo_) {
                if (videoInfo && strcasecmp(ptIter.second->codec_.data(), videoInfo->codec_.data()) == 0) {
                    if (isH264) {
                        if (first) {
                            remotePtInfo = ptIter.second;
                            first = false;
                        }
                        if (ptIter.second->fmtp_.find("42e01f") != string::npos) {
                            remotePtInfo = ptIter.second;
                            break;
                        }
                    } else {
                        remotePtInfo = ptIter.second;
                        break;
                    }
                } else {
                    if (ptIter.second->fmtp_.find("42e01f") != string::npos) {
                        remotePtInfo = ptIter.second;
                        break;
                    }
                }
            }

            if (enableTwcc) {
                auto iter = sdpMedia->mapExtmap_.find(TWCCUrl);
                if (iter != sdpMedia->mapExtmap_.end()) {
                    // rtp 扩展头里的id，是从extmap里获取的
                    rtpExtTypeMap.addId(iter->second, iter->first);
                }
            }

            if (enableRtx) {
                auto iter = sdpMedia->mapExtmap_.find(RtpStreamIdUrl);
                if (iter != sdpMedia->mapExtmap_.end()) {
                    // rtp 扩展头里的id，是从extmap里获取的
                    rtpExtTypeMap.addId(iter->second, iter->first);
                }
                
                iter = sdpMedia->mapExtmap_.find(RepairedRtpStreamIdUrl);
                if (iter != sdpMedia->mapExtmap_.end()) {
                    // rtp 扩展头里的id，是从extmap里获取的
                    rtpExtTypeMap.addId(iter->second, iter->first);
                }
            }

            for (auto ptIter : sdpMedia->mapPtInfo_) {
                if (enableRtx && ptIter.second->codec_ == "rtx" && ptIter.second->payloadType_ == remotePtInfo->rtxPt_) {
                    _videoRtxPtInfo = ptIter.second;
                }

                if (enableRed && ptIter.second->codec_ == "red") {
                    _videoRedPtInfo = ptIter.second;
                }

                if (enableUlpfec && ptIter.second->codec_ == "ulpfec") {
                    _videoUlpfecPtInfo = ptIter.second;
                }
            }

            _videoPtInfo = remotePtInfo;
        } else if (sdpMedia->media_ == "audio") {
            if (!videoTrackNum) {
                _videoFirst = false;
            }
            bool first = true;

            // cloneTrack(trackInfo, audioInfo);
            WebrtcPtInfo::Ptr opusPtInfo;
            bool findFlag = false;

            for (auto ptIter : sdpMedia->mapPtInfo_) {
                if (first) {
                    remotePtInfo = ptIter.second;
                    first = false;
                }
                if (audioInfo && ((strcasecmp(ptIter.second->codec_.data(), audioInfo->codec_.data()) == 0) || 
                    (audioInfo->codec_ == "g711a" && ptIter.second->codec_ == "PCMA"))) {
                    remotePtInfo = ptIter.second;
                    findFlag = true;
                    break;
                }
                if (ptIter.second->codec_ == "opus") {
                    opusPtInfo = ptIter.second;
                }
            }

            for (auto ptIter : sdpMedia->mapPtInfo_) {
                if (enableRtx && ptIter.second->codec_ == "rtx") {
                    _audioRtxPtInfo = ptIter.second;
                }

                if (enableRed && ptIter.second->codec_ == "red") {
                    _audioRedPtInfo = ptIter.second;
                }

                if (enableUlpfec && ptIter.second->codec_ == "ulpfec") {
                    _audioUlpfecPtInfo = ptIter.second;
                }
            }

            if (!findFlag && opusPtInfo) {
                remotePtInfo = opusPtInfo;
            }

            _audioPtInfo = remotePtInfo;
        } else {
            return ;
        }

        if (remotePtInfo) {
            localSdpMedia->mapPtInfo_.emplace(remotePtInfo->payloadType_, remotePtInfo);
            localSdpMedia->media_ = sdpMedia->media_;
            localSdpMedia->port_ = sdpMedia->port_;
            localSdpMedia->protocol_ = sdpMedia->protocol_;
            localSdpMedia->mid_ = sdpMedia->mid_;
            localSdpMedia->mapExtmap_ = sdpMedia->mapExtmap_;
            if (!_isPlayer) {
                if (sdpMedia->mapSsrc_.find("origin") == sdpMedia->mapSsrc_.end()) {
                    remotePtInfo->ssrc_ = sdpMedia->mapSsrcInfo_.begin()->first;
                } else {
                    remotePtInfo->ssrc_ = sdpMedia->mapSsrc_["origin"];
                }
            }
            
            auto ssrcInfo = make_shared<SsrcInfo>();
            if (sdpMedia->media_ == "video") {
                ssrcInfo->ssrc_ = _isPlayer ? 20000 : remotePtInfo->ssrc_;//(!videoInfo || videoInfo->trackType_.empty()) ? 0 : 20000;
                if (_videoRtxPtInfo) {
                    localSdpMedia->mapPtInfo_.emplace(_videoRtxPtInfo->payloadType_, _videoRtxPtInfo);
                    
                    auto rtxSsrcInfo = make_shared<SsrcInfo>();
                    rtxSsrcInfo->ssrc_ = _isPlayer ? 20001 : sdpMedia->mapSsrc_["rtx"];
                    rtxSsrcInfo->cname_ = _path;
                    localSdpMedia->mapSsrcInfo_.emplace(rtxSsrcInfo->ssrc_, rtxSsrcInfo);
                    _videoRtxPtInfo->ssrc_ = rtxSsrcInfo->ssrc_;

                    localSdpMedia->mapSsrcGroup_["FID"].push_back(ssrcInfo->ssrc_);
                    localSdpMedia->mapSsrcGroup_["FID"].push_back(rtxSsrcInfo->ssrc_);
                }
                if (_videoRedPtInfo) {
                    localSdpMedia->mapPtInfo_.emplace(_videoRedPtInfo->payloadType_, _videoRedPtInfo);
                }
                if (_videoUlpfecPtInfo) {
                    localSdpMedia->mapPtInfo_.emplace(_videoUlpfecPtInfo->payloadType_, _videoUlpfecPtInfo);
                }
            } else {
                ssrcInfo->ssrc_ = _isPlayer ? 10000 : remotePtInfo->ssrc_;//(!audioInfo || audioInfo->trackType_.empty()) ? 0 : 10000;
            }
            ssrcInfo->cname_ = _path;
            localSdpMedia->mapSsrcInfo_.emplace(ssrcInfo->ssrc_, ssrcInfo);
            remotePtInfo->ssrc_ = ssrcInfo->ssrc_;

            switch (sdpMedia->sendRecvType_)
            {
            case SendOnly:
                localSdpMedia->sendRecvType_ = RecvOnly;
                break;
            case RecvOnly:
                localSdpMedia->sendRecvType_ = SendOnly;
                break;
            case SendRecv:
                localSdpMedia->sendRecvType_ = SendRecv;
                break;
            case Inactive:
                localSdpMedia->sendRecvType_ = Inactive;
                break;
            
            default:
                break;
            }

            localSdpMedia->fingerprintAlg_ = "sha-256";
            localSdpMedia->fingerprint_ = g_dtlsCertificate->getFingerprint();
            localSdpMedia->iceOptions_ = sdpMedia->iceOptions_.empty() ? "trickle" : _remoteSdp->_title->iceOptions_;
            localSdpMedia->iceUfrag_ = _iceUfrag;
            localSdpMedia->icePwd_ = _icePwd;
            localSdpMedia->setup_ = "passive";

            localSdpMedia->rtcpMux_ = sdpMedia->rtcpMux_;
            localSdpMedia->rtcpRsize_ = sdpMedia->rtcpRsize_;

            _localSdp->_title->groups_.emplace_back(localSdpMedia->mid_);
            _localSdp->_vecSdpMedia.push_back(localSdpMedia);
        }
    }

    if (_remoteSdp->_dataChannelSdp) {
        static int channelPort = Config::instance()->getAndListen([](const json &config){
            channelPort = Config::instance()->get("Webrtc", "Server", "Server1", "channelPort");
        }, "Webrtc", "Server", "Server1", "channelPort");
        
        _peerChannelPort = _remoteSdp->_dataChannelSdp->channelPort_;
        _localChannelPort = channelPort;

        _localSdp->_dataChannelSdp = _remoteSdp->_dataChannelSdp;
        _localSdp->_dataChannelSdp->channelPort_ = channelPort;
    }

    static string candidateIp = Config::instance()->getAndListen([](const json &config){
        candidateIp = Config::instance()->get("Webrtc", "Server", "Server1", "candidateIp");
    }, "Webrtc", "Server", "Server1", "candidateIp");

    static int port = Config::instance()->getAndListen([](const json &config){
        port = Config::instance()->get("Webrtc", "Server", "Server1", "port");
    }, "Webrtc", "Server", "Server1", "port");

    static int enableTcp = Config::instance()->getAndListen([](const json &config){
        enableTcp = Config::instance()->get("Webrtc", "Server", "Server1", "enableTcp");
    }, "Webrtc", "Server", "Server1", "enableTcp");

    if (enableTcp) {
        auto candidate = make_shared<CandidateInfo>();
        candidate->foundation_ = "0";
        candidate->ip_ = candidateIp;
        candidate->port_ = port;
        candidate->priority_ = 223456;
        candidate->candidateType_ = "host";
        candidate->transType_ = "tcp";

        _localSdp->addCandidate(candidate);
    } else {

        auto candidateUdp = make_shared<CandidateInfo>();
        candidateUdp->foundation_ = "1";
        candidateUdp->ip_ = candidateIp;
        candidateUdp->port_ = port;
        candidateUdp->priority_ = 123456;
        candidateUdp->candidateType_ = "host";
        candidateUdp->transType_ = "udp";
        _localSdp->addCandidate(candidateUdp);
    }
}

bool WebrtcContext::isAlive()
{
    return _alive;
}

void WebrtcContext::onManager()
{
    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Webrtc", "Server", "Server1", "timeout");
    }, "Webrtc", "Server", "Server1", "timeout");

    if (timeout == 0) {
        timeout = 5000;
    }

    logInfo << "_lastPktClock.startToNow(): " << _lastPktClock.startToNow();
    logInfo << "timeout: " << timeout;
    if (_lastPktClock.startToNow() > timeout) {
        close();
    }
}

void WebrtcContext::checkAndSendRtcpNack()
{
    vector<uint16_t> videoVecSeq = _videoSort->getLossSeq();
    vector<uint16_t> audioVecSeq = _audioSort->getLossSeq();

    if (!videoVecSeq.empty()) {
        RtcpNack videoNack;
        videoNack.setSsrc(_videoPtInfo->ssrc_);
        videoNack.setLossSn(videoVecSeq);
        auto videoNackBuffer = videoNack.encode();
        auto bufferRtcp = videoNackBuffer;

        if (_enbaleSrtp) {
            char plaintext[1500];
            int nb_plaintext = videoNackBuffer->size();
            auto data = videoNackBuffer->data();
            if (0 != _srtpSession->protectRtcp(data, plaintext, nb_plaintext)) {
                close();
                return ;
            }

            bufferRtcp = StreamBuffer::create();
            bufferRtcp->assign(plaintext, nb_plaintext);
        }

        logInfo << "_socket->send(videoNackBuffer); " << bufferRtcp->size();
        if (_socket->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = bufferRtcp->size() >> 8;
            payload_ptr[1] = bufferRtcp->size() & 0x00FF;

            _socket->send((char*)payload_ptr, 2);

            _socket->send(bufferRtcp);
        } else {
            _socket->send(bufferRtcp, 1, 0, 0, _addr, _addrLen);
        }
    }

    if (!audioVecSeq.empty()) {
        RtcpNack audioNack;
        audioNack.setSsrc(_audioPtInfo->ssrc_);
        audioNack.setLossSn(audioVecSeq);
        auto audioNackBuffer = audioNack.encode();
        auto bufferRtcp = audioNackBuffer;

        if (_enbaleSrtp) {
            char plaintext[1500];
            int nb_plaintext = audioNackBuffer->size();
            auto data = audioNackBuffer->data();
            if (0 != _srtpSession->protectRtcp(data, plaintext, nb_plaintext)) {
                close();
                return ;
            }

            bufferRtcp = StreamBuffer::create();
            bufferRtcp->assign(plaintext, nb_plaintext);
        }

        logInfo << "_socket->send(audioNackBuffer);" << bufferRtcp->size();
        if (_socket->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = bufferRtcp->size() >> 8;
            payload_ptr[1] = bufferRtcp->size() & 0x00FF;

            _socket->send((char*)payload_ptr, 2);

            _socket->send(bufferRtcp);
        } else {
            _socket->send(bufferRtcp, 1, 0, 0, _addr, _addrLen);
        }
    }
}

void WebrtcContext::sendRtcpPli(int ssrc)
{
    logInfo << "send a rtcp pli";
    RtcpPli pli;
    auto buffer = pli.encode(ssrc);
    auto bufferRtcp = buffer;

    if (_enbaleSrtp) {
        char plaintext[1500];
        int nb_plaintext = buffer->size();
        auto data = buffer->data();
        if (0 != _srtpSession->protectRtcp(data, plaintext, nb_plaintext)) {
            close();
            return ;
        }

        bufferRtcp = StreamBuffer::create();
        bufferRtcp->assign(plaintext, nb_plaintext);
    }

    if (_socket->getSocketType() == SOCKET_TCP) {
        uint8_t payload_ptr[2];
        payload_ptr[0] = bufferRtcp->size() >> 8;
        payload_ptr[1] = bufferRtcp->size() & 0x00FF;

        _socket->send((char*)payload_ptr, 2);
    }

    _socket->send(bufferRtcp, 1, 0, 0, _addr, _addrLen);
}

void WebrtcContext::onRtpPacket(const Socket::Ptr& socket, const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len)
{
    if (!_srtpSession || !_hasInitSrtp) {
        return ;
    }
    // logInfo << "get a rtp packet: " << rtp->size();
    _lastPktClock.update();

    // 如果是rtx，并且有padding，认为是探测包，不处理；判断方式是否正确??
    if (_videoRtxPtInfo && rtp->getSSRC() == _videoRtxPtInfo->ssrc_ && rtp->getHeader()->padding) {
        return ;
    }

    static int lossIntervel = Config::instance()->getAndListen([](const json &config){
        lossIntervel = Config::instance()->get("Webrtc", "Server", "Server1", "lossIntervel");
    }, "Webrtc", "Server", "Server1", "lossIntervel");

    // 模拟丢包
    if (lossIntervel && rtp->getSSRC() == _videoPtInfo->ssrc_ && _totalRtpCnt++ % lossIntervel == 0) {
        logInfo << "loss a seq: " << rtp->getSeq();
        return ;
    }

    // rtx 包的payload前两个字节，表示源rtp的序号
    // rtx 包的时间戳与源rtp一致

    uint32_t ssrc = rtp->getSSRC();
    uint32_t sdpSsrc = _videoPtInfo->ssrc_;
    uint rtxSsrc = _videoRtxPtInfo ? _videoRtxPtInfo->ssrc_ : 0;
    // logInfo << "rtp ssrc: " << ssrc << ", sdp ssrc: " << sdpSsrc << ", rtx ssrc: " << rtxSsrc;
    weak_ptr<WebrtcContext> wSelf = dynamic_pointer_cast<WebrtcContext>(shared_from_this());
    if (ssrc == sdpSsrc && !_rtcpPliTimerCreated) {
        _loop->addTimerTask(2000, [wSelf, ssrc](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }

            self->sendRtcpPli(ssrc);

            return 2000;
        }, nullptr);

        if (!_isPlayer) {
            _loop->addTimerTask(50, [wSelf, ssrc](){
                auto self = wSelf.lock();
                if (!self) {
                    return 0;
                }

                self->checkAndSendRtcpNack();

                return 50;
            }, nullptr);
        }

        _rtcpPliTimerCreated = true;
    }

    WebrtcRtpPacket::Ptr rtpPacket;

    if (_enbaleSrtp) {
        char plaintext[1500];
        int nb_plaintext = rtp->size();
        auto data = rtp->data();
        if (0 != _srtpSession->unprotectRtp(data, plaintext, nb_plaintext)) 
            return ;

        auto rtpbuffer = StreamBuffer::create();
        rtpbuffer->assign(plaintext, nb_plaintext);

        rtpPacket = make_shared<WebrtcRtpPacket>(rtpbuffer);
    } else {
        rtpPacket = dynamic_pointer_cast<WebrtcRtpPacket>(rtp);
    }

    // rtx 包
    if (_videoRtxPtInfo && rtpPacket->getSSRC() == _videoRtxPtInfo->ssrc_) {
        // 转换为正常包，方便后续的转发
        rtpPacket->setRtxFlag(true);
        rtpPacket->getHeader()->pt = _videoPtInfo->payloadType_;
        rtpPacket->getHeader()->seq = htons(rtpPacket->getSeq());
        rtpPacket->getHeader()->ssrc = htonl(_videoPtInfo->ssrc_);
        memmove(rtpPacket->data() + 2, rtpPacket->data(), (char*)rtpPacket->getHeader()->getPayloadData() - rtpPacket->data());
        rtpPacket->buffer()->substr(2);
        rtpPacket->setRtxFlag(false);
        rtpPacket->resetHeader();
        logInfo << "get a resend packet: " << rtpPacket->getSeq();
    }

    if (rtp->getHeader()->pt == _videoPtInfo->payloadType_ 
            || (_videoRtxPtInfo && rtp->getHeader()->pt == _videoRtxPtInfo->payloadType_)) {
        logTrace << "decode video rtp";
        if (_socket->getSocketType() == SOCKET_TCP) {
            _videoDecodeTrack->onRtpPacket(rtpPacket);
        } else {
            _videoSort->inputRtp(rtpPacket);
        }
    } else if (rtp->getHeader()->pt == _audioPtInfo->payloadType_) {
        if (_socket->getSocketType() == SOCKET_TCP) {
            _audioDecodeTrack->onRtpPacket(rtpPacket);
        } else {
            _audioSort->inputRtp(rtpPacket);
        }
    } else {
        // logWarn << "videoDecodeTrack is empty";
    }
}

void WebrtcContext::onStunPacket(const Socket::Ptr& socket, const WebrtcStun& stunReq, struct sockaddr* addr, int len)
{
    _lastPktClock.update();

    if (!stunReq.isBindingRequest()) {
		logError << "only handle stun bind request";
		return ;
	}

    if (!_addr && addr) {
        _socket = socket;
	    _addrLen = len;
        _addr = (struct sockaddr*)malloc(len);
	    memcpy(_addr, addr, len);

        _loop = socket->getLoop();
        weak_ptr<WebrtcContext> wSelf = dynamic_pointer_cast<WebrtcContext>(shared_from_this());

        if (!_isPlayer) {
            auto rtcSrc = _source.lock();
            if (rtcSrc) {
                rtcSrc->setLoop(_loop);
            }
            
            for (auto sdpMediaIter : _localSdp->_vecSdpMedia) {
                if (sdpMediaIter->media_ == "video") {
                    _videoDecodeTrack = make_shared<WebrtcDecodeTrack>(VideoTrackType, VideoTrackType, _videoPtInfo/*sdpMediaIter->mapPtInfo_.begin()->second*/);
                    rtcSrc->addTrack(_videoDecodeTrack);
                } else if (sdpMediaIter->media_ == "audio") {
                    _audioDecodeTrack = make_shared<WebrtcDecodeTrack>(AudioTrackType, AudioTrackType, _audioPtInfo/*sdpMediaIter->mapPtInfo_.begin()->second*/);
                    if (_audioDecodeTrack->getTrackInfo()) {
                        rtcSrc->addTrack(_audioDecodeTrack);
                    }
                }
            }

            _videoSort = make_shared<RtpSort>(25);
            _videoSort->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp){
                // logInfo << "decode rtp seq: " << rtp->getSeq() << ", rtp size: " << rtp->size() << ", rtp time: " << rtp->getStamp();
                auto self = wSelf.lock();
                if (self) {
                    self->_videoDecodeTrack->onRtpPacket(rtp);
                }
            });

            _audioSort = make_shared<RtpSort>(25);
            _audioSort->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp){
                // logInfo << "decode rtp seq: " << rtp->getSeq() << ", rtp size: " << rtp->size() << ", rtp time: " << rtp->getStamp();
                auto self = wSelf.lock();
                if (self) {
                    self->_audioDecodeTrack->onRtpPacket(rtp);
                }
            });
        }

        _loop->addTimerTask(2000, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }

            self->onManager();

            return 2000;
        }, nullptr);
    }

        

	WebrtcStun stunRsp;
	stunRsp.setType(BindingResponse);
	stunRsp.setLocalUfrag(stunReq.getRemoteUfrag());
    stunRsp.setRemoteUfrag(stunReq.getLocalUfrag());
    stunRsp.setTranscationId(stunReq.getTranscationId());

	// struct sockaddr_in* peer_addr = (struct sockaddr_in*)_addr;
    if (_addr->sa_family == AF_INET) {
	    stunRsp.setMappedAddress(_addr);
    } else {
        stunRsp.setMappedAddress(_addr);
    }
	// stunRsp.setMappedPort(ntohs(peer_addr->sin_port));

    // char buf[kRtpPacketSize];
    // SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    // SrsAutoFree(SrsBuffer, stream);

    auto buffer = make_shared<StringBuffer>();
    // char buf[1500];
    // SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    // SrsAutoFree(SrsBuffer, stream);
	if (0 != stunRsp.toBuffer(_icePwd, buffer)) {
		logError << "encode stun response failed";
		return ;
	}

    logInfo << "send a stun responce";

    if (socket->getSocketType() == SOCKET_TCP) {
        uint8_t payload_ptr[2];
        payload_ptr[0] = buffer->size() >> 8;
        payload_ptr[1] = buffer->size() & 0x00FF;

        socket->send((char*)payload_ptr, 2);
    }
    
	socket->send(buffer->data(), buffer->size(), 1, _addr, len);
	_lastRecvTime = time(nullptr);

    if (!_enbaleDtls) {
        startPlay(); 
    }
}

void WebrtcContext::onDtlsPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    _lastPktClock.update();

    // cerr << "WebrtcContext::onDtlsPacket =============== ";
    _dtlsSession->onDtls(socket, buffer, addr, len);

    if (!_addr && addr) {
        _socket = socket;
	    _addrLen = len;
        _addr = (struct sockaddr*)malloc(len);
	    memcpy(_addr, addr, len);

        _loop = socket->getLoop();
    }

    // cerr << "WebrtcContext::_srtpSession =============== " << _srtpSession;
}

void WebrtcContext::onRtcpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    if (!_srtpSession || !_hasInitSrtp) {
        logInfo << "_srtpSession is empty";
        return ;
    }
    _lastPktClock.update();

    // logInfo << "WebrtcContext::onRtcpPacket =============== " << buffer->size();
    if (!_addr && addr) {
        _socket = socket;
	    _addrLen = len;
        _addr = (struct sockaddr*)malloc(len);
	    memcpy(_addr, addr, len);

        _loop = socket->getLoop();
    }

	nackHeartBeat();

	_lastRecvTime = time(nullptr);

    if (_enbaleSrtp) {
        char plaintext[1500];
        int nb_plaintext = buffer->size();
        auto data = buffer->data();
        if (0 != _srtpSession->unprotectRtcp(data, plaintext, nb_plaintext)) 
            return ;

        handleRtcp(plaintext, nb_plaintext);
    } else {
        handleRtcp(buffer->data(), buffer->size());
    }
}

void WebrtcContext::nackHeartBeat()
{
	uint32_t heartbeatTime = 10;
	int sec = _timeClock.startToNow() / 1000;
    if (sec >= heartbeatTime) {
		uint64_t sendRtpPack_10s = _sendRtpPack_10s;
		uint64_t sendRtcpNackPack_10s = _sendRtcpNackPack_10s;
		uint64_t rtpLoss_10s = _rtpLoss_10s;
		uint64_t resendRtpPack_10s = _resendRtpPack_10s;
		uint64_t send = sendRtpPack_10s - resendRtpPack_10s;

		_lossPercent = rtpLoss_10s == 0 ? 0 : rtpLoss_10s * 1.0 / send;

		logInfo << " nack heartbeat : streamname " << _path << " : === rtp lose percent : "
                << _lossPercent << " === ; sendRtpPack_10s : " << send << " ; rtpLoss_10s : "
                << rtpLoss_10s << " ; resendRtpPack_10s : " << resendRtpPack_10s;

		_sendRtcpNackPack_10s -= sendRtcpNackPack_10s;
		_rtpLoss_10s -= rtpLoss_10s;
		_resendRtpPack_10s -= resendRtpPack_10s;
		_sendRtpPack_10s -= sendRtpPack_10s;

		_timeClock.update();
	}
}

//收到Genetic RTP Feedback 报文,进行丢包重传
void WebrtcContext::handleRtcp(char* buf, int size)
{
    logInfo << "start WebrtcContext::handleRtcp";
    auto buffer = make_shared<StreamBuffer>();
    buffer->move(buf, size, 0);
    RtcpPacket rtcp(buffer, 0);

    rtcp.setOnRtcp([this](const RtcpPacket::Ptr &subRtcp){
        if (subRtcp->getHeader()->type == RtcpType_RTPFB && subRtcp->getHeader()->rc == RtcpRtpFBFmt_NACK) {
            ++_sendRtcpNackPack_10s;
            
            auto nackRtcp = dynamic_pointer_cast<RtcpNack>(subRtcp);
            nackRtcp->parse();
            auto nackId = nackRtcp->getLossPacket();
            for(auto id : nackId){
                ++_rtpLoss_10s;
                auto index = id % 256;
                if(_rtpCache[index]){
                    if(_rtpCache[index]->getSeq() == id) {
                        sendMedia(_rtpCache[index]);
                        _resendRtpPack_10s++;
                        if(_firstResend){
                            _firstResend = false;
                            logInfo <<"resend rtp success, stream is : " << _path << endl;
                        }
                    }
                }
            }
        } else if (subRtcp->getHeader()->type == RtcpType_RTPFB && subRtcp->getHeader()->rc == RtcpRtpFBFmt_TWCC) {
            auto nackRtcp = dynamic_pointer_cast<RtcpTWCC>(subRtcp);
            nackRtcp->parse();
            auto curSeq = nackRtcp->getFbPktCnt();
            // TODO lastSeq == curSeq + 1，判断是否丢包
            auto pktChunks = nackRtcp->getPktChunks();
            // TODO 判断是否拥塞阻塞了，进一步可以调整码率，帧率之类的操作
        }
    });

    rtcp.parse();
}

void WebrtcContext::sendMedia(const RtpPacket::Ptr& rtp)
{
    // if (rtp->type_ == "audio") {
    //     return ;
    // }

    if (_sctp && !_audioPtInfo && !_videoPtInfo) {
        sendDatachannel(0, 53, rtp->data(), rtp->size());

        return ;
    }

    int startSize = rtp->getStartSize();

    logInfo << "WebrtcContext::sendMedia: " << startSize << ", rtp size: " << rtp->size();
	int nb_cipher = rtp->size() - startSize;
    // char data[1500];
    auto buffer = make_shared<StreamBuffer>(1500 + 1);
    auto data = buffer->data();
    memcpy(data, rtp->data() + startSize, nb_cipher);

	// auto sdp_video_pt = 106;

	// auto _video_payload_type = sdp_video_pt == 0 ? 106 : sdp_video_pt;
	data[1] = (data[1] & 0x80) | (rtp->type_ == "audio" ? _audioPtInfo->payloadType_ : _videoPtInfo->payloadType_);
	uint32_t ssrc = htonl(rtp->type_ == "audio" ? 10000 : 20000);
	memcpy(data + 8, &ssrc, sizeof(ssrc));	

    // FILE* fp = fopen("test.rtp", "ab+");
	// fwrite(data, nb_cipher, 1, fp);
	// fclose(fp);

	if (_enbaleSrtp) {
        if (0 != _srtpSession->protectRtp(data, &nb_cipher)) {
            logWarn << "protect srtp failed";
            return ;
        }
		// lastest_packet_send_time_ = time(nullptr);
	}
    logInfo << "protect rtp size: " << nb_cipher;
    if (_socket->getSocketType() == SOCKET_TCP) {
        uint8_t payload_ptr[2];
        payload_ptr[0] = nb_cipher >> 8;
        payload_ptr[1] = nb_cipher & 0x00FF;

        _socket->send((char*)payload_ptr, 2);
    }
    _socket->send(buffer, 1, 0, nb_cipher, _addr, _addrLen);
    _sendRtpPack_10s++;
	// _bytes += nb_cipher;
}

void WebrtcContext::onRecvDtlsApplicationData(const char* data, int len)
{
    if (_sctp) {
        _sctp->ProcessSctpData((uint8_t*)data, len);
    }
}

void WebrtcContext::startPlay()
{
    weak_ptr<WebrtcContext> wSelf = dynamic_pointer_cast<WebrtcContext>(shared_from_this());
    if (_localSdp->_dataChannelSdp) {
        _sctp = std::make_shared<SctpAssociationImp>(_loop, _localChannelPort, _peerChannelPort, 128, 128, 262144, true);
        _sctp->TransportConnected();
        _sctp->setOnSctpAssociationConnecting([wSelf](SctpAssociation* sctpAssociation){

        });

        _sctp->setOnSctpAssociationConnected([wSelf](SctpAssociation* sctpAssociation){
            
        });

        _sctp->setOnSctpAssociationFailed([wSelf](SctpAssociation* sctpAssociation){
            
        });

        _sctp->setOnSctpAssociationClosed([wSelf](SctpAssociation* sctpAssociation){
            
        });

        _sctp->setOnSctpAssociationSendData([wSelf](SctpAssociation* sctpAssociation, const uint8_t* data, size_t len){
            auto self = wSelf.lock();
            if (self) {
                self->_dtlsSession->sendApplicationData(self->_socket, (char*)data, len);
            }
        });

        _sctp->setOnSctpAssociationMessageReceived([wSelf](SctpAssociation* sctpAssociation,
                                                            uint16_t streamId,
                                                            uint32_t ppid,
                                                            const uint8_t* msg,
                                                            size_t len){
            
        });
    }

    logInfo << "start init _srtpSession";
    if (_enbaleSrtp && !_srtpSession) {
		_srtpSession.reset(new SrtpSession());
		std::string recv_key, send_key;
		if (0 != _dtlsSession->getSrtpKey(recv_key, send_key)) {
			logError << "dtls get srtp key failed";
            throw runtime_error("dtls get srtp key failed");
		}

        if (!_srtpSession->init(recv_key, send_key)) {
            logError << "srtp session init failed";
            throw runtime_error("srtp session init failed");
        }

        _hasInitSrtp = true;
	}

    _urlParser.path_ = _path;
    _urlParser.vhost_ = DEFAULT_VHOST;
    _urlParser.protocol_ = PROTOCOL_WEBRTC;
    _urlParser.type_ = DEFAULT_TYPE;

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

            self->startPlay(src);
        }, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<WebrtcMediaSource>(self->_urlParser, nullptr, true);
    }, this);
}

// PPID 51: 文本string
// PPID 53: 二进制
void WebrtcContext::sendDatachannel(uint16_t streamId, uint32_t ppid, const char *msg, size_t len)
{
    if (_sctp) {
        SctpStreamParameters params;
        params.streamId = streamId;
        _sctp->SendSctpMessage(params, ppid, (uint8_t *)msg, len);
    }
}

void WebrtcContext::startPlay(const MediaSource::Ptr &src)
{
    auto rtcSrc = dynamic_pointer_cast<WebrtcMediaSource>(src);
    if (!rtcSrc) {
        return ;
    }

    _source = rtcSrc;

    if (!_playReader/* && _rtp_type != Rtsp::RTP_MULTICAST*/) {
        logInfo << "start play attach ring";
        weak_ptr<WebrtcContext> weak_self = shared_from_this();
        _playReader = rtcSrc->getRing()->attach(_loop, true);
        _playReader->setGetInfoCB([weak_self]() {
            auto self = weak_self.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->_socket->getLocalIp();
            ret.port_ = self->_socket->getLocalPort();
            ret.protocol_ = PROTOCOL_WEBRTC;
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
        _playReader->setReadCB([weak_self](const WebrtcMediaSource::DataType &pack) {
            auto self = weak_self.lock();
            if (!self/* || pack->empty()*/) {
                return;
            }
            
            self->sendRtpPacket(pack);
        });

        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_WEBRTC;
        info.status = "on";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void WebrtcContext::changeLoop(const EventLoop::Ptr& loop)
{
    // 切换线程，再开始发送数据
    _playReader = nullptr;
    _loop = loop;
    if (_isPlayer) {
        auto src = _source.lock();
        if (src) {
            startPlay(src);
        }
    }
}

void WebrtcContext::close()
{
    logInfo << "close webrtc context";
    _alive = false;
    if (_addr) {
        auto hash = ntohl(((sockaddr_in*)_addr)->sin_addr.s_addr) << 32 + ntohs(((sockaddr_in*)_addr)->sin_port);
        WebrtcContextManager::instance()->delContext(hash);
    }
    WebrtcContextManager::instance()->delContext(_username);
}

string WebrtcContext::getLocalSdp() 
{
    if (_localSdp) {
        return _localSdp->getSdp();
    }

    return "";
}

void WebrtcContext::sendRtpPacket(const WebrtcMediaSource::DataType &pack)
{
    if (!_srtpSession || !_hasInitSrtp) {
        logInfo << "_srtpSession is empty";
        return ;
    }
    int i = 0;
    int len = pack->size() - 1;
    auto pktlist = *(pack.get());

    for (auto& packet: pktlist) {
        int index = packet->getSeq() % 256;
        _rtpCache[index] = packet;

        sendMedia(packet);
    };
}