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

using namespace std;

class WebrtcContext : public enable_shared_from_this<WebrtcContext> {
public:
    using Ptr = shared_ptr<WebrtcContext>;
    using Wptr = weak_ptr<WebrtcContext>;

    WebrtcContext();
    ~WebrtcContext();

public:
    void initPlayer(const string& appName, const string& streamName, const string& sdp);
    void initPublisher(const string& appName, const string& streamName, const string& sdp);
    void onRtpPacket(const Socket::Ptr& socket, const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len);
    bool isAlive();
    void setDtls(int dtls) {_enbaleDtls = dtls;}
    string getUsername() {return _username;}
    void onStunPacket(const Socket::Ptr& socket, const WebrtcStun& stunReq, struct sockaddr* addr, int len);
    void onDtlsPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& stunReq, struct sockaddr* addr, int len);
    void onRtcpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& stunReq, struct sockaddr* addr, int len);
    EventLoop::Ptr getLoop() {return _loop;}
    void changeLoop(const EventLoop::Ptr& loop);
    void close();

    string getLocalSdp();

    static void initDtlsCert();
    static string getFingerprint();
    static shared_ptr<DtlsCertificate> getDtlsCertificate();

private:
    void negotiatePlayValid(const shared_ptr<TrackInfo>& videoInfo, const shared_ptr<TrackInfo>& audioInfo);
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
    bool _enbaleDtls = false;
    bool _firstResend = true;
    bool _alive = true;
    bool _isPlayer = true;
    bool _rtcpPliTimerCreated = false;
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

    uint64_t _sendRtpPack_10s;
    uint64_t _sendRtcpNackPack_10s;
    uint64_t _rtpLoss_10s;
    uint64_t _resendRtpPack_10s;
    uint64_t _totalRtpCnt = 0;

    float _lossPercent;

    Socket::Ptr	_socket;
	struct sockaddr* _addr = nullptr;
	int _addrLen = 0;
    int _localChannelPort = 0;
    int _peerChannelPort = 0;

    string _path;
    string _iceUfrag;
    string _icePwd;
    string _username;
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
    shared_ptr<WebrtcSdp> _localSdp;
    shared_ptr<WebrtcSdp> _remoteSdp;
    shared_ptr<DtlsSession> _dtlsSession;
    shared_ptr<SrtpSession> _srtpSession;
    WebrtcMediaSource::Wptr _source;
    WebrtcMediaSource::QueType::DataQueReaderT::Ptr _playReader;
    
	//缓存rtp报文,大小为256
	vector<RtpPacket::Ptr> _rtpCache = std::vector<RtpPacket::Ptr>{256, nullptr};
};

#endif //GB28181Manager_h