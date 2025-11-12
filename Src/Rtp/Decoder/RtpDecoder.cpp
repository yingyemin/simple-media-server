#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecoder.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "RtpDecodeH264.h"
#include "RtpDecodeH265.h"
#include "RtpDecodeH266.h"
#include "RtpDecodeAac.h"
#include "RtpDecodeCommon.h"
#include "RtpDecodeMp3.h"
#include "RtpDecodeVp8.h"
#include "RtpDecodeVp9.h"
#include "RtpDecodeAV1.h"

using namespace std;

RtpDecoder::Ptr RtpDecoder::creatDecoder(const shared_ptr<TrackInfo>& trackInfo)
{
    logDebug << "codec: " << trackInfo->codec_;
    if (trackInfo->codec_ == "h264") {
        return make_shared<RtpDecodeH264>(trackInfo);
    } else if (trackInfo->codec_ == "aac") {
        return make_shared<RtpDecodeAac>(trackInfo);
    } else if (trackInfo->codec_ == "h265") {
        return make_shared<RtpDecodeH265>(trackInfo);
    } else if (trackInfo->codec_ == "h266") {
        return make_shared<RtpDecodeH266>(trackInfo);
    } else if (trackInfo->codec_ == "mp3") {
        return make_shared<RtpDecodeMp3>(trackInfo);
    } else if (trackInfo->codec_ == "vp8") {
        return make_shared<RtpDecodeVp8>(trackInfo);
    } else if (trackInfo->codec_ == "vp9") {
        return make_shared<RtpDecodeVP9>(trackInfo);
    } else if (trackInfo->codec_ == "av1") {
        return make_shared<RtpDecodeAV1>(trackInfo);
    } else {
        return make_shared<RtpDecodeCommon>(trackInfo);
    }
}
