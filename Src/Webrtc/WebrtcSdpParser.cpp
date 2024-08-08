#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WebrtcSdpParser.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

// sdp解析文档 https://blog.csdn.net/caesar1228/article/details/127822042
// https://blog.csdn.net/hyl999/article/details/116561558

//////////////////////////WebrtcSdpTitle///////////////////////////////////

void WebrtcSdpTitle::parseVersion(const string& value)
{
    version_ = value;
}

void WebrtcSdpTitle::parseOrigin(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 6) {
        return ;
    }

    username_ = vecValue[0];
    sessionId_ = vecValue[1];
    sessionVersion_ = vecValue[2];
    netType_ = vecValue[3];
    addrType_ = vecValue[4];
    addr_ = vecValue[5];
}

void WebrtcSdpTitle::parseSession(const string& value)
{
    sessionName_ = value;
}

void WebrtcSdpTitle::parseTime(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    startTime_ = stoull(vecValue[0]);
    endTime_ = stoull(vecValue[1]);
}

void WebrtcSdpTitle::parseAttr(const string& value)
{
    std::string attrKey = "";
    std::string attrValue = "";
    size_t pos = value.find_first_of(":");

    if (pos == std::string::npos) {
        attrKey = value;
    } else {
        attrKey = value.substr(0, pos);
        attrValue = value.substr(pos + 1);
    }

    if (attrKey == "ice-options") {
        iceOptions_ = attrValue;
        return ;
    } else if (attrKey == "ice-ufrag") {
        iceUfrag_ = attrValue;
        return ;
    } else if (attrKey == "ice-pwd") {
        icePwd_ = attrValue;
        return ;
    } else if (attrKey == "setup") {
        setup_ = attrValue;
        return ;
    } else if (attrKey == "extmap-allow-mixed") {
        mixed_ = true;
        return ;
    }

    auto vecValue = split(attrValue, " ");

    if (attrKey == "fingerprint") {    
        if (vecValue.size() < 2) {
            return ;
        }
        fingerprintAlg_ = vecValue[0];
        fingerprint_ = vecValue[1];
    } else if (attrKey == "msid-semantic") {
        int size = vecValue.size();
        msidSemantic_ = vecValue[0];
        
        if (vecValue.size() < 2) {
            return ;
        }

        for (int i = 1; i < size; ++i) {
            msids_.push_back(vecValue[i]);
        }
    } else if (attrKey == "group") {
        int size = vecValue.size();
        groupPolicy_ = vecValue[0];
        
        if (vecValue.size() < 2) {
            return ;
        }

        for (int i = 1; i < size; ++i) {
            groups_.push_back(vecValue[i]);
        }
    }
}

void WebrtcSdpTitle::encode(stringstream& ss)
{
    ss << "v=" << version_ << "\r\n"
       << "o=" << username_ << " " << sessionId_ << " " << sessionVersion_ << " " 
               << netType_ << " " << addrType_ << " " << addr_ << "\r\n"
       << "s=" << sessionName_ << "\r\n"
       << "t=" << startTime_ << " " << endTime_ << "\r\n";

    if (!groups_.empty()) {
        ss << "a=group:" << groupPolicy_;
        for (auto& iter : groups_) {
            ss << " " << iter;
        }
        ss << "\r\n";
    }

    if (!msids_.empty()) {
        ss << "a=msid-semantic:" << msidSemantic_;
        for (auto& iter : msids_) {
            ss << " " << iter;
        }
        ss << "\r\n";
    }

    if (!iceRole_.empty()) {
        ss << "a=" << iceRole_ << "\r\n";
    }

    if (!iceUfrag_.empty()) {
        ss << "a=ice-ufrag:" << iceUfrag_ << "\r\n";
    }

    if (!icePwd_.empty()) {
        ss << "a=ice-pwd:" << icePwd_ << "\r\n";
    }

    if (!iceOptions_.empty()) {
        ss << "a=ice-options:" << iceOptions_ << "\r\n";
    } else {
        ss << "a=ice-options:trickle" << "\r\n";
    }

    if (!fingerprintAlg_.empty() && ! fingerprint_.empty()) {
        ss << "a=fingerprint:" << fingerprintAlg_ << " " << fingerprint_ << "\r\n";
    }

    if (! setup_.empty()) {
        ss << "a=setup:" << setup_ << "\r\n";
    }
}

