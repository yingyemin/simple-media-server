#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <iomanip>

#include "Mp3Track.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

Mp3Track::Mp3Track()
{
	payloadType_ = PayloadType_Mp3;
}

string Mp3Track::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	   << "a=rtpmap:" << payloadType_ << " MPA" << "\r\n"
	   << "\r\na=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

Mp3Track::Ptr Mp3Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<Mp3Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "mp3";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "audio";
    trackInfo->samplerate_ = samplerate;
    trackInfo->bitPerSample_ = 16;

    return trackInfo;
}
