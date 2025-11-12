#ifndef HttpPsVodClient_H
#define HttpPsVodClient_H

#ifdef ENABLE_MPEG

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Track.h"
#include "Mpeg/PsDemuxer.h"
#include "HttpVodClient.h"

// using namespace std;

class HttpPsVodClient : public HttpVodClient
{
public:
    using Ptr = std::shared_ptr<HttpPsVodClient>;
    HttpPsVodClient(MediaClientType type, const std::string& appName, const std::string& streamName);
    virtual ~HttpPsVodClient()  {}

public:
    bool start(const std::string& localIp, int localPort, const std::string& url, int timeout) override;
    void decode(const char* data, int len) override;
    void stopDecode();
    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}

private:
    bool _startDemuxer = false;
    bool _firstVps = true;
    bool _firstSps = true;
    bool _firstPps = true;
    bool _hasReady = false;
    uint32_t _timestap;
    int _index;
    int _type;
    std::shared_ptr<PsDemuxer> _demuxer;
};

#endif
#endif //HttpPsVodClient_H
