#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtmpEncode.h"
#include "Logger.h"
#include "Util/String.h"
#include "RtmpEncodeAac.h"
#include "RtmpEncodeCommon.h"
#include "RtmpEncodeH264.h"
#include "RtmpEncodeH265.h"

using namespace std;


RtmpEncode::Ptr RtmpEncode::create(const shared_ptr<TrackInfo>& trackInfo)
{
    logInfo << "codec: " << trackInfo->codec_;
    if (trackInfo->codec_ == "h264") {
        return make_shared<RtmpEncodeH264>(trackInfo);
    } else if (trackInfo->codec_ == "h265") {
        return make_shared<RtmpEncodeH265>(trackInfo);
    } else if (trackInfo->codec_ == "aac") {
        return make_shared<RtmpEncodeAac>(trackInfo);
    } else if (trackInfo->codec_ == "g711a" || trackInfo->codec_ == "g711u") {
        return make_shared<RtmpEncodeCommon>(trackInfo);
    } else {
        return nullptr;
    }
}