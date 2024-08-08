#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>
#include <sstream>

#include "Webrtc.h"

using namespace std;

int guessType(const StreamBuffer::Ptr& buffer)
{
    uint8_t* data = (uint8_t*)buffer->data();
	size_t len = buffer->size();

	// 0x0001 stun bind request
	if (data[0] == 0 && data[1] == 1) {
		return kStunPkt;
    }

	if (len >= 13 && data[0] >= 16 && data[0] <= 64) {
		return kDtlsPkt;
    }

	// 0x80
	if (len >= 12 && (data[0] & 0xC0) == 0x80) {
		return (data[1] < 128 || data[1] > 223) ? kRtpPkt : kRtcpPkt;
	}
		
	return kUnkown;
}
