#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>
#include <sstream>
#include <iomanip>

#include "WebrtcClient.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "EventPoller/EventLoopPool.h"
#include "Common/Track.h"
#include "WebrtcContext.h"
#include "Webrtc.h"
#include "Codec/H264Track.h"
#include "Codec/AacTrack.h"
#include "Common/HookManager.h"

using namespace std;

WebrtcClient::WebrtcClient(MediaClientType type, const string& appName, const string& streamName)
{
    logInfo << "WebrtcClient";
    _request = (type == MediaClientType_Push ? "push" : "pull");
    _urlParser.path_ = "/" + appName + "/" + streamName;
    _urlParser.protocol_ = PROTOCOL_WEBRTC;
    _urlParser.vhost_ = DEFAULT_VHOST;
    _urlParser.type_ = DEFAULT_TYPE;

    _loop = EventLoop::getCurrentLoop();
}

WebrtcClient::~WebrtcClient()
{
    logInfo << "~WebrtcClient";
    auto srtSrc = _source.lock();
    if (_request == "pull" && srtSrc) {
        srtSrc->delConnection(this);
        srtSrc->release();
    } else if (srtSrc) {
        srtSrc->delConnection(this);
    }

    if (_dtlsTimeTask) {
        _dtlsTimeTask->quit = false;
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

        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPlayer(info);
        }
    }
}

void WebrtcClient::init()
{
    MediaClient::registerCreateClient("webrtc", [](MediaClientType type, const std::string &appName, const std::string &streamName){
        return make_shared<WebrtcClient>(type, appName, streamName);
    });
}

bool WebrtcClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
#ifdef ENABLE_HTTP
    _sourceUrl = url;
    _dtlsSession.reset(new DtlsSession("client"));
	if (!_dtlsSession->init(WebrtcContext::getDtlsCertificate())) {
		logError << "dtls session init failed";
        return false;
    }

    auto localSdp = getLocalSdp();
    logInfo << "local sdp : " << localSdp;

    // string apiUrl = "http://127.0.0.1:1985/rtc/v1/play/";
    // json value;
    // value["streamurl"] = url;
    // value["sdp"] = localSdp;
    // value["api"] = apiUrl;

    weak_ptr<WebrtcClient> wSelf = shared_from_this();
    _parser.setOnRtcPacket([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if(!self){
            return;
        }

        self->onRtcPacket(data + 2, len - 2);
    });

    shared_ptr<HttpClientApi> client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
    // client->addHeader("Content-Type", "application/json;charset=UTF-8");
    client->setMethod("GET");
    client->setContent(localSdp);
    client->setOnHttpResponce([wSelf, client](const HttpParser &parser){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        logDebug << "uri: " << parser._url;
        // logInfo << "status: " << parser._version;
        // logInfo << "method: " << parser._method;
        // logInfo << "_content: " << parser._content;
        if (parser._url != "201") {
            logInfo << "http error: " << parser._content;
            return ;
        }
        // try {
            // json value = json::parse(parser._content);
            self->setRemoteSdp(parser._content);
        // } catch (exception& ex) {
        //     logInfo << "json parse failed: " << ex.what() << ", content: " << parser._content;
        // }

        const_cast<shared_ptr<HttpClientApi> &>(client).reset();
    });
    logInfo << "connect to utl: " << url;
    if (client->sendHeader(localIp, localPort, url, timeout) != -1) {
        close();
        return false;
    }
#else
    logInfo << "not support http client";
    close();
#endif

    return true;
}

void WebrtcClient::stop()
{
    close();
}

void WebrtcClient::pause()
{

}

void WebrtcClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void WebrtcClient::addOnReady(void* key, const function<void()>& onReady)
{
    lock_guard<mutex> lck(_mtx);
    auto rtcSrc =_source.lock();
    if (rtcSrc) {
        rtcSrc->addOnReady(key, onReady);
        return ;
    }
    _mapOnReady.emplace(key, onReady);
}

void WebrtcClient::getProtocolAndType(string& protocol, MediaClientType& type)
{
    protocol = _urlParser.protocol_;
    type = _request == "pull" ? MediaClientType_Pull : MediaClientType_Push;
}

void WebrtcClient::close()
{
    if (_onClose) {
        _onClose();
    }
}

void WebrtcClient::onError(const string& err)
{
    logInfo << "get a err: " << err;
    close();
}

