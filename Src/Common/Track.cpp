#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Track.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

unordered_map<string, TrackInfo::funcCreateTrackInfo> TrackInfo::_mapCreateTrack;

TrackInfo::Ptr TrackInfo::createTrackInfo(const string& codecName)
{
    auto iter = _mapCreateTrack.find(codecName);
    if (iter != _mapCreateTrack.end()) {
        return iter->second(0, 0, 0);
    } else {
        logInfo << "invalid codec name: " << codecName;
    }
    return nullptr;
}

void TrackInfo::registerTrackInfo(const string& codecName, const funcCreateTrackInfo& func)
{
    _mapCreateTrack[codecName] = func;
}