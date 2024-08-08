#ifndef HLSPARSER_H
#define HLSPARSER_H

#include <string>
#include <list>
#include <map>
#include <functional>
#include <unordered_map>

#include "Util/Variant.h"

using namespace std;

struct TsInfo
{
    //ts切片长度
    float duration;
    //url地址
    string url;
};

struct M3u8Info
{
    int width;
    int height;
    uint64_t bandwidth;
    string programId;
    string url;
};


class HlsParser {
public:
    HlsParser(){}
    ~HlsParser(){}
    
    map<uint64_t, TsInfo> getTsList(const string &m3u8);
    bool isLive();
    bool isMutilM3u8();
    int getSeq();
    map<uint64_t, M3u8Info> getM3u8List(const string &m3u8);

    void onTsInfo(const map<uint64_t, TsInfo> &tsList);
    void setOnTsInfo(const function<void(const map<uint64_t, TsInfo> &tsList)>& cb);
    
    void onM3u8Info(const map<uint64_t, M3u8Info> &m3u8List);
    void setOnM3u8Info(const function<void(const map<uint64_t, M3u8Info> &m3u8List)>& cb);

protected:
    Variant getHeader(const string& key);
    bool existHeader(const string& key);

private:
    bool _isMutilM3u8 = false;
    float _totalDuration = 0;

    unordered_map<string, string> _mapHeader;
    function<void(const map<uint64_t, TsInfo> &tsList)> _onTsInfo;
    function<void(const map<uint64_t, M3u8Info> &m3u8List)> _onM3u8Info;
};

#endif //HTTP_HLSPARSER_H
