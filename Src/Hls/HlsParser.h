#ifndef HLSPARSER_H
#define HLSPARSER_H

#include <string>
#include <list>
#include <map>
#include <functional>
#include <unordered_map>

#include "Util/Variant.h"

// using namespace std;

struct TsInfo
{
    //ts切片长度
    float duration;
    //url地址
    std::string url;
};

struct M3u8Info
{
    int width;
    int height;
    uint64_t bandwidth;
    std::string programId;
    std::string url;
};


class HlsParser {
public:
    HlsParser(){}
    ~HlsParser(){}
    
    std::map<uint64_t, TsInfo> getTsList(const std::string &m3u8);
    bool isLive();
    bool isMutilM3u8();
    int getSeq();
    std::map<uint64_t, M3u8Info> getM3u8List(const std::string &m3u8);

    void onTsInfo(const std::map<uint64_t, TsInfo> &tsList);
    void setOnTsInfo(const std::function<void(const std::map<uint64_t, TsInfo> &tsList)>& cb);
    
    void onM3u8Info(const std::map<uint64_t, M3u8Info> &m3u8List);
    void setOnM3u8Info(const std::function<void(const std::map<uint64_t, M3u8Info> &m3u8List)>& cb);

protected:
    Variant getHeader(const std::string& key);
    bool existHeader(const std::string& key);

private:
    bool _isMutilM3u8 = false;
    float _totalDuration = 0;

    std::unordered_map<std::string, std::string> _mapHeader;
    std::function<void(const std::map<uint64_t, TsInfo> &tsList)> _onTsInfo;
    std::function<void(const std::map<uint64_t, M3u8Info> &m3u8List)> _onM3u8Info;
};

#endif //HTTP_HLSPARSER_H
