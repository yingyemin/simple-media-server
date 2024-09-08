#ifndef WebrtcSdpParser_H
#define WebrtcSdpParser_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

using namespace std;

enum SendRecvType
{
    Unknown,
    SendOnly,
    RecvOnly,
    SendRecv,
    Inactive
};

class WebrtcPtInfo
{
public:
    using Ptr = shared_ptr<WebrtcPtInfo>;
    void encode(stringstream& ss);
public:
    int payloadType_;
    string codec_;
    string codecExt_;
    int samplerate_;

    string fmtp_;
    vector<string> rtcpFbs_;

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
    using Ptr = shared_ptr<CandidateInfo>;
    void encode(stringstream& ss);
public:
    string foundation_;
    int componentId_;
    // tcp udp
    string transType_;
    int priority_;
    string ip_;
    int port_;
    // host srflx prflx relay
    string candidateType_;
    string relAddr_;
    int relPort_;
};

class SsrcInfo
{
public:
    using Ptr = shared_ptr<SsrcInfo>;
    
    void encode(stringstream& ss);
public:
    uint64_t ssrc_;
    string cname_;
    string msid_;
    vector<string> msidTracker_;
    string mslabel_;
    string label_;
};

class WebrtcSdpMedia
{
public:
    void parseMediaDesc(const string& value);
    void parseAttr(const string& value);
    void parseConnect(const string& value);

    void encode(stringstream& ss);

private:
    // decode the attrs
    void parseExtmap(const string& value);
    void parseRtpmap(const string& value);
    void parseRtcp(const string& value);
    void parseRtcpFb(const string& value);
    void parseFmtp(const string& value);
    void parseMid(const string& value);
    void parseMsid(const string& value);
    void parseSsrc(const string& value);
    void parseSsrcGroup(const string& value);
    void parseRtcpMux(const string& value);
    void parseRtcpRsize(const string& value);
    void parseRecvonly(const string& value);
    void parseSendonly(const string& value);
    void parseSendrecv(const string& value);
    void parseInactive(const string& value);
    void parseIceOptions(const string& value);
    void parseIceUflag(const string& value);
    void parseIcePwd(const string& value);
    void parseSetup(const string& value);
    void parseExtmapAllowMixed(const string& value);
    void parseFingerprint(const string& value);
    void parseCandidate(const string& value);

public:
    int index_;
    string media_;
    int port_;
    int channelPort_;
    string protocol_;
    bool rtcpMux_ = false;
    bool rtcpRsize_ = false;
    SendRecvType sendRecvType_ = Unknown;
    string iceOptions_;
    string iceUfrag_;
    string icePwd_;
    string setup_;
    bool mixed_ = false;
    string fingerprintAlg_;
    string fingerprint_;
    string mid_;
    string msid_;
    vector<string> msidTracker_;
    string channelName_;

    unordered_map<int, WebrtcPtInfo::Ptr> mapPtInfo_;
    unordered_map<uint64_t, SsrcInfo::Ptr> mapSsrcInfo_;
    vector<CandidateInfo::Ptr> candidates_;
    // string fid,fec,sim
    unordered_map<string, vector<uint64_t>> mapSsrcGroup_;
    unordered_map<string, uint64_t> mapSsrc_;

    unordered_map<char, string> mapMedia_;
    unordered_map<string, string> mapAttr_;
    unordered_map<string, int> mapExtmap_;
};

class WebrtcSdpTitle
{
public:
    void parseVersion(const string& value);
    void parseOrigin(const string& value);
    void parseSession(const string& value);
    void parseTime(const string& value);
    void parseAttr(const string& value);

    void encode(stringstream& ss);
public:
    //v
    string version_;
    //o
    string username_;
    string sessionId_;
    string sessionVersion_;
    string netType_;
    string addrType_;
    string addr_;
    //s
    string sessionName_;
    //t
    uint64_t startTime_;
    uint64_t endTime_;
    //a
    string groupPolicy_;
    vector<string> groups_;
    string msidSemantic_;
    vector<string> msids_;
    //a ice
    string fingerprintAlg_;
    string fingerprint_;
    string iceOptions_;
    string iceUfrag_;
    string icePwd_;
    string setup_;
    string iceRole_;
    //extmap-allow-mixed
    bool mixed_ = false;

    unordered_map<char, string> mapTitle_;
    unordered_map<string, string> mapAttr_;
};

class WebrtcSdp {
public:
    void parse(const string& sdp);
    string getSdp();
    void addCandidate(const CandidateInfo::Ptr& info);

public:
    
    string _sdp;
    shared_ptr<WebrtcSdpTitle> _title;
    // index , SdpMedia
    vector<shared_ptr<WebrtcSdpMedia>> _vecSdpMedia;
    shared_ptr<WebrtcSdpMedia> _dataChannelSdp;
};



#endif //WebrtcSdpParser_H
