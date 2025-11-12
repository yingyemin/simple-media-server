#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <iomanip>

#include "OpusTrack.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

OpusTrack::OpusTrack()
{
	payloadType_ = PayloadType_OPUS;
	samplerate_ = 48000;
	channel_ = 1;
	bitPerSample_ = 8;
}

string OpusTrack::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	   << "a=rtpmap:" << payloadType_ << " OPUS/" << samplerate_ << "/" << channel_ << "\r\n"
	   << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

OpusTrack::Ptr OpusTrack::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<OpusTrack>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "opus";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "audio";
    trackInfo->samplerate_ = samplerate;
    trackInfo->bitPerSample_ = 16;

    return trackInfo;
}

void OpusTrack::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("opus", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<OpusTrack>();
		trackInfo->index_ = AudioTrackType;
		trackInfo->codec_ = "opus";
		trackInfo->payloadType_ = PayloadType_OPUS;
		trackInfo->trackType_ = "audio";
		trackInfo->samplerate_ = 48000;

		return trackInfo;
	});
}
