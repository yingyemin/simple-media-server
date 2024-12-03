#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <iomanip>

#include "G711Track.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

G711aTrack::G711aTrack()
{
	payloadType_ = PayloadType_G711A;
	samplerate_ = 8000;
	channel_ = 1;
	bitPerSample_ = 8;
}

string G711aTrack::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	   << "a=rtpmap:" << payloadType_ << " PCMA/" << samplerate_ << "/" << channel_ << "\r\n"
	   << "\r\na=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

G711aTrack::Ptr G711aTrack::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<G711aTrack>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "g711a";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "audio";
    trackInfo->samplerate_ = samplerate;
    trackInfo->bitPerSample_ = 16;

    return trackInfo;
}

G711uTrack::G711uTrack()
{
	payloadType_ = PayloadType_G711U;
	samplerate_ = 8000;
	channel_ = 1;
	bitPerSample_ = 8;
}

string G711uTrack::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	   << "a=rtpmap:" << payloadType_ << " PCMU/" << samplerate_ << "/" << channel_ << "\r\n"
	   << "\r\na=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

G711uTrack::Ptr G711uTrack::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<G711uTrack>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "g711u";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "audio";
    trackInfo->samplerate_ = samplerate;
    trackInfo->bitPerSample_ = 16;

    return trackInfo;
}