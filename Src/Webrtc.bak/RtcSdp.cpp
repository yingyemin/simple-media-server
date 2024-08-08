/*
 * 
 *
 * 
 *
 * 
 * 
 * 
 */


#include "RtcSdp.h"


#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <deque>
#include "Common/config.h"

using namespace std;

const std::string kCRLF = "\r\n";

#define FETCH(is,word) \
if (! (is >> word)) {\
	logError << "fetch failed";\
    return ERROR_RTC_SDP_DECODE;\
}\

#define FETCH_WITH_DELIM(is,word,delim) \
if (! getline(is,word,delim)) {\
	logError << "fetch with delim failed";\
    return ERROR_RTC_SDP_DECODE;\
}\

static std::vector<std::string> split_str(const std::string& str, const std::string& delim)
{
    std::vector<std::string> ret;
    size_t pre_pos = 0;
    std::string tmp;
    size_t pos = 0;
    do {
        pos = str.find(delim, pre_pos);
        tmp = str.substr(pre_pos, pos - pre_pos);
        ret.push_back(tmp);
        pre_pos = pos + delim.size();
    } while (pos != std::string::npos);

    return ret;
}

static void skip_first_spaces(std::string& str)
{
    while (! str.empty() && str[0] == ' ') {
        str.erase(0, 1);
    }
}

srs_error_t parse_h264_fmtp(const std::string& fmtp, H264SpecificParam& h264_param)
{
    srs_error_t err = srs_success;
    std::vector<std::string> vec = split_str(fmtp, ";");
    for (size_t i = 0; i < vec.size(); ++i) {
        std::vector<std::string> kv = split_str(vec[i], "=");
        if (kv.size() == 2) {
            if (kv[0] == "profile-level-id") {
                h264_param.profile_level_id = kv[1];
            } else if (kv[0] == "packetization-mode") {
                // 6.3.  Non-Interleaved Mode
                // This mode is in use when the value of the OPTIONAL packetization-mode
                // media type parameter is equal to 1.  This mode SHOULD be supported.
                // It is primarily intended for low-delay applications.  Only single NAL
                // unit packets, STAP-As, and FU-As MAY be used in this mode.  STAP-Bs,
                // MTAPs, and FU-Bs MUST NOT be used.  The transmission order of NAL
                // units MUST comply with the NAL unit decoding order.
                // @see https://tools.ietf.org/html/rfc6184#section-6.3
                h264_param.packetization_mode = kv[1];
            } else if (kv[0] == "level-asymmetry-allowed") {
                h264_param.level_asymmerty_allow = kv[1];
            } else {
				logError << "invalid h264 param=" << kv[0];
                return ERROR_RTC_SDP_DECODE;
            }
        } else {
			logError << "invalid h264 param=" << vec[i];
            return ERROR_RTC_SDP_DECODE;
        }
    }

    return err;
}


SrsSessionInfo::SrsSessionInfo()
{
}

SrsSessionInfo::~SrsSessionInfo()
{
}

srs_error_t SrsSessionInfo::parse_attribute(const std::string& attribute, const std::string& value)
{
    srs_error_t err = srs_success;
    if (attribute == "ice-ufrag") {
        ice_ufrag_ = value;
    } else if (attribute == "ice-pwd") {
        ice_pwd_ = value;
    } else if (attribute == "ice-options") {
        ice_options_ = value;
    } else if (attribute == "fingerprint") {
        std::istringstream is(value);
        FETCH(is, fingerprint_algo_);
        FETCH(is, fingerprint_);
    } else if (attribute == "setup") {
        // @see: https://tools.ietf.org/html/rfc4145#section-4
        setup_ = value;
    } else {
		logInfo << "ignore attribute=" << attribute << ", value=" << value;
    }

    return err;
}

srs_error_t SrsSessionInfo::encode(std::ostringstream& os)
{
    srs_error_t err = srs_success;
    if (! ice_ufrag_.empty()) {
        os << "a=ice-ufrag:" << ice_ufrag_ << kCRLF;
    }
    if (! ice_pwd_.empty()) {
        os << "a=ice-pwd:" << ice_pwd_ << kCRLF;
    }
    if (! ice_options_.empty()) {
        os << "a=ice-options:" << ice_options_ << kCRLF;
    } else {
		// @see: https://webrtcglossary.com/trickle-ice/
        // Trickle ICE is an optimization of the ICE specification for NAT traversal.
        os << "a=ice-options:trickle" << kCRLF;
    }
    if (! fingerprint_algo_.empty() && ! fingerprint_.empty()) {
        os << "a=fingerprint:" << fingerprint_algo_ << " " << fingerprint_ << kCRLF;
    }
    if (! setup_.empty()) {
        os << "a=setup:" << setup_ << kCRLF;
    }

    return err;
}

bool SrsSessionInfo::operator=(const SrsSessionInfo& rhs)
{
    return ice_ufrag_        == rhs.ice_ufrag_ &&
           ice_pwd_          == rhs.ice_pwd_ &&
           ice_options_      == rhs.ice_options_ &&
           fingerprint_algo_ == rhs.fingerprint_algo_ &&
           fingerprint_      == rhs.fingerprint_ &&
           setup_            == rhs.setup_;
}

SrsSSRCInfo::SrsSSRCInfo()
{
    ssrc_ = 0;
}

SrsSSRCInfo::~SrsSSRCInfo()
{
}

srs_error_t SrsSSRCInfo::encode(std::ostringstream& os)
{
    srs_error_t err = srs_success;
    if (ssrc_ == 0) {
		logError << "invalid ssrc";
        return ERROR_RTC_SDP_DECODE;
    }

    os << "a=ssrc:" << ssrc_ << " cname:" << cname_ << kCRLF;
    if (! msid_.empty()) {
        os << "a=ssrc:" << ssrc_ << " msid:" << msid_;
        if (! msid_tracker_.empty()) {
            os << " " << msid_tracker_;
        }
        os << kCRLF;
    }
    if (! mslabel_.empty()) {
        os << "a=ssrc:" << ssrc_ << " mslabel:" << mslabel_ << kCRLF;
    }
    if (! label_.empty()) {
        os << "a=ssrc:" << ssrc_ << " label:" << label_ << kCRLF;
    }

    return err;
}

SrsMediaPayloadType::SrsMediaPayloadType(int payload_type)
{
    payload_type_ = payload_type;
}


SrsMediaPayloadType::~SrsMediaPayloadType()
{
}

srs_error_t SrsMediaPayloadType::encode(std::ostringstream& os)
{
    srs_error_t err = srs_success;

    os << "a=rtpmap:" << payload_type_ << " " << encoding_name_ << "/" << clock_rate_;
    if (! encoding_param_.empty()) {
        os << "/" << encoding_param_;
    }
    os << kCRLF;

    for (std::vector<std::string>::iterator iter = rtcp_fb_.begin(); iter != rtcp_fb_.end(); ++iter) {
        os << "a=rtcp-fb:" << payload_type_ << " " << *iter << kCRLF;
    }

    if (! format_specific_param_.empty()) {
        os << "a=fmtp:" << payload_type_ << " " << format_specific_param_ << kCRLF;
    }

    return err;
}

SrsMediaDesc::SrsMediaDesc(const std::string& type)
{
    type_ = type;

    rtcp_mux_ = false;

    sendrecv_ = false;
    recvonly_ = false;
    sendonly_ = false;
    inactive_ = false;
}

SrsMediaDesc::~SrsMediaDesc()
{
}

SrsMediaPayloadType* SrsMediaDesc::find_media_with_payload_type(int payload_type)
{
    for (size_t i = 0; i < payload_types_.size(); ++i) {
        if (payload_types_[i].payload_type_ == payload_type) {
            return &payload_types_[i];
        }
    }

    return NULL;
}

vector<SrsMediaPayloadType> SrsMediaDesc::find_media_with_encoding_name(const std::string& encoding_name) const
{
    std::vector<SrsMediaPayloadType> payloads;

    for (size_t i = 0; i < payload_types_.size(); ++i) {
        if (payload_types_[i].encoding_name_ == encoding_name) {
            payloads.push_back(payload_types_[i]);
        }
    }

    return payloads;
}


srs_error_t SrsMediaDesc::parse_line(const std::string& line)
{
    srs_error_t err = srs_success;
    std::string content = line.substr(2);

    switch (line[0]) {
        case 'a': {
            return parse_attribute(content);
        }
        case 'c': {
            // TODO: process c-line
            break;
        }
        default: {
			logInfo << "ignore media line=" << line;
            break;
        }
    }

    return err;
}

srs_error_t SrsMediaDesc::encode(std::ostringstream& os)
{
    srs_error_t err = srs_success;

    os << "m=" << type_ << " " << port_ << " " << protos_;
    for (std::vector<SrsMediaPayloadType>::iterator iter = payload_types_.begin(); iter != payload_types_.end(); ++iter) {
        os << " " << iter->payload_type_;
    }

    os << kCRLF;

    // TODO:nettype and address type
    os << "c=IN IP4 0.0.0.0" << kCRLF;

    if ((err = session_info_.encode(os)) != srs_success) {
		logError << "encode session info failed";
		return err;
    }

    os << "a=mid:" << mid_ << kCRLF;
    if (! msid_.empty()) {
        os << "a=msid:" << msid_;
        
        if (! msid_tracker_.empty()) {
            os << " " << msid_tracker_;
        }

        os << kCRLF;
    }

    if (sendonly_) {
        os << "a=sendonly" << kCRLF;
    }
    if (recvonly_) {
        os << "a=recvonly" << kCRLF;
    }
    if (sendrecv_) {
        os << "a=sendrecv" << kCRLF;
    }
    if (inactive_) {
        os << "a=inactive" << kCRLF;
    }

    if (rtcp_mux_) {
        os << "a=rtcp-mux" << kCRLF;
    }

    if (rtcp_rsize_) {
        os << "a=rtcp-rsize" << kCRLF;
    }

    for (std::vector<SrsMediaPayloadType>::iterator iter = payload_types_.begin(); iter != payload_types_.end(); ++iter) {
        if ((err = iter->encode(os)) != srs_success) {
			logError << "encode media payload failed";
			return err;
        }
    }

    for (std::vector<SrsSSRCInfo>::iterator iter = ssrc_infos_.begin(); iter != ssrc_infos_.end(); ++iter) {
        SrsSSRCInfo& ssrc_info = *iter;

        if ((err = ssrc_info.encode(os)) != srs_success) {
			logError << "encode ssrc failed";
			return err;
        }
    }

    int foundation = 0;
    int component_id = 1; /* RTP */
	for (std::vector<SrsCandidate>::iterator iter = candidates_.begin(); iter != candidates_.end(); ++iter) {
        // @see: https://tools.ietf.org/html/draft-ietf-ice-rfc5245bis-00#section-4.2
        uint32_t priority = (1<<24)*(126) + (1<<8)*(65535) + (1)*(256 - component_id);

        // @see: https://tools.ietf.org/id/draft-ietf-mmusic-ice-sip-sdp-14.html#rfc.section.5.1
        os << "a=candidate:" << foundation++ << " "
           << component_id << " udp " << priority << " "
           << iter->ip_ << " " << iter->port_
           << " typ " << iter->type_ 
           << " generation 0" << kCRLF;
    }

    return err;
}

srs_error_t SrsMediaDesc::parse_attribute(const std::string& content)
{
    srs_error_t err = srs_success;
    std::string attribute = "";
    std::string value = "";
    size_t pos = content.find_first_of(":");

    if (pos != std::string::npos) {
        attribute = content.substr(0, pos);
        value = content.substr(pos + 1);
    } else {
        attribute = content;
    }

    if (attribute == "extmap") {
        // TODO:We don't parse "extmap" currently.
        return 0;
    } else if (attribute == "rtpmap") {
        return parse_attr_rtpmap(value);
    } else if (attribute == "rtcp") {
        return parse_attr_rtcp(value);
    } else if (attribute == "rtcp-fb") {
        return parse_attr_rtcp_fb(value);
    } else if (attribute == "fmtp") {
        return parse_attr_fmtp(value);
    } else if (attribute == "mid") {
        return parse_attr_mid(value);
    } else if (attribute == "msid") {
        return parse_attr_msid(value);
    } else if (attribute == "ssrc") {
        return parse_attr_ssrc(value);
    } else if (attribute == "ssrc-group") {
        return parse_attr_ssrc_group(value);
    } else if (attribute == "rtcp-mux") {
        rtcp_mux_ = true;
    } else if (attribute == "rtcp-rsize") {
        rtcp_rsize_ = true;
    } else if (attribute == "recvonly") {
        recvonly_ = true;
    } else if (attribute == "sendonly") {
        sendonly_ = true;
    } else if (attribute == "sendrecv") {
        sendrecv_ = true;
    } else if (attribute == "inactive") {
        inactive_ = true;
	} else {
        return session_info_.parse_attribute(attribute, value);
    }

    return err;
}

srs_error_t SrsMediaDesc::parse_attr_rtpmap(const std::string& value)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc4566#page-25
    // a=rtpmap:<payload type> <encoding name>/<clock rate> [/<encoding parameters>]

    std::istringstream is(value);

    int payload_type = 0;
    FETCH(is, payload_type);

    SrsMediaPayloadType* payload = find_media_with_payload_type(payload_type);
    if (payload == NULL) {
		logError << "can not find payload [" << payload_type << "] when pase rtpmap";
        return ERROR_RTC_SDP_DECODE;
    }

    std::string word;
    FETCH(is, word);

    std::vector<std::string> vec = split_str(word, "/");
    if (vec.size() < 2) {
		logError << "invalid rtpmap line=" << value;
        return ERROR_RTC_SDP_DECODE;
    }

    payload->encoding_name_ = vec[0];
    payload->clock_rate_ = atoi(vec[1].c_str());

    if (vec.size() == 3) {
        payload->encoding_param_ = vec[2];
    }

    return err;
}

srs_error_t SrsMediaDesc::parse_attr_rtcp(const std::string& value)
{
    srs_error_t err = srs_success;

    // TODO:parse rtcp attribute

    return err;
}

srs_error_t SrsMediaDesc::parse_attr_rtcp_fb(const std::string& value)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc5104#section-7.1

    std::istringstream is(value);

    int payload_type = 0;
    FETCH(is, payload_type);

    SrsMediaPayloadType* payload = find_media_with_payload_type(payload_type);
    if (payload == NULL) {
		logError << "can not find payload [" << payload_type << "] when pase rtcp-fb";
        return ERROR_RTC_SDP_DECODE;
    }

    std::string rtcp_fb = is.str().substr(is.tellg());
    skip_first_spaces(rtcp_fb);

    payload->rtcp_fb_.push_back(rtcp_fb);

    return err;
}

srs_error_t SrsMediaDesc::parse_attr_fmtp(const std::string& value)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc4566#page-30
    // a=fmtp:<format> <format specific parameters>

    std::istringstream is(value);

    int payload_type = 0;
    FETCH(is, payload_type);

    SrsMediaPayloadType* payload = find_media_with_payload_type(payload_type);
    if (payload == NULL) {
		logError << "can not find payload [" << payload_type << "] when pase fmtp";
        return ERROR_RTC_SDP_DECODE;
    }

    std::string word;
    FETCH(is, word);

    payload->format_specific_param_ = word;

    return err;
}

srs_error_t SrsMediaDesc::parse_attr_mid(const std::string& value)
{
    // @see: https://tools.ietf.org/html/rfc3388#section-3
    srs_error_t err = srs_success;
    std::istringstream is(value);
    // mid_ means m-line id
    FETCH(is, mid_);
    return err;
}

srs_error_t SrsMediaDesc::parse_attr_msid(const std::string& value)
{
    // @see: https://tools.ietf.org/id/draft-ietf-mmusic-msid-08.html#rfc.section.2
    // TODO: msid and msid_tracker
    srs_error_t err = srs_success;
    std::istringstream is(value);
    // msid_ means media stream id
    FETCH(is, msid_);
    is >> msid_tracker_;
    return err;
}

srs_error_t SrsMediaDesc::parse_attr_ssrc(const std::string& value)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc5576#section-4.1

    std::istringstream is(value);

    uint32_t ssrc = 0;
    FETCH(is, ssrc);

    std::string ssrc_attr = "";
    FETCH_WITH_DELIM(is, ssrc_attr, ':');
    skip_first_spaces(ssrc_attr);

    std::string ssrc_value = is.str().substr(is.tellg());
    skip_first_spaces(ssrc_value);

    SrsSSRCInfo& ssrc_info = fetch_or_create_ssrc_info(ssrc);

    if (ssrc_attr == "cname") {
        // @see: https://tools.ietf.org/html/rfc5576#section-6.1
        ssrc_info.cname_ = ssrc_value;
        ssrc_info.ssrc_ = ssrc;
    } else if (ssrc_attr == "msid") {
        std::vector<std::string> vec = split_str(ssrc_value, " ");
        if (vec.empty()) {
			logError << "invalid ssrc line=" << value;
            return ERROR_RTC_SDP_DECODE;
        }

        ssrc_info.msid_ = vec[0];
        if (vec.size() > 1) {
            ssrc_info.msid_tracker_ = vec[1];
        }
    } else if (ssrc_attr == "mslabel") {
        ssrc_info.mslabel_ = ssrc_value;
    } else if (ssrc_attr == "label") {
        ssrc_info.label_ = ssrc_value;
    }

    return err;
}

srs_error_t SrsMediaDesc::parse_attr_ssrc_group(const std::string& value)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc5576#section-4.2
    // a=ssrc-group:<semantics> <ssrc-id> ...

    std::istringstream is(value);

    std::string semantics;
    FETCH(is, semantics);

    // TODO: ssrc group process
    if (semantics == "FID") {
    }

    return err;
}

SrsSSRCInfo& SrsMediaDesc::fetch_or_create_ssrc_info(uint32_t ssrc)
{
    for (size_t i = 0; i < ssrc_infos_.size(); ++i) {
        if (ssrc_infos_[i].ssrc_ == ssrc) {
            return ssrc_infos_[i];
        }
    }

    SrsSSRCInfo ssrc_info;
    ssrc_info.ssrc_ = ssrc;
    ssrc_infos_.push_back(ssrc_info);

    return ssrc_infos_.back();
}

SrsSdp::SrsSdp()
{
    in_media_session_ = false;
    
    start_time_ = 0;
    end_time_ = 0;
}

SrsSdp::~SrsSdp()
{
}

srs_error_t SrsSdp::parse(const std::string& sdp_str)
{
    srs_error_t err = srs_success;

    // All webrtc SrsSdp annotated example
    // @see: https://tools.ietf.org/html/draft-ietf-rtcweb-SrsSdp-11
    // Sdp example
    // session info
    // v=
    // o=
    // s=
    // t=
    // media description
    // m=
    // a=
    // ...
    // media description
    // m=
    // a=
    // ...
    std::istringstream is(sdp_str);
    std::string line;
    while (getline(is, line)) {
        if (line.size() < 2 || line[1] != '=') {
			logError << "invalid sdp line=" << line;
			return ERROR_RTC_SDP_DECODE;
        }
        if (! line.empty() && line[line.size()-1] == '\r') {
            line.erase(line.size()-1, 1);
        }

        if ((err = parse_line(line)) != srs_success) {
			logError << "parse sdp line failed";
            return err;
        }
    }

    return err;
}

srs_error_t SrsSdp::encode(std::ostringstream& os)
{
    srs_error_t err = srs_success;

    os << "v=" << version_ << kCRLF;
    os << "o=" << username_ << " " << session_id_ << " " << session_version_ << " " << nettype_ << " " << addrtype_ << " " << unicast_address_ << kCRLF;
    os << "s=" << session_name_ << kCRLF;
    os << "t=" << start_time_ << " " << end_time_ << kCRLF;
    // ice-lite is a minimal version of the ICE specification, intended for servers running on a public IP address.
    os << "a=ice-lite" << kCRLF;

    if (! groups_.empty()) {
        os << "a=group:" << group_policy_;
        for (std::vector<std::string>::iterator iter = groups_.begin(); iter != groups_.end(); ++iter) {
            os << " " << *iter;
        }
        os << kCRLF;
    }

    os << "a=msid-semantic: " << msid_semantic_;
    for (std::vector<std::string>::iterator iter = msids_.begin(); iter != msids_.end(); ++iter) {
        os << " " << *iter;
    }
    os << kCRLF;

    if ((err = session_info_.encode(os)) != srs_success) {
		logError << "encode session info failed";
		return err;
    }

    for (std::vector<SrsMediaDesc>::iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        if ((err = (*iter).encode(os)) != srs_success) {
			logError << "encode media description failed";
			return err;
        }
    }

    return err;
}

const SrsMediaDesc* SrsSdp::find_media_desc(const std::string& type) const
{
    for (std::vector<SrsMediaDesc>::const_iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        if (iter->type_ == type) {
            return &(*iter);
        }
    }

    return NULL;
}

void SrsSdp::set_ice_ufrag(const std::string& ufrag)
{
    for (std::vector<SrsMediaDesc>::iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        iter->session_info_.ice_ufrag_ = ufrag;
    }
}

void SrsSdp::set_ice_pwd(const std::string& pwd)
{
    for (std::vector<SrsMediaDesc>::iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        iter->session_info_.ice_pwd_ = pwd;
    }
}

void SrsSdp::set_fingerprint_algo(const std::string& algo)
{
    for (std::vector<SrsMediaDesc>::iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        iter->session_info_.fingerprint_algo_ = algo;
    }
}

void SrsSdp::set_fingerprint(const std::string& fingerprint)
{
    for (std::vector<SrsMediaDesc>::iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        iter->session_info_.fingerprint_ = fingerprint;
    }
}

void SrsSdp::add_candidate(const std::string& ip, const int& port, const std::string& type)
{
    // @see: https://tools.ietf.org/id/draft-ietf-mmusic-ice-sip-sdp-14.html#rfc.section.5.1
    SrsCandidate candidate;
    candidate.ip_ = ip;
    candidate.port_ = port;
    candidate.type_ = type;

    for (std::vector<SrsMediaDesc>::iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
    	iter->candidates_.push_back(candidate);
    }
}

std::string SrsSdp::get_ice_ufrag() const
{
    // Becaues we use BUNDLE, so we can choose the first element.
    for (std::vector<SrsMediaDesc>::const_iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        return iter->session_info_.ice_ufrag_;
    }

    return "";
}

std::string SrsSdp::get_ice_pwd() const
{
    // Becaues we use BUNDLE, so we can choose the first element.
    for (std::vector<SrsMediaDesc>::const_iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        return iter->session_info_.ice_pwd_;
    }

    return "";
}

int SrsSdp::get_video_pt(){
    int pt = 0;
    for (std::vector<SrsMediaDesc>::const_iterator iter = media_descs_.begin(); iter != media_descs_.end(); ++iter) {
        if(iter->is_video()){
            pt =  iter->get_payload_type();
            break;
        }
    }
    return pt;
}

srs_error_t SrsSdp::parse_line(const std::string& line)
{
    srs_error_t err = srs_success;

    std::string content = line.substr(2);

    switch (line[0]) {
        case 'o': {
            return parse_origin(content);
        }
        case 'v': {
            return parse_version(content);
        }
        case 's': {
            return parse_session_name(content);
        }
        case 't': {
            return parse_timing(content);
        }
        case 'a': {
            if (in_media_session_) {
                return media_descs_.back().parse_line(line);
            }
            return parse_attribute(content);
        }
        case 'm': {
            return parse_media_description(content);
        }
        case 'c': {
            // TODO: process c-line
            break;
        }
        default: {
        	logInfo << "ignore sdp line=" << line;
            break;
        }
    }

    return err;
}

srs_error_t SrsSdp::parse_origin(const std::string& content)
{
    srs_error_t err = srs_success;

    // @see: https://tools.ietf.org/html/rfc4566#section-5.2
    // o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
    // eg. o=- 9164462281920464688 2 IN IP4 127.0.0.1
    std::istringstream is(content);

    FETCH(is, username_);
    FETCH(is, session_id_);
    FETCH(is, session_version_);
    FETCH(is, nettype_);
    FETCH(is, addrtype_);
    FETCH(is, unicast_address_);

    return err;
}

srs_error_t SrsSdp::parse_version(const std::string& content)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc4566#section-5.1

    std::istringstream is(content);

    FETCH(is, version_);

    return err;
}

srs_error_t SrsSdp::parse_session_name(const std::string& content)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc4566#section-5.3
    // s=<session name>

    session_name_ = content;

    return err;
}

srs_error_t SrsSdp::parse_timing(const std::string& content)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc4566#section-5.9
    // t=<start-time> <stop-time>
    
    std::istringstream is(content);

    FETCH(is, start_time_);
    FETCH(is, end_time_);

    return err;
}

srs_error_t SrsSdp::parse_attribute(const std::string& content)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc4566#section-5.13
    // a=<attribute>
    // a=<attribute>:<value>

    std::string attribute = "";
    std::string value = "";
    size_t pos = content.find_first_of(":");

    if (pos != std::string::npos) {
        attribute = content.substr(0, pos);
        value = content.substr(pos + 1);
    }

    if (attribute == "group") {
        return parse_attr_group(value);
    } else if (attribute == "msid-semantic") {
        std::istringstream is(value);
        FETCH(is, msid_semantic_);

        std::string msid;
        while (is >> msid) {
            msids_.push_back(msid);
        }
    } else {
        return session_info_.parse_attribute(attribute, value);
    }

    return err;
}

srs_error_t SrsSdp::parse_attr_group(const std::string& value)
{
    srs_error_t err = srs_success;
    // @see: https://tools.ietf.org/html/rfc5888#section-5

    std::istringstream is(value);

    FETCH(is, group_policy_);

    std::string word;
    while (is >> word) {
        groups_.push_back(word);
    }

    return err;
}

srs_error_t SrsSdp::parse_media_description(const std::string& content)
{
    srs_error_t err = srs_success;

    // @see: https://tools.ietf.org/html/rfc4566#section-5.14
    // m=<media> <port> <proto> <fmt> ...
    // m=<media> <port>/<number of ports> <proto> <fmt> ...
    std::istringstream is(content);

    std::string media;
    FETCH(is, media);

    int port;
    FETCH(is, port);

    std::string proto;
    FETCH(is, proto);

    media_descs_.push_back(SrsMediaDesc(media));
    media_descs_.back().protos_ = proto;
    media_descs_.back().port_ = port;

    int fmt;
    while (is >> fmt) {
        media_descs_.back().payload_types_.push_back(SrsMediaPayloadType(fmt));
    }

    if (! in_media_session_) {
        in_media_session_ = true;
    }

    return err;
}


int ExchangeSdp(const std::string msid, const SrsSdp& remote_sdp, SrsSdp& local_sdp) {
	srs_error_t err = srs_success;
	
	local_sdp.version_ 		   = "0";
    local_sdp.username_        = "vss-streamingserver";
    local_sdp.session_id_      = "1234567890";
    local_sdp.session_version_ = "2";
    local_sdp.nettype_         = "IN";
    local_sdp.addrtype_        = "IP4";
    local_sdp.unicast_address_ = "0.0.0.0";

    local_sdp.session_name_ = "live_play_session";

    local_sdp.msid_semantic_ = "WMS";
    local_sdp.msids_.push_back(msid);

    local_sdp.group_policy_ = "BUNDLE";

    for (size_t i = 0; i < remote_sdp.media_descs_.size(); ++i) {
        const SrsMediaDesc& remote_media_desc = remote_sdp.media_descs_[i];

        if (remote_media_desc.is_audio()) {
            local_sdp.media_descs_.push_back(SrsMediaDesc("audio"));
        } else if (remote_media_desc.is_video()) {
            local_sdp.media_descs_.push_back(SrsMediaDesc("video"));
        }

        SrsMediaDesc& local_media_desc = local_sdp.media_descs_.back();

        if (remote_media_desc.is_audio()) {
            // TODO: check opus format specific param
            std::vector<SrsMediaPayloadType> payloads = remote_media_desc.find_media_with_encoding_name("PCMA");
            for (std::vector<SrsMediaPayloadType>::iterator iter = payloads.begin(); iter != payloads.end(); ++iter) {
                // Only choose one match opus codec.
                local_media_desc.payload_types_.push_back(*iter);
                break;
            }

            if (local_media_desc.payload_types_.empty()) {
				logError << "no found valid opus payload type";
                return -1;
            }

        } else if (remote_media_desc.is_video()) {
            std::deque<SrsMediaPayloadType> backup_payloads;
            std::vector<SrsMediaPayloadType> payloads = remote_media_desc.find_media_with_encoding_name("H264");
            for (std::vector<SrsMediaPayloadType>::iterator iter = payloads.begin(); iter != payloads.end(); ++iter) {
                if (iter->format_specific_param_.empty()) {
                    backup_payloads.push_front(*iter);
                    continue;
                }
				
                H264SpecificParam h264_param;
                if ((err = parse_h264_fmtp(iter->format_specific_param_, h264_param)) != srs_success) {
                    continue;
                }

                // Try to pick the "best match" H.264 payload type.
                if (h264_param.profile_level_id == "42e01f" && h264_param.packetization_mode == "1" && h264_param.level_asymmerty_allow == "1") {
                    // Only choose first match H.264 payload type.
                    local_media_desc.payload_types_.push_back(*iter);
                    break;
                }

                backup_payloads.push_back(*iter);
            }

            // Try my best to pick at least one media payload type.
            if (local_media_desc.payload_types_.empty() && ! backup_payloads.empty()) {
                logWarn << "choose backup H.264 payload type [" << backup_payloads.front().payload_type_ << "]";
                local_media_desc.payload_types_.push_back(backup_payloads.front());
            }

            if (local_media_desc.payload_types_.empty()) {
				logError << "no found valid H.264 payload type";
				return ERROR_RTC_SDP_EXCHANGE;
            }
        }

        local_media_desc.mid_ = remote_media_desc.mid_;
        local_sdp.groups_.push_back(local_media_desc.mid_);

        local_media_desc.port_ = 9;
        //local_media_desc.protos_ = "RTP/AVPF";
		local_media_desc.protos_ = "UDP/TLS/RTP/SAVPF";

        if (remote_media_desc.session_info_.setup_ == "active") {
            local_media_desc.session_info_.setup_ = "passive";
        } else if (remote_media_desc.session_info_.setup_ == "passive") {
            local_media_desc.session_info_.setup_ = "active";
        } else if (remote_media_desc.session_info_.setup_ == "actpass") {
            local_media_desc.session_info_.setup_ = "passive";
        }

        if (remote_media_desc.sendonly_) {
            local_media_desc.recvonly_ = true;
        } else if (remote_media_desc.recvonly_) {
            local_media_desc.sendonly_ = true;
        } else if (remote_media_desc.sendrecv_) {
            local_media_desc.sendrecv_ = true;
        }

        local_media_desc.rtcp_mux_ = true;
        local_media_desc.rtcp_rsize_ = true;

        SrsSSRCInfo ssrc_info;
		
        ssrc_info.ssrc_ = remote_media_desc.is_audio() ? 10000 : 20000;
		ssrc_info.cname_ = "test_sdp_cname";
        local_media_desc.ssrc_infos_.push_back(ssrc_info);
    }

	return 0;
}
