#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncoder.h"
#include "Logger.h"
#include "Util/String.h"
#include "RtpEncodeH264.h"
#include "RtpEncodeH265.h"
#include "RtpEncodeAac.h"
#include "RtpEncodeCommon.h"

using namespace std;


RtpEncoder::Ptr RtpEncoder::create(const shared_ptr<TrackInfo>& trackInfo)
{
    logInfo << "codec: " << trackInfo->codec_;
    if (trackInfo->codec_ == "h264") {
        return make_shared<RtpEncodeH264>(trackInfo);
    } else if (trackInfo->codec_ == "h265") {
        return make_shared<RtpEncodeH265>(trackInfo);
    } else if (trackInfo->codec_ == "aac") {
        return make_shared<RtpEncodeAac>(trackInfo);
    } else {
        return make_shared<RtpEncodeCommon>(trackInfo);
    }
}