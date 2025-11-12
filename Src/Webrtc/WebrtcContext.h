#ifndef WebrtcContext_h
#define WebrtcContext_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "WebrtcContext.h"
#include "WebrtcRtpPacket.h"
#include "WebrtcDtlsSession.h"
#include "WebrtcStun.h"
#include "WebrtcSrtpSession.h"
#include "Util/TimeClock.h"
#include "WebrtcMediaSource.h"
#include "Common/UrlParser.h"
#include "Rtp/RtpSort.h"
#include "SctpAssociation.h"

// using namespace std;

class WebrtcContext : public std::enable_shared_from_this<WebrtcContext> {
public:
    using Ptr = std::shared_ptr<WebrtcContext>;
    using Wptr = std::weak_ptr<WebrtcContext>;

    WebrtcContext();
    ~WebrtcContext();

public:
    void initPlayer(const std::string& appName, const std::string& streamName, const std::string& sdp);
    void initPublisher(const std::string& appName, const std::string& streamName, const std::string& sdp);
    void onRtpPacket(const Socket::Ptr& socket, const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len);
    bool isAlive();
    void setDtls(int dtls) {_enbaleDtls = dtls;}
    void setPreferVideoCodec(const std::string& preferVideoCodec) {_preferVideoCodec = preferVideoCodec;}
    void setPreferAudioCodec(const std::string& preferAudioCodec) {_preferAudioCodec = preferAudioCodec;}
    void setParams(const string& params) {_params = params;}
    string getUsername() {return _username;}
    void onStunPacket(const Socket::Ptr& socket, const WebrtcStun& stunReq, struct sockaddr* addr, int len);
    void onDtlsPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& stunReq, struct sockaddr* addr, int len);
    void onRtcpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& stunReq, struct sockaddr* addr, int len);
    EventLoop::Ptr getLoop() {return _loop;}
    void changeLoop(const EventLoop::Ptr& loop);
    void close();

    std::string getLocalSdp();

    static void initDtlsCert();
    static std::string getFingerprint();
    static std::shared_ptr<DtlsCertificate> getDtlsCertificate();

private:
    void negotiatePlayValid(const std::shared_ptr<TrackInfo>& videoInfo, const std::shared_ptr<TrackInfo>& audioInfo);
    void initPlayerLocalSdp(int trackNum);
    void nackHeartBeat();
    void handleRtcp(char* buf, int size);
    void startPlay();
    void startPlay(const MediaSource::Ptr &src);
    void sendRtpPacket(const WebrtcMediaSource::DataType &pack);
    void sendMedia(const RtpPacket::Ptr& rtp);
    void sendRtcpPli(int ssrc);
    void onManager();
    void checkAndSendRtcpNack();
    void onRecvDtlsApplicationData(const char* data, int len);
    void sendDatachannel(uint16_t streamId, uint32_t ppid, const char *msg, size_t len);

private:
    bool _enbaleDtls = true;
    bool _enbaleSrtp = true;
    bool _firstResend = true;
    bool _alive = true;
    bool _isPlayer = true;
    bool _rtcpPliTimerCreated = false;
    bool _hasInitSrtp = false;
    bool _hasOnAuth = false;
    int _videoFirst = 0;
    uint64_t _lastRecvTime;

    WebrtcPtInfo::Ptr _videoRedPtInfo;
    WebrtcPtInfo::Ptr _audioRedPtInfo;

    WebrtcPtInfo::Ptr _videoUlpfecPtInfo;
    WebrtcPtInfo::Ptr _audioUlpfecPtInfo;

    WebrtcPtInfo::Ptr _videoFlexfecPtInfo;
    WebrtcPtInfo::Ptr _audioFlexfecPtInfo;

    WebrtcPtInfo::Ptr _videoRtxPtInfo;
    WebrtcPtInfo::Ptr _audioRtxPtInfo;

    std::string _preferVideoCodec = "h264";
    std::string _preferAudioCodec = "g711a";

    uint64_t _sendRtpPack_10s;
    uint64_t _sendRtcpNackPack_10s;
    uint64_t _rtpLoss_10s;
    uint64_t _resendRtpPack_10s;
    uint64_t _totalRtpCnt = 0;
    uint64_t _totalRtpBytes = 0;
    uint32_t _lastRtpTs = 0;

    float _lossPercent;

    Socket::Ptr	_debugSocket;
	struct sockaddr* _debugAddr = nullptr;

    Socket::Ptr	_socket;
	struct sockaddr* _addr = nullptr;
	int _addrLen = 0;
    int _localChannelPort = 0;
    int _peerChannelPort = 0;

    std::string _path;
    std::string _params;
    std::string _iceUfrag;
    std::string _icePwd;
    std::string _username;
    TimeClock _timeClock;
    TimeClock _lastPktClock;
    UrlParser _urlParser;
    RtpExtTypeMap rtpExtTypeMap;
    SctpAssociationImp::Ptr _sctp;
    RtpSort::Ptr _videoSort;
    RtpSort::Ptr _audioSort;
    EventLoop::Ptr _loop;
    WebrtcPtInfo::Ptr _videoPtInfo;
    WebrtcPtInfo::Ptr _audioPtInfo;
    WebrtcDecodeTrack::Ptr _videoDecodeTrack;
    WebrtcDecodeTrack::Ptr _audioDecodeTrack;
    std::shared_ptr<WebrtcSdp> _localSdp;
    std::shared_ptr<WebrtcSdp> _remoteSdp;
    std::shared_ptr<DtlsSession> _dtlsSession;
    std::shared_ptr<SrtpSession> _srtpSession;
    WebrtcMediaSource::Wptr _source;
    WebrtcMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    
	//缓存rtp报文,大小为256
	vector<RtpPacket::Ptr> _rtpCache = std::vector<RtpPacket::Ptr>{256, nullptr};
};

#endif //GB28181Manager_h