/////////////////////////////WebrtcSdpMedia/////////////////////////////

void WebrtcSdpMedia::parseMediaDesc(const string& value)
{
    auto vecValue = split(value, " ");
    int size = vecValue.size();
    if (size < 4) {
        return ;
    }

    media_ = vecValue[0];
    port_ = stoi(vecValue[1]);
    protocol_ = vecValue[2];

    for (int i = 3; i < size; ++i) {
        auto ptInfo = make_shared<WebrtcPtInfo>();
        ptInfo->payloadType_ = stoi(vecValue[i]);
        mapPtInfo_.emplace(ptInfo->payloadType_, ptInfo);
    }
}

void WebrtcSdpMedia::parseAttr(const string& value)
{
    std::string attrKey = "";
    std::string attrValue = "";
    size_t pos = value.find_first_of(":");

    if (pos != std::string::npos) {
        attrKey = value.substr(0, pos);
        attrValue = value.substr(pos + 1);
    } else {
        attrKey = value;
    }

    static unordered_map<string, void (WebrtcSdpMedia::*)(const string& value)> sdpMediaHandle {
        {"extmap", &WebrtcSdpMedia::parseExtmap},
        {"rtpmap", &WebrtcSdpMedia::parseRtpmap},
        {"rtcp", &WebrtcSdpMedia::parseRtcp},
        {"rtcp-fb", &WebrtcSdpMedia::parseRtcpFb},
        {"fmtp", &WebrtcSdpMedia::parseFmtp},
        {"mid", &WebrtcSdpMedia::parseMid},
        {"msid", &WebrtcSdpMedia::parseMsid},
        {"ssrc", &WebrtcSdpMedia::parseSsrc},
        {"ssrc-group", &WebrtcSdpMedia::parseSsrcGroup},
        {"rtcp-mux", &WebrtcSdpMedia::parseRtcpMux},
        {"rtcp-rsize", &WebrtcSdpMedia::parseRtcpRsize},
        {"recvonly", &WebrtcSdpMedia::parseRecvonly},
        {"sendonly", &WebrtcSdpMedia::parseSendonly},
        {"sendrecv", &WebrtcSdpMedia::parseSendrecv},
        {"inactive", &WebrtcSdpMedia::parseInactive},
        {"ice-options", &WebrtcSdpMedia::parseIceOptions},
        {"ice-ufrag", &WebrtcSdpMedia::parseIceUflag},
        {"ice-pwd", &WebrtcSdpMedia::parseIcePwd},
        {"setup", &WebrtcSdpMedia::parseSetup},
        {"extmap-allow-mixed", &WebrtcSdpMedia::parseExtmapAllowMixed},
        {"fingerprint", &WebrtcSdpMedia::parseFingerprint},
        {"candidate", &WebrtcSdpMedia::parseCandidate}
    };
    
    auto iter = sdpMediaHandle.find(attrKey);

    if (iter != sdpMediaHandle.end()) {
        try {
            (this->*(iter->second))(attrValue);
        } catch (exception& ex) {
            logInfo << attrValue << " error: " << ex.what();
        }
    } else {
        logInfo << "unsurpport attr: " << attrKey << ", value: " << attrValue;
    }
}

void WebrtcSdpMedia::parseRtcpMux(const string& value)
{
    rtcpMux_ = true;
}

void WebrtcSdpMedia::parseRtcpRsize(const string& value)
{
    rtcpRsize_ = true;
}

void WebrtcSdpMedia::parseRecvonly(const string& value)
{
    sendRecvType_ = RecvOnly;
}

void WebrtcSdpMedia::parseSendonly(const string& value)
{
    sendRecvType_ = SendOnly;
}

void WebrtcSdpMedia::parseSendrecv(const string& value)
{
    sendRecvType_ = SendRecv;
}

void WebrtcSdpMedia::parseInactive(const string& value)
{
    sendRecvType_ = Inactive;
}

void WebrtcSdpMedia::parseIceOptions(const string& value)
{
    iceOptions_ = value;
}

void WebrtcSdpMedia::parseIceUflag(const string& value)
{
    iceUfrag_ = value;
}

void WebrtcSdpMedia::parseIcePwd(const string& value)
{
    icePwd_ = value;
}

void WebrtcSdpMedia::parseSetup(const string& value)
{
    setup_ = value;
}

void WebrtcSdpMedia::parseExtmapAllowMixed(const string& value)
{
    mixed_ = true;
}

void WebrtcSdpMedia::parseFingerprint(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }
    fingerprintAlg_ = vecValue[0];
    fingerprint_ = vecValue[1];
}

void WebrtcSdpMedia::parseCandidate(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 10) {
        return ;
    }

    auto candidate = make_shared<CandidateInfo>();
    candidate->foundation_ = vecValue[0];
    candidate->componentId_ = stoi(vecValue[1]);
    candidate->transType_ = vecValue[2];
    candidate->priority_ = stoull(vecValue[3]);
    candidate->ip_ = vecValue[4];
    candidate->port_ = stoi(vecValue[5]);
    candidate->candidateType_ = vecValue[6];

    for (auto candidateIter : candidates_) {
        if (candidateIter->foundation_ == candidate->foundation_) {
            return ;
        }
    }
    
    candidates_.push_back(candidate);
}

void WebrtcSdpMedia::parseConnect(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 3) {
        return ;
    }

    string netType = vecValue[0];
    string AddrType = vecValue[1];
    string addr = vecValue[2];
}

void WebrtcSdpMedia::parseExtmap(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    mapExtmap_.emplace(vecValue[1], stoi(vecValue[0]));
}

void WebrtcSdpMedia::parseRtpmap(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    auto payloadType = stoi(vecValue[0]);
    auto payloadDesc = vecValue[1];

    auto iter = mapPtInfo_.find(payloadType);
    if (iter == mapPtInfo_.end()) {
        return ;
    }

    auto vecPtDesc = split(payloadDesc, "/");
    if (vecPtDesc.size() < 2) {
        return ;
    }

    auto ptInfo = iter->second;
    ptInfo->codec_ = vecPtDesc[0];
    ptInfo->samplerate_ = stoi(vecPtDesc[1]);
    if (vecPtDesc.size() > 2) {
        ptInfo->codecExt_ = vecPtDesc[2];
    }
}

void WebrtcSdpMedia::parseRtcp(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 4) {
        return ;
    }

    auto pt = stoi(vecValue[0]);
    auto netType = vecValue[1];
    auto addrType = vecValue[2];
    auto addr = vecValue[3];
}

void WebrtcSdpMedia::parseRtcpFb(const string& value)
{
    auto pos = value.find_first_of(" ");
    auto pt = stoi(value.substr(0, pos));
    auto rtcpFb = value.substr(pos + 1);

    auto iter = mapPtInfo_.find(pt);
    if (iter == mapPtInfo_.end()) {
        return ;
    }

    iter->second->rtcpFbs_.push_back(rtcpFb);
}

void WebrtcSdpMedia::parseFmtp(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    int pt = stoi(vecValue[0]);
    auto iter = mapPtInfo_.find(pt);
    if (iter == mapPtInfo_.end()) {
        return ;
    }

    iter->second->fmtp_ = vecValue[1];
}

void WebrtcSdpMedia::parseMid(const string& value)
{
    mid_ = value;
}

void WebrtcSdpMedia::parseMsid(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    msid_ = vecValue[0];

    for (int i = 1; i < vecValue.size(); ++ i) {
        msidTracker_.push_back(vecValue[i]);
    }
}

void WebrtcSdpMedia::parseSsrc(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    auto ssrc = stoull(vecValue[0]);
    auto& ssrcInfo = mapSsrcInfo_[ssrc];
    if (!ssrcInfo) {
        ssrcInfo = make_shared<SsrcInfo>();
        ssrcInfo->ssrc_ = ssrc;
    }

    auto vecSsrc = split(vecValue[1], ":");
    if (vecSsrc[0] == "msid") {
        ssrcInfo->msid_ = vecSsrc[1];
        for (int i = 2; i < vecValue.size(); ++i) {
            ssrcInfo->msidTracker_.push_back(vecValue[i]);
        }
    } else if (vecSsrc[0] == "cname") {
        ssrcInfo->cname_ = vecSsrc[1];
    } else if (vecSsrc[0] == "lable") {
        ssrcInfo->label_ = vecSsrc[1];
    } else if (vecSsrc[0] == "mslabel") {
        ssrcInfo->mslabel_ = vecSsrc[1];
    }
}

void WebrtcSdpMedia::parseSsrcGroup(const string& value)
{
    auto vecValue = split(value, " ");
    if (vecValue.size() < 2) {
        return ;
    }

    for (int i = 1; i < vecValue.size(); ++i) {
        mapSsrcGroup_[vecValue[0]].push_back(stoull(vecValue[i]));
    }
}

void WebrtcSdpMedia::encode(stringstream& ss)
{
    logInfo << "media pt info size: " << mapPtInfo_.size();

    ss << "m=" << media_ << " " << port_ << " " << protocol_;
    for (auto ptInfo : mapPtInfo_) {
        ss << " " << ptInfo.first;
    }

    ss << "\r\n";
    ss << "c=IN IP4 0.0.0.0\n";

    if (!iceUfrag_.empty()) {
        ss << "a=ice-ufrag:" << iceUfrag_ << "\r\n";
    }
    if (!icePwd_.empty()) {
        ss << "a=ice-pwd:" << icePwd_ << "\r\n";
    }
    if (!iceOptions_.empty()) {
        ss << "a=ice-options:" << iceOptions_ << "\r\n";
    } else {
        ss << "a=ice-options:trickle" << "\r\n";
    }
    if (!fingerprintAlg_.empty() && ! fingerprint_.empty()) {
        ss << "a=fingerprint:" << fingerprintAlg_ << " " << fingerprint_ << "\r\n";
    }
    if (! setup_.empty()) {
        ss << "a=setup:" << setup_ << "\r\n";
    }

    ss << "a=mid:" << mid_ << "\r\n";
    if (!msid_.empty()) {
        ss << "a=msid:" << msid_;
        
        for (auto tracker : msidTracker_) {
            ss << " " << tracker;
        }

        ss << "\r\n";
    }

    switch (sendRecvType_)
    {
    case SendOnly:
        ss << "a=sendonly" << "\r\n";
        break;
    case RecvOnly:
        ss << "a=recvonly" << "\r\n";
        break;
    case SendRecv:
        ss << "a=sendrecv" << "\r\n";
        break;
    case Inactive:
        ss << "a=inactive" << "\r\n";
        break;
    
    default:
        break;
    }

    if (rtcpMux_) {
        ss << "a=rtcp-mux" << "\r\n";
    }

    if (rtcpRsize_) {
        ss << "a=rtcp-rsize" << "\r\n";
    }

    for (auto iter : mapPtInfo_) {
        iter.second->encode(ss);
    }

    for (auto iter : mapSsrcInfo_) {
        iter.second->encode(ss);
    }

	for (auto iter : candidates_) {
        iter->encode(ss);
    }
}

void SsrcInfo::encode(stringstream& ss)
{
    if (ssrc_ == 0) {
		logError << "invalid ssrc";
        return ;
    }

    ss << "a=ssrc:" << ssrc_ << " cname:" << cname_ << "\r\n";
    if (! msid_.empty()) {
        ss << "a=ssrc:" << ssrc_ << " msid:" << msid_;
        for (auto iter : msidTracker_) {
            ss << " " << iter;
        }
        ss << "\r\n";
    }
    if (! mslabel_.empty()) {
        ss << "a=ssrc:" << ssrc_ << " mslabel:" << mslabel_ << "\r\n";
    }
    if (! label_.empty()) {
        ss << "a=ssrc:" << ssrc_ << " label:" << label_ << "\r\n";
    }
}

void WebrtcPtInfo::encode(stringstream& ss)
{
    ss << "a=rtpmap:" << payloadType_ << " " << codec_ << "/" << samplerate_;
    if (!codecExt_.empty()) {
        ss << "/" << codecExt_;
    }
    ss << "\r\n";

    for (auto iter : rtcpFbs_) {
        ss << "a=rtcp-fb:" << payloadType_ << " " << iter << "\r\n";
    }

    if (!fmtp_.empty()) {
        ss << "a=fmtp:" << payloadType_ << " " << fmtp_ << "\r\n";
    }
}

void CandidateInfo::encode(stringstream& ss)
{
    int foundation = 0; /* transType_ and candidateType_ */
    int component_id = 1; /* RTP */

    uint32_t priority = (1<<24)*(126) + (1<<8)*(65535) + (1)*(256 - component_id);

    ss << "a=candidate:" << foundation_ << " "
       << component_id << " " << transType_ << " " << priority << " "
       << ip_ << " " << port_
       << " typ " << candidateType_;
    if (transType_ == "tcp") {
        ss << " tcptype passive\r\n";
    } else {
        ss << " generation 0" << "\r\n";
    }
}

/////////////////////////////WebrtcSdp////////////////////////////

void WebrtcSdp::parse(const string& sdp)
{
    _sdp = sdp;
    int stage = 1;
    _title = make_shared<WebrtcSdpTitle>();
    shared_ptr<WebrtcSdpMedia> media;

    auto sdpLine = split(sdp, "\r\n");
    logInfo << "sdp lines: " << sdpLine.size();
    logInfo << "sdp : " << sdp;
    for (int i = 0; i < sdpLine.size(); ++i) {
        auto &line = sdpLine[i];
        if (line.size() < 2 || line[1] != '=') {
            continue;
        }
        auto key = line[0];
        auto value = line.substr(2);
        if (stage == 1) {
            switch (key) {
                case 'm' : {
                    stage = 2;
                    if (media) {
                        media->index_ = _vecSdpMedia.size();
                        _vecSdpMedia.emplace_back(media);
                    }
                    media = make_shared<WebrtcSdpMedia>();
                    media->parseMediaDesc(value);
                    break;
                }
                case 'v' : {
                    _title->parseVersion(value);
                    break;
                }
                case 'o' : {
                    _title->parseOrigin(value);
                    break;
                }
                case 's' : {
                    _title->parseSession(value);
                    break;
                }
                case 't' : {
                    _title->parseTime(value);
                    break;
                }
                case 'a' : {
                    _title->mapAttr_.emplace(to_string(key), value);
                    _title->parseAttr(value);
                    break;
                }
                default : {
                    _title->mapTitle_.emplace(key, value);
                }
            }
        } else if (stage == 2) {
            switch (key) {
                case 'm' : {
                    if (media) {
                        media->index_ = _vecSdpMedia.size();
                        _vecSdpMedia.emplace_back(media);
                    }
                    media = make_shared<WebrtcSdpMedia>();
                    media->parseMediaDesc(value);
                    break;
                }
                case 'a' : {
                    media->mapAttr_.emplace(to_string(key), value);
                    media->parseAttr(value);
                    break;
                }
                case 'c' :{
                    media->parseConnect(value);
                    break;
                }
                default : {
                    media->mapMedia_.emplace(key, value);
                }
            }
        }
    }
    if (media) {
        media->index_ = _vecSdpMedia.size();
        _vecSdpMedia.emplace_back(media);
    }
}

string WebrtcSdp::getSdp()
{
    if (!_sdp.empty()) {
        return _sdp;
    }

    stringstream ss;
    _title->encode(ss);

    logInfo << "sdp title: " << ss.str();
    logInfo << "sdp media size: " << _vecSdpMedia.size();

    for (auto& sdpMedia : _vecSdpMedia) {
        sdpMedia->encode(ss);
    }

    return ss.str();
}

void WebrtcSdp::addCandidate(const CandidateInfo::Ptr& info)
{
    for (auto iter: _vecSdpMedia) {
        iter->candidates_.push_back(info);
    }
}