string WebrtcClient::getLocalSdp()
{
    if (_request == "pull") {
        initPuller();
    } else {
        initPusher();
    }

    if (_localSdp) {
        return _localSdp->getSdp();
    }

    return "";
}

void WebrtcClient::initPuller()
{
    // _path = "/" + appName + "/" + streamName;
    // _urlParser.path_ = _path;
    // _urlParser.vhost_ = DEFAULT_VHOST;
    // _urlParser.protocol_ = PROTOCOL_WEBRTC;
    // _urlParser.type_ = DEFAULT_TYPE;
    auto source = MediaSource::getOrCreate(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_,
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
    rtcSource->setOrigin();
    rtcSource->setOriginSocket(_socket);
    rtcSource->setAction(false);
    {
        lock_guard<mutex> lck(_mtx);
        for (auto &iter : _mapOnReady) {
            rtcSource->addOnReady(iter.first, iter.second);
        }
        _mapOnReady.clear();
    }

    // shared_ptr<TrackInfo> videoInfo = make_shared<H264Track>();
    // shared_ptr<TrackInfo> audioInfo = make_shared<TrackInfo>();

    // videoInfo->codec_ = "h264";
    // audioInfo->codec_ = "opus";

    stringstream ss;
    initLocalSdpTitle(ss, 2);
    initPullerLocalSdpMedia(ss);

    _localSdp = make_shared<WebrtcSdp>();
    _localSdp->parse(ss.str());

    // if (!_srtpSession) {
    //     _srtpSession.reset(new SrtpSession());
    //     std::string recv_key, send_key;
    //     if (0 != _dtlsSession->getSrtpKey(recv_key, send_key)) {
    //         logError << "dtls get srtp key failed";
    //         throw runtime_error("dtls get srtp key failed");
    //     }

    //     if (!_srtpSession->init(recv_key, send_key)) {
    //         logError << "srtp session init failed";
    //         throw runtime_error("srtp session init failed");
    //     }
    // }
}

void WebrtcClient::initPusher()
{
    auto source = MediaSource::get(_urlParser.path_, DEFAULT_VHOST);
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

    // if (!videoInfo || videoInfo->codec_ != "h264") {
    //     throw runtime_error("only surpport h264 now");
    // }

    stringstream ss;
    initLocalSdpTitle(ss, trackNum);
    initPusherLocalSdpMedia(ss, videoInfo, audioInfo);

    _localSdp = make_shared<WebrtcSdp>();
    _localSdp->parse(ss.str());
}

void WebrtcClient::initLocalSdpTitle(stringstream& ss, int trackNum)
{
    string groupAttr = "a=group:BUNDLE";
    for (int i = 0; i < trackNum; ++i) {
        groupAttr += " " + to_string(i);
    }

    ss << "v=0\r\n"
       << "o=SimpleMediaServer 3259346588694 2 IN IP4 127.0.0.1\r\n"
       << "s=play_connection\r\n"
       << "t=0 0\r\n"
       << groupAttr << "\r\n"
       << "a=extmap-allow-mixed\r\n"
       << "a=msid-semantic: WMS\r\n";

    _iceUfrag = randomString(8);
    _icePwd = randomString(32);
}

void WebrtcClient::initPusherLocalSdpMedia(stringstream& ss, const shared_ptr<TrackInfo>& videoInfo, const shared_ptr<TrackInfo>& audioInfo)
{
    int midIndex = 0;
    string audioMsid = "sms-audio-msid";
    string videoMsid = "sms-video-msid";
    string cname = "sms-cname";
    int audiossrc = 10000;
    int videossrc = 20000;
    if (audioInfo) {
        if (audioInfo->codec_ == "g711a") {
            ss << "m=audio 9 UDP/TLS/RTP/SAVPF 8\r\n"
               << "a=rtpmap:8 PCMA/8000\r\n";
        } else if (audioInfo->codec_ == "g711u") {
            ss << "m=audio 9 UDP/TLS/RTP/SAVPF 0\r\n"
               << "a=rtpmap:0 PCMU/8000\r\n";
        } else if (audioInfo->codec_ == "opus") {
            ss << "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
               << "a=rtpmap:111 opus/48000/2\r\n"
               << "a=rtcp-fb:111 transport-cc\r\n"
               << "a=fmtp:111 minptime=10;useinbandfec=1\r\n";
        } else if (audioInfo->codec_ == "aac") {
            ss << "m=audio 9 UDP/TLS/RTP/SAVPF 97\r\n"
               << "a=rtpmap:97 MPEG4-GENERIC/" << audioInfo->samplerate_ << "/" << audioInfo->channel_ << "\r\n"
               << "a=fmtp:97 streamtype=5;profile-level-id=1;mode=AAC-hbr;"
	           << "sizelength=13;indexlength=3;indexdeltalength=3;config=";
            auto aacTrack = dynamic_pointer_cast<AacTrack>(audioInfo);
            auto aacCondig = aacTrack->getAacInfo();
            for(auto &ch : aacCondig){
                // snprintf(buf, sizeof(buf), "%02X", (uint8_t)ch);
                // configStr.append(buf);
                ss << std::hex << setw(2) << setfill('0') << (int)((uint8_t)ch);
            }
            ss << "\r\n";
        }
        ss  << "c=IN IP4 0.0.0.0\r\n"
            << "a=rtcp:9 IN IP4 0.0.0.0\r\n"
            << "a=ice-ufrag:" << _iceUfrag << "\r\n"
            << "a=ice-pwd:" << _icePwd << "\r\n"
            << "a=ice-options:trickle\r\n"
            << "a=fingerprint:sha-256 " << WebrtcContext::getFingerprint() << "\r\n"
            << "a=setup:actpass\r\n"
            << "a=mid:" << midIndex++ << "\r\n"
            << "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
            << "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
            << "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
            << "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
            << "a=sendonly\r\n"
            << "a=msid:- " << audioMsid << "\r\n"
            << "a=rtcp-mux\r\n"
            << "a=ssrc:" << audiossrc << " cname:" << cname << "\r\n"
            << "a=ssrc:" << audiossrc << " msid:- " << audioMsid << "\r\n";
    }

    if (videoInfo) {
        if (videoInfo->codec_ == "h264") {
            ss << "m=video 9 UDP/TLS/RTP/SAVPF 106\r\n"
               << "a=rtpmap:106 H264/90000\r\n"
               << "a=rtcp-fb:106 goog-remb\r\n"
               << "a=rtcp-fb:106 transport-cc\r\n"
               << "a=rtcp-fb:106 ccm fir\r\n"
               << "a=rtcp-fb:106 nack\r\n"
               << "a=rtcp-fb:106 nack pli\r\n"
               << "a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n";
        } else if (videoInfo->codec_ == "h265") {
            ss << "m=video 9 UDP/TLS/RTP/SAVPF 106\r\n"
               << "a=rtpmap:106 H265/90000\r\n"
               << "a=rtcp-fb:106 goog-remb\r\n"
               << "a=rtcp-fb:106 transport-cc\r\n"
               << "a=rtcp-fb:106 ccm fir\r\n"
               << "a=rtcp-fb:106 nack\r\n"
               << "a=rtcp-fb:106 nack pli\r\n"
               << "a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n";
        } else if (videoInfo->codec_ == "vp8") {
            ss << "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
               << "a=rtpmap:96 VP8/90000\r\n"
               << "a=rtcp-fb:96 goog-remb\r\n"
               << "a=rtcp-fb:96 transport-cc\r\n"
               << "a=rtcp-fb:96 ccm fir\r\n"
               << "a=rtcp-fb:96 nack\r\n"
               << "a=rtcp-fb:96 nack pli\r\n";
        } else if (videoInfo->codec_ == "vp9") {
            ss << "m=video 9 UDP/TLS/RTP/SAVPF 100\r\n"
               << "a=rtpmap:100 VP9/90000\r\n"
               << "a=rtcp-fb:100 goog-remb\r\n"
               << "a=rtcp-fb:100 transport-cc\r\n"
               << "a=rtcp-fb:100 ccm fir\r\n"
               << "a=rtcp-fb:100 nack\r\n"
               << "a=rtcp-fb:100 nack pli\r\n"
               << "a=fmtp:100 profile-id=2\r\n";
        } else if (videoInfo->codec_ == "av1") {
            ss << "m=video 9 UDP/TLS/RTP/SAVPF 45\r\n"
               << "a=rtpmap:45 AV1/90000\r\n"
               << "a=rtcp-fb:45 goog-remb\r\n"
               << "a=rtcp-fb:45 transport-cc\r\n"
               << "a=rtcp-fb:45 ccm fir\r\n"
               << "a=rtcp-fb:45 nack\r\n"
               << "a=rtcp-fb:45 nack pli\r\n"
               << "a=fmtp:45 level-idx=5;profile=0;tier=0\r\n";
        }

        ss << "c=IN IP4 0.0.0.0\r\n"
           << "a=rtcp:9 IN IP4 0.0.0.0\r\n"
           << "a=ice-ufrag:" << _iceUfrag << "\r\n"
           << "a=ice-pwd:" << _icePwd << "\r\n"
           << "a=ice-options:trickle\r\n"
           << "a=fingerprint:sha-256 " << WebrtcContext::getFingerprint() << "\r\n"
           << "a=setup:actpass\r\n"
           << "a=mid:" << midIndex++ << "\r\n"
           << "a=extmap:14 urn:ietf:params:rtp-hdrext:toffset\r\n"
           << "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
           << "a=extmap:13 urn:3gpp:video-orientation\r\n"
           << "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
           << "a=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay\r\n"
           << "a=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type\r\n"
           << "a=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing\r\n"
           << "a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space\r\n"
           << "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
           << "a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\n"
           << "a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\n"
           << "a=sendonly\r\n"
           << "a=msid:- " << videoMsid << "\r\n"
           << "a=rtcp-mux\r\n"
           << "a=rtcp-rsize\r\n"
           << "a=ssrc:" << videossrc << " cname:" << cname << "\r\n"
           << "a=ssrc:" << videossrc << " msid:- " << videoMsid << "\r\n";
    }
}

void WebrtcClient::initPullerLocalSdpMedia(stringstream& ss)
{
    int midIndex = 0;
    string audioMsid = "sms-audio-msid";
    string videoMsid = "sms-video-msid";
    string cname = "sms-cname";
    int audiossrc = 10000;
    int videossrc = 20000;

    ss << "m=audio 9 UDP/TLS/RTP/SAVPF 8 111 0 97\r\n"
        << "c=IN IP4 0.0.0.0\r\n"
        << "a=rtcp:9 IN IP4 0.0.0.0\r\n"
        << "a=ice-ufrag:" << _iceUfrag << "\r\n"
        << "a=ice-pwd:" << _icePwd << "\r\n"
        << "a=ice-options:trickle\r\n"
        << "a=fingerprint:sha-256 " << WebrtcContext::getFingerprint() << "\r\n"
        << "a=setup:actpass\r\n"
        << "a=mid:" << midIndex++ << "\r\n"
        << "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
        << "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
        << "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
        << "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
        << "a=recvonly\r\n"
        << "a=msid:- " << audioMsid << "\r\n"
        << "a=rtcp-mux\r\n"
        << "a=rtpmap:8 PCMA/8000\r\n"
        << "a=rtpmap:0 PCMU/8000\r\n"
        << "a=rtpmap:97 MPEG4-GENERIC/48000/2\r\n"
        << "a=rtpmap:111 opus/48000/2\r\n"
        << "a=rtcp-fb:111 transport-cc\r\n"
        << "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
        << "a=ssrc:" << audiossrc << " cname:" << cname << "\r\n"
        << "a=ssrc:" << audiossrc << " msid:- " << audioMsid << "\r\n";

    ss << "m=video 9 UDP/TLS/RTP/SAVPF 106 108 96 100 45\r\n"
        << "c=IN IP4 0.0.0.0\r\n"
        << "a=rtcp:9 IN IP4 0.0.0.0\r\n"
        << "a=ice-ufrag:" << _iceUfrag << "\r\n"
        << "a=ice-pwd:" << _icePwd << "\r\n"
        << "a=ice-options:trickle\r\n"
        << "a=fingerprint:sha-256 " << WebrtcContext::getFingerprint() << "\r\n"
        << "a=setup:actpass\r\n"
        << "a=mid:" << midIndex++ << "\r\n"
        << "a=extmap:14 urn:ietf:params:rtp-hdrext:toffset\r\n"
        << "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
        << "a=extmap:13 urn:3gpp:video-orientation\r\n"
        << "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
        << "a=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay\r\n"
        << "a=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type\r\n"
        << "a=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing\r\n"
        << "a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space\r\n"
        << "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
        << "a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\n"
        << "a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\n"
        << "a=recvonly\r\n"
        << "a=msid:- " << videoMsid << "\r\n"
        << "a=rtcp-mux\r\n"
        << "a=rtcp-rsize\r\n"
        << "a=rtpmap:106 H264/90000\r\n"
        << "a=rtcp-fb:106 goog-remb\r\n"
        << "a=rtcp-fb:106 transport-cc\r\n"
        << "a=rtcp-fb:106 ccm fir\r\n"
        << "a=rtcp-fb:106 nack\r\n"
        << "a=rtcp-fb:106 nack pli\r\n"
        << "a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n"
        << "a=rtpmap:108 H265/90000\r\n"
        << "a=rtcp-fb:108 goog-remb\r\n"
        << "a=rtcp-fb:108 transport-cc\r\n"
        << "a=rtcp-fb:108 ccm fir\r\n"
        << "a=rtcp-fb:108 nack\r\n"
        << "a=rtcp-fb:108 nack pli\r\n"
        << "a=fmtp:108 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n"
        << "a=rtpmap:96 VP8/90000\r\n"
        << "a=rtcp-fb:96 goog-remb\r\n"
        << "a=rtcp-fb:96 transport-cc\r\n"
        << "a=rtcp-fb:96 ccm fir\r\n"
        << "a=rtcp-fb:96 nack\r\n"
        << "a=rtcp-fb:96 nack pli\r\n"
        << "a=rtpmap:100 VP9/90000\r\n"
        << "a=rtcp-fb:100 goog-remb\r\n"
        << "a=rtcp-fb:100 transport-cc\r\n"
        << "a=rtcp-fb:100 ccm fir\r\n"
        << "a=rtcp-fb:100 nack\r\n"
        << "a=rtcp-fb:100 nack pli\r\n"
        << "a=fmtp:100 profile-id=2"
        << "a=rtpmap:45 AV1/90000\r\n"
        << "a=rtcp-fb:45 goog-remb\r\n"
        << "a=rtcp-fb:45 transport-cc\r\n"
        << "a=rtcp-fb:45 ccm fir\r\n"
        << "a=rtcp-fb:45 nack\r\n"
        << "a=rtcp-fb:45 nack pli\r\n"
        << "a=fmtp:45 level-idx=5;profile=0;tier=0\r\n"
        << "a=ssrc:" << videossrc << " cname:" << cname << "\r\n"
        << "a=ssrc:" << videossrc << " msid:- " << videoMsid << "\r\n";
}

void WebrtcClient::setRemoteSdp(const string& sdp)
{
    _remoteSdp = make_shared<WebrtcSdp>();
    _remoteSdp->parse(sdp);

    auto rtcSrc = _source.lock();
    if (!rtcSrc && _request == "pull") {
        logError << "source is empty, path: " << _urlParser.path_;
        return ;
    }
    if (_remoteSdp->_vecSdpMedia.size() == 0) {
        logError << "sdp media line is zero";
    }
    for (auto sdpMedia : _remoteSdp->_vecSdpMedia) {
        if (sdpMedia->media_ == "video") {
            for (auto ptIter : sdpMedia->mapPtInfo_) {
                _videoPtInfo = ptIter.second;
                if (_request == "pull") {
                    _videoDecodeTrack = make_shared<WebrtcDecodeTrack>(VideoTrackType, VideoTrackType, _videoPtInfo);
                    if (_videoDecodeTrack->getTrackInfo()) {
                        rtcSrc->addTrack(_videoDecodeTrack);
                    }
                }
            }
        } else if (sdpMedia->media_ == "audio") {
            for (auto ptIter : sdpMedia->mapPtInfo_) {
                _audioPtInfo = ptIter.second;
                if (_request == "pull") {
                    _audioDecodeTrack = make_shared<WebrtcDecodeTrack>(AudioTrackType, AudioTrackType, _audioPtInfo);
                    if (_audioDecodeTrack->getTrackInfo()) {
                        rtcSrc->addTrack(_audioDecodeTrack);
                    }
                }
            }
        }
    }

    weak_ptr<WebrtcClient> wSelf = shared_from_this();
    _socket = make_shared<Socket>(EventLoop::getCurrentLoop());
    auto iter = _remoteSdp->_vecSdpMedia.begin();
    if (iter != _remoteSdp->_vecSdpMedia.end()) {
        auto candidates = (*iter)->candidates_;
        if (candidates.size() > 0) {
            auto candidate = candidates[0];
            if (candidate->transType_ == "tcp") {
                logTrace << "create tcp socket";
                _socket->createSocket(SOCKET_TCP);
                _socket->connect(candidate->ip_, candidate->port_);
                _socket->setWriteCb([wSelf](){
                    auto self = wSelf.lock();
                    if (self) {
                        self->onConnected();
                    }
                });
            } else {
                logTrace << "create udp socket";
                auto addr = _socket->createSocket(candidate->ip_, candidate->port_, SOCKET_UDP);
                if (_socket->bind(0, "0.0.0.0") == -1) {
                    onError("create rtp/udp socket failed");
                    return ;
                }
                _addrLen = sizeof(sockaddr);
                _addr = (struct sockaddr*)malloc(sizeof(sockaddr));
                memcpy(_addr, ((sockaddr*)&addr), sizeof(sockaddr));
                _socket->bindPeerAddr(_addr);
                logTrace << "socket ip: " << _socket->getLocalIp() << ", port: " << _socket->getLocalPort();
                onConnected();
            }
            _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
                auto self = wSelf.lock();
                if (self) {
                    self->onRead(buffer);
                }

                return 0;
            });
            _socket->addToEpoll();
        }
    }
}

void WebrtcClient::onConnected()
{
    _socket->setWriteCb(nullptr);

    string remoteIceUfrag;
    if (_remoteSdp->_title->iceUfrag_.empty()) {
        auto iter = _remoteSdp->_vecSdpMedia.begin();
        if (iter != _remoteSdp->_vecSdpMedia.end()) {
            remoteIceUfrag = (*iter)->iceUfrag_;
        }
    } else {
        remoteIceUfrag = _remoteSdp->_title->iceUfrag_;
    }
    _username = remoteIceUfrag + _iceUfrag;
    string tid = randomStr(12);

    WebrtcStun stunReq;
    stunReq.setType(BindingRequest);
	stunReq.setLocalUfrag(_iceUfrag);
    stunReq.setRemoteUfrag(remoteIceUfrag);
    stunReq.setTranscationId(tid);

    logTrace << "socket ip: " << _socket->getLocalIp() << ", port: " << _socket->getLocalPort();
    // string localIp = "127.0.0.1";
	// stunReq.setMappedAddress(ntohl(inet_addr(_socket->getLocalIp().c_str())));
	// stunReq.setMappedPort(ntohs(_socket->getLocalPort()));
	// stunReq.setMappedPort(_socket->getLocalPort());
    if (_addr->sa_family == AF_INET) {
	    stunReq.setMappedAddress((sockaddr *)_socket->getPeerAddr4());
    } else {
        stunReq.setMappedAddress((sockaddr *)_socket->getPeerAddr6());
    }

    // char buf[kRtpPacketSize];
    // SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    // SrsAutoFree(SrsBuffer, stream);

    auto buffer = make_shared<StringBuffer>();
    // char buf[1500];
    // SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    // SrsAutoFree(SrsBuffer, stream);
    string remoteIcePwd;
    if (_remoteSdp->_title->icePwd_.empty()) {
        auto iter = _remoteSdp->_vecSdpMedia.begin();
        if (iter != _remoteSdp->_vecSdpMedia.end()) {
            remoteIcePwd = (*iter)->icePwd_;
        }
    } else {
        remoteIcePwd = _remoteSdp->_title->icePwd_;
    }
	if (0 != stunReq.toBuffer(remoteIcePwd, buffer)) {
		logError << "encode stun request failed";
		return ;
	}

    if (_socket->getSocketType() == SOCKET_TCP) {
        uint8_t payload_ptr[2];
        payload_ptr[0] = buffer->size() >> 8;
        payload_ptr[1] = buffer->size() & 0x00FF;

        _socket->send((char*)payload_ptr, 2);
    }

    logDebug << "send a stun request, path: " << _urlParser.path_;
    _socket->send(buffer);

    weak_ptr<WebrtcClient> wSelf = shared_from_this();
    _loop->addTimerTask(5000, [wSelf, buffer](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        if (self->_socket->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = buffer->size() >> 8;
            payload_ptr[1] = buffer->size() & 0x00FF;

            self->_socket->send((char*)payload_ptr, 2);
        }

        logDebug << "send a stun request, path: " << self->_urlParser.path_;
        self->_socket->send(buffer);

        return 5000;
    }, nullptr);
}

void WebrtcClient::onRead(const StreamBuffer::Ptr& buffer)
{
    if (_socket->getSocketType() == SOCKET_TCP) {
        _parser.parse(buffer->data(), buffer->size());
    } else {
        onRtcPacket(buffer->data(), buffer->size());
    }
}

void WebrtcClient::onRtcPacket(const char* data, int len)
{
    auto buffer = StreamBuffer::create();
    buffer->assign(data, len);int pktType = guessType(buffer);

    logTrace << "get a packet: " << pktType;
    switch(pktType) {
        case kStunPkt: {
            onStunPacket(buffer);
            break;
        }
        case kDtlsPkt: {
            onDtlsPacket(buffer);
            break;
        }
        case kRtpPkt: {
            auto rtp = make_shared<WebrtcRtpPacket>(buffer, 0);
            onRtpPacket(rtp);
            break;
        }
        case kRtcpPkt: {
            onRtcpPacket(buffer);
            break;
        }
        default: {
            logWarn << "unknown pkt type: " << pktType;
            break;
        }
    }
}

void WebrtcClient::onStunPacket(const StreamBuffer::Ptr& buffer)
{
	WebrtcStun stunRsp;
	if (0 != stunRsp.parse(buffer->data(), buffer->size())) {
		logError << "parse stun packet failed";
		return;
	}

    if (_enableDtls) {
        if (!_sendDtls) {
            weak_ptr<WebrtcClient> wSelf = shared_from_this();
            _dtlsSession->setOnHandshakeDone([wSelf](){
                logInfo << "start to publish";
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
                self->_dtlsHandshakeDone = true;
                if (self->_enableSrtp && !self->_srtpSession) {
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
                }

                if (self->_request == "push") {
                    self->startPlay();
                }
            });
            _dtlsSession->startActiveHandshake(_socket);
            _sendDtls = true;

            _loop->addTimerTask(50, [wSelf](){
                auto self = wSelf.lock();
                if (!self) {
                    return 0;
                }
                int loopTime = self->onDtlsCheck();

                if (loopTime == 0 && self->_dtlsTimeTask) {
                    self->_dtlsTimeTask->quit = true;
                    self->_dtlsTimeTask = nullptr;
                }

                return loopTime;
            }, [wSelf](bool success, shared_ptr<TimerTask> timeTask){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
                self->_dtlsTimeTask = timeTask;
            });
        }
    } else if (_request == "push") {
        startPlay();
    }
}

int64_t WebrtcClient::onDtlsCheck()
{
    int32_t err = 0;
    // const int32_t max_loop = 512;
    // int32_t arq_count = 0;
    // int32_t arq_max_retry = 12 * 2;
    // for (int32_t i = 0; arq_count < arq_max_retry && i < max_loop; i++) {
    if (_dtlsHandshakeDone) {
        return 0;
    }

    // // For DTLS client ARQ, the state should be specified.
    // if (m_state != YangDtlsStateClientHello && m_state != YangDtlsStateClientCertificate) {
    //     return err;
    // }

        // If there is a timeout in progress, it sets *out to the time remaining
        // and returns one. Otherwise, it returns zero.

    return _dtlsSession->checkTimeout(_socket);
}

void WebrtcClient::onDtlsPacket(const StreamBuffer::Ptr& buffer)
{
    if (_dtlsSession->decodeHandshake(_socket, buffer->data(), buffer->size()) != 0)
    {
        logError << "decode dtls packet faled";
    }
}

void WebrtcClient::onRtcpPacket(const StreamBuffer::Ptr& buffer)
{
    logDebug << "get a rtcp packet";
}

void WebrtcClient::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    RtpPacket::Ptr rtpPacket;

    if (_enableSrtp) {
        char plaintext[1500];
        int nb_plaintext = rtp->size();
        auto data = rtp->data();
        if (0 != _srtpSession->unprotectRtp(data, plaintext, nb_plaintext))
            return ;

        auto rtpbuffer = StreamBuffer::create();
        rtpbuffer->assign(plaintext, nb_plaintext);

        rtpPacket = make_shared<WebrtcRtpPacket>(rtpbuffer);
    } else {
        rtpPacket = rtp;
    }

    if (rtp->getHeader()->pt == _videoPtInfo->payloadType_ && _videoDecodeTrack) {
        // logTrace << "decode rtp";
        _videoDecodeTrack->decodeRtp(rtpPacket);
    } else if (rtp->getHeader()->pt == _audioPtInfo->payloadType_ && _audioDecodeTrack) {
        // logWarn << "videoDecodeTrack is empty";
        _audioDecodeTrack->decodeRtp(rtpPacket);
    }
}

