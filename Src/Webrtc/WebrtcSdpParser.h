#ifndef WebrtcSdpParser_H
#define WebrtcSdpParser_H

#include <cstdint>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// using namespace std;

namespace SMS
{
    enum SendRecvType
    {
        Unknown,
        SendOnly,
        RecvOnly,
        SendRecv,
        Inactive
    };
};

class WebrtcPtInfo
{
public:
    using Ptr = std::shared_ptr<WebrtcPtInfo>;
    void encode(std::stringstream& ss);
public:
    int payloadType_;
    std::string codec_;
    std::string codecExt_;
    int samplerate_;

    std::string fmtp_;
    std::vector<std::string> rtcpFbs_;

    // 关联 ssrcinfo
    uint32_t ssrc_;

    // 关联rtx
    uint32_t rtxPt_;

    // 如果是rtx，关联源pt
    uint32_t origPt_;
};

class CandidateInfo
{
public:
    using Ptr = std::shared_ptr<CandidateInfo>;
    void encode(std::stringstream& ss);
public:
    std::string foundation_;
    int componentId_;
    // tcp udp
    std::string transType_;
    int priority_;
    std::string ip_;
    int port_;
    // host srflx prflx relay
    std::string candidateType_;
    std::string relAddr_;
    int relPort_;
};

class SsrcInfo
{
public:
    using Ptr = std::shared_ptr<SsrcInfo>;
    
    void encode(std::stringstream& ss);
public:
    uint64_t ssrc_;
    std::string cname_;
    std::string msid_;
    std::vector<std::string> msidTracker_;
    std::string mslabel_;
    std::string label_;
};

class WebrtcSdpMedia
{
public:
    void parseMediaDesc(const std::string& value);
    void parseAttr(const std::string& value);
    void parseConnect(const std::string& value);

    void encode(std::stringstream& ss);

private:
    // decode the attrs
    void parseExtmap(const std::string& value);
    void parseRtpmap(const std::string& value);
    void parseRtcp(const std::string& value);
    void parseRtcpFb(const std::string& value);
    void parseFmtp(const std::string& value);
    void parseMid(const std::string& value);
    void parseMsid(const std::string& value);
    void parseSsrc(const std::string& value);
    void parseSsrcGroup(const std::string& value);
    void parseRtcpMux(const std::string& value);
    void parseRtcpRsize(const std::string& value);
    void parseRecvonly(const std::string& value);
    void parseSendonly(const std::string& value);
    void parseSendrecv(const std::string& value);
    void parseInactive(const std::string& value);
    void parseIceOptions(const std::string& value);
    void parseIceUflag(const std::string& value);
    void parseIcePwd(const std::string& value);
    void parseSetup(const std::string& value);
    void parseExtmapAllowMixed(const std::string& value);
    void parseFingerprint(const std::string& value);
    void parseCandidate(const std::string& value);
    void parseSctpPort(const std::string& value);

public:
    int index_;
    std::string media_;
    int port_;
    int channelPort_;
    std::string protocol_;
    bool rtcpMux_ = false;
    bool rtcpRsize_ = false;
    SMS::SendRecvType sendRecvType_ = SMS::Unknown;
    std::string iceOptions_;
    std::string iceUfrag_;
    std::string icePwd_;
    std::string setup_;
    bool mixed_ = false;
    std::string fingerprintAlg_;
    std::string fingerprint_;
    std::string mid_;
    std::string msid_;
    std::vector<std::string> msidTracker_;
    std::string channelName_;

    std::unordered_map<int, WebrtcPtInfo::Ptr> mapPtInfo_;
    std::unordered_map<uint64_t, SsrcInfo::Ptr> mapSsrcInfo_;
    std::vector<CandidateInfo::Ptr> candidates_;
    // string fid,fec,sim
    std::unordered_map<std::string, std::vector<uint64_t>> mapSsrcGroup_;
    std::unordered_map<std::string, uint64_t> mapSsrc_;

    std::unordered_map<char, std::string> mapMedia_;
    std::unordered_map<std::string, std::string> mapAttr_;
    std::unordered_map<std::string, int> mapExtmap_;
};

class WebrtcSdpTitle
{
public:
    void parseVersion(const std::string& value);
    void parseOrigin(const std::string& value);
    void parseSession(const std::string& value);
    void parseTime(const std::string& value);
    void parseAttr(const std::string& value);

    void encode(std::stringstream& ss);
public:
    //v
    std::string version_;
    //o
    std::string username_;
    std::string sessionId_;
    std::string sessionVersion_;
    std::string netType_;
    std::string addrType_;
    std::string addr_;
    //s
    std::string sessionName_;
    //t
    uint64_t startTime_;
    uint64_t endTime_;
    //a
    std::string groupPolicy_;
    std::vector<std::string> groups_;
    std::string msidSemantic_;
    std::vector<std::string> msids_;
    //a ice
    std::string fingerprintAlg_;
    std::string fingerprint_;
    std::string iceOptions_;
    std::string iceUfrag_;
    std::string icePwd_;
    std::string setup_;
    std::string iceRole_;
    //extmap-allow-mixed
    bool mixed_ = false;

    std::unordered_map<char, std::string> mapTitle_;
    std::unordered_map<std::string, std::string> mapAttr_;
};

class WebrtcSdp {
public:
    void parse(const std::string& sdp);
    std::string getSdp();
    void addCandidate(const CandidateInfo::Ptr& info);

public:
    
    std::string _sdp;
    std::shared_ptr<WebrtcSdpTitle> _title;
    // index , SdpMedia
    std::vector<std::shared_ptr<WebrtcSdpMedia>> _vecSdpMedia;
    std::shared_ptr<WebrtcSdpMedia> _dataChannelSdp;
};



#endif //WebrtcSdpParser_H
