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
#include "RtmpEncodeH266.h"
#include "RtmpEncodeVPX.h"
#include "RtmpEncodeAV1.h"

using namespace std;


RtmpEncode::Ptr RtmpEncode::create(const shared_ptr<TrackInfo>& trackInfo)
{
    logInfo << "codec: " << trackInfo->codec_;
    RtmpEncode::Ptr source;
    if (trackInfo->codec_ == "h264") {
        source = make_shared<RtmpEncodeH264>(trackInfo);
    } else if (trackInfo->codec_ == "h265") {
        source =  make_shared<RtmpEncodeH265>(trackInfo);
    } else if (trackInfo->codec_ == "h266") {
        source =  make_shared<RtmpEncodeH266>(trackInfo);
    }  else if (trackInfo->codec_ == "aac") {
        source =  make_shared<RtmpEncodeAac>(trackInfo);
    } else if (trackInfo->codec_ == "vp9") {
        source =  make_shared<RtmpEncodeVPX>(trackInfo);
    } else if (trackInfo->codec_ == "av1") {
        source =  make_shared<RtmpEncodeAV1>(trackInfo);
    } else if (trackInfo->codec_ == "g711a" || trackInfo->codec_ == "g711u"
            || trackInfo->codec_ == "mp3" || trackInfo->codec_ == "opus"
            || trackInfo->codec_ == "adpcma") {
        source =  make_shared<RtmpEncodeCommon>(trackInfo);
    } else {
        return nullptr;
    }


    if (source) {
        source->setEnhanced(false);
    }

    return source;
}