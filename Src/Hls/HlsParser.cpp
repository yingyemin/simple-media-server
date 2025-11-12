#include <cstdlib>
#include <inttypes.h>
#include <sstream>
#include "HlsParser.h"
#include "Util/String.hpp"
#include "Log/Logger.h"

using namespace std;

map<uint64_t, TsInfo> HlsParser::getTsList(const string &m3u8)
{
    // map<uint64_t, M3u8Info> m3u8List = getM3u8List(m3u8);
    // if (m3u8List.size() == 0) {
    //     logWarn << "no m3u8 exist";
    //     return false;
    // }

    // if (_isMutilM3u8) {
    //     onM3u8Info(m3u8List);

    //     return true;
    // }

    // string newM3u8 = m3u8List.begin()->second.url;
    string line;
    stringstream ss(m3u8);
    TsInfo info;
    map<uint64_t, TsInfo> tsList;
    int index = -1;

    while(getline(ss, line)) {
        if (line.find("#EXTINF:") != string::npos) {
            sscanf(line.data(), "#EXTINF:%f,", &info.duration);
            _totalDuration += info.duration;
            if (index == -1) {
                index = getSeq();
                if (index == -1) {
                    logWarn << "seq is error";
                    return tsList;
                }
            }
        } else if (line[0] != '#') {
            logInfo << "ts url: " << line << ", index: " << index;
            info.url = line;
            tsList[index++] = info;
        } else {
            auto pos = line.find_first_of(":");
            if (pos == string::npos) {
                _mapHeader[line] = "";
            } else {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 1);
                _mapHeader[key] = value;
            }
        }
    }

    return tsList;
}

bool HlsParser::isLive()
{
    return !existHeader("#EXT-X-ENDLIST");
}

bool HlsParser::isMutilM3u8()
{
    return _isMutilM3u8;
}

int HlsParser::getSeq()
{
    if (!existHeader("#EXT-X-MEDIA-SEQUENCE")) {
        return -1;
    }

    return getHeader("#EXT-X-MEDIA-SEQUENCE");
}

void HlsParser::onTsInfo(const map<uint64_t, TsInfo> &tsList)
{
    if (_onTsInfo) {
        _onTsInfo(tsList);
    }
}

void HlsParser::setOnTsInfo(const function<void(const map<uint64_t, TsInfo> &tsList)>& cb)
{
    _onTsInfo = cb;
}

void HlsParser::onM3u8Info(const map<uint64_t, M3u8Info> &m3u8List)
{
    if (_onM3u8Info) {
        _onM3u8Info(m3u8List);
    }
}

void HlsParser::setOnM3u8Info(const function<void(const map<uint64_t, M3u8Info> &m3u8List)>& cb)
{
    _onM3u8Info = cb;
}


map<uint64_t, M3u8Info> HlsParser::getM3u8List(const string &m3u8)
{
    string line;
    stringstream ss(m3u8);
    map<uint64_t, M3u8Info> m3u8List;

    getline(ss, line);
    if (line != "#EXTM3U") {
        logWarn << "it is not a m3u8";
        return m3u8List;
    }

    M3u8Info info;
    while(getline(ss, line)) {
        static string streamInf = "#EXT-X-STREAM-INF:";
        if (line.find(streamInf) != string::npos) {
            _isMutilM3u8 = true;
            auto key_val = split(line.substr(streamInf.size()), ",", "=");
            info.programId = atoll(key_val["PROGRAM-ID"].data());
            info.bandwidth = atoll(key_val["BANDWIDTH"].data());
            sscanf(key_val["RESOLUTION"].data(), "%dx%d", &info.width, &info.height);
        } else if (line.find("#EXTINF") != string::npos) {
            info.url = m3u8;
            m3u8List[0] = info;
            return m3u8List;
        } else if (line[0] != '#') {
            info.url = line;
            m3u8List[info.bandwidth] = info;
        }
    }

    return m3u8List;
}

Variant HlsParser::getHeader(const string& key)
{
    if (existHeader(key)) {
        return _mapHeader[key];
    }

    return "";
}

bool HlsParser::existHeader(const string& key)
{
    if (_mapHeader.find(key) != _mapHeader.end()) {
        return true;
    }

    return false;
}