#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <iomanip>

#include "AdpcmaTrack.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

AdpcmaTrack::AdpcmaTrack()
{
	payloadType_ = PayloadType_ADPCMA;
	samplerate_ = 8000;
	channel_ = 1;
	bitPerSample_ = 8;
}

string AdpcmaTrack::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	   << "a=rtpmap:" << payloadType_ << " ADPCM/" << samplerate_ << "/" << channel_ << "\r\n"
	   << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

AdpcmaTrack::Ptr AdpcmaTrack::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<AdpcmaTrack>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "adpcma";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "audio";
    trackInfo->samplerate_ = samplerate;
    trackInfo->bitPerSample_ = 16;

    return trackInfo;
}

void AdpcmaTrack::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("adpcma", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<AdpcmaTrack>();
		trackInfo->index_ = AudioTrackType;
		trackInfo->codec_ = "adpcma";
		trackInfo->payloadType_ = PayloadType_ADPCMA;
		trackInfo->trackType_ = "audio";
		trackInfo->samplerate_ = 8000;

		return trackInfo;
	});
}