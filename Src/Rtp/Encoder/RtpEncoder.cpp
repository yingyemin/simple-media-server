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
#include "RtpEncodeMp3.h"
#include "Common/Config.h"

using namespace std;

RtpEncoder::Ptr RtpEncoder::create(const shared_ptr<TrackInfo>& trackInfo)
{
    logInfo << "codec: " << trackInfo->codec_;
    RtpEncoder::Ptr encoder;
    if (trackInfo->codec_ == "h264") {
        encoder = make_shared<RtpEncodeH264>(trackInfo);
    } else if (trackInfo->codec_ == "h265") {
        encoder = make_shared<RtpEncodeH265>(trackInfo);
    } else if (trackInfo->codec_ == "aac") {
        encoder = make_shared<RtpEncodeAac>(trackInfo);
    } else if (trackInfo->codec_ == "mp3") {
        encoder = make_shared<RtpEncodeMp3>(trackInfo);
    } else {
        encoder = make_shared<RtpEncodeCommon>(trackInfo);
    }

    if (encoder) {
        encoder->setEnableHuge(false);
        encoder->setFastPts(false);
    }

    return encoder;
}

void RtpEncoder::setFastPts(bool enabled)
{
    _enableFastPts = enabled;
}

void RtpEncoder::setEnableHuge(bool enabled)
{
    // 配置读取
    static int maxRtpSize = Config::instance()->getAndListen([](const json &config){
        maxRtpSize = Config::instance()->get("Rtp", "maxRtpSize");
    }, "Rtp", "maxRtpSize");

    static int hugeRtpSize = Config::instance()->getAndListen([](const json &config){
        hugeRtpSize = Config::instance()->get("Rtp", "hugeRtpSize");
    }, "Rtp", "hugeRtpSize");
    
    _enableHuge = enabled;
    if (_enableHuge) {
        _maxRtpSize = hugeRtpSize;
    } else {
        _maxRtpSize = maxRtpSize;
    }

    if (_maxRtpSize == 0) {
        _maxRtpSize = 1400;
    }
}