#ifndef Webrtc_H
#define Webrtc_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"

using namespace std;

enum { 
    kStunPkt, 
    kDtlsPkt, 
    kRtpPkt, 
    kRtcpPkt, 
    kUnkown
};

int guessType(const StreamBuffer::Ptr& buffer);

#endif //Ehome2Connection_H