void WebrtcClient::startPlay()
{

    // if (_enableSrtp && !_srtpSession) {
	// 	_srtpSession.reset(new SrtpSession());
	// 	std::string recv_key, send_key;
	// 	if (0 != _dtlsSession->getSrtpKey(recv_key, send_key)) {
	// 		logError << "dtls get srtp key failed";
    //         throw runtime_error("dtls get srtp key failed");
	// 	}

    //     if (!_srtpSession->init(recv_key, send_key)) {
    //         logError << "srtp session init failed";
    //         throw runtime_error("srtp session init failed");
    //     }
	// }

    weak_ptr<WebrtcClient> wSelf = dynamic_pointer_cast<WebrtcClient>(shared_from_this());
    // _urlParser.path_ = _path;
    // _urlParser.vhost_ = DEFAULT_VHOST;
    // _urlParser.protocol_ = PROTOCOL_WEBRTC;
    // _urlParser.type_ = DEFAULT_TYPE;

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

void WebrtcClient::startPlay(const MediaSource::Ptr &src)
{
    auto rtcSrc = dynamic_pointer_cast<WebrtcMediaSource>(src);
    if (!rtcSrc) {
        return ;
    }

    _source = rtcSrc;

    if (!_playReader/* && _rtp_type != Rtsp::RTP_MULTICAST*/) {
        weak_ptr<WebrtcClient> weak_self = static_pointer_cast<WebrtcClient>(shared_from_this());
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

        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPlayer(info);
        }
    }
}

void WebrtcClient::sendRtpPacket(const WebrtcMediaSource::DataType &pack)
{
    int i = 0;
    int len = pack->size() - 1;
    auto pktlist = *(pack.get());

    for (auto& packet: pktlist) {
        sendMedia(packet);
    };
}

void WebrtcClient::sendMedia(const RtpPacket::Ptr& rtp)
{
    if (!rtp) {
        return ;
    } else if (rtp->type_ == "video" && !_videoPtInfo) {
        logDebug << "video pt info is empty";
        return ;
    } else if (rtp->type_ == "audio" && !_audioPtInfo) {
        logDebug << "audio pt info is empty";
        return ;
    }

	int nb_cipher = rtp->size() - 4;
    // char data[1500];
    auto buffer = make_shared<StreamBuffer>(1500 + 1);
    auto data = buffer->data();
    memcpy(data, rtp->data() + 4, nb_cipher);

	// auto sdp_video_pt = 106;

	// auto _video_payload_type = sdp_video_pt == 0 ? 106 : sdp_video_pt;
	data[1] = (data[1] & 0x80) | (rtp->type_ == "audio" ? _audioPtInfo->payloadType_ : _videoPtInfo->payloadType_);
	uint32_t ssrc = htonl(rtp->type_ == "audio" ? 10000 : 20000);
	memcpy(data + 8, &ssrc, sizeof(ssrc));

    // FILE* fp = fopen("test.rtp", "ab+");
	// fwrite(data, nb_cipher, 1, fp);
	// fclose(fp);

    if (_enableSrtp) {
        if (0 != _srtpSession->protectRtp(data, &nb_cipher)) {
            return ;
        }
    }

	// if (0 == _srtpSession->protectRtp(data, &nb_cipher)) {
        if (_socket->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = nb_cipher >> 8;
            payload_ptr[1] = nb_cipher & 0x00FF;

            _socket->send((char*)payload_ptr, 2);
        }
		_socket->send(buffer, 1, 0, nb_cipher, _addr, _addrLen);
		// _sendRtpPack_10s++;
		// lastest_packet_send_time_ = time(nullptr);
	// }
	// _bytes += nb_cipher;
}