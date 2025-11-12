#ifndef Webrtc_H
#define Webrtc_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"

// using namespace std;

enum { 
    kStunPkt, 
    kDtlsPkt, 
    kRtpPkt, 
    kRtcpPkt, 
    kUnkown
};

int guessType(const StreamBuffer::Ptr& buffer);

#define AudioLevelUrl "urn:ietf:params:rtp-hdrext:ssrc-audio-level"
#define AbsoluteSendTimeUrl "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"
#define TWCCUrl "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
#define MidUrl "urn:ietf:params:rtp-hdrext:sdes:mid"
#define RtpStreamIdUrl "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id"
#define RepairedRtpStreamIdUrl "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id"
#define VideoTimingUrl "http://www.webrtc.org/experiments/rtp-hdrext/video-timing"
#define ColorSpaceUrl "http://www.webrtc.org/experiments/rtp-hdrext/color-space"
#define CsrcAudioLevelUrl "urn:ietf:params:rtp-hdrext:csrc-audio-level"
#define FrameMarkingUrl "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07"
#define VideoContentTypeUrl "http://www.webrtc.org/experiments/rtp-hdrext/video-content-type"
#define PlayoutDelayUrl "http://www.webrtc.org/experiments/rtp-hdrext/playout-delay"
#define VideoRotationUrl "urn:3gpp:video-orientation"
#define TOffsetUrl "urn:ietf:params:rtp-hdrext:toffset"
#define Av1Url "https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension"
#define EncryptUrl "urn:ietf:params:rtp-hdrext:encryp"

#endif //Ehome2Connection_H
