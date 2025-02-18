#ifndef HttpPsVodClient_H
#define HttpPsVodClient_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Track.h"
#include "Mpeg/PsDemuxer.h"
#include "HttpVodClient.h"

using namespace std;

class HttpPsVodClient : public HttpVodClient
{
public:
    using Ptr = shared_ptr<HttpPsVodClient>;
    HttpPsVodClient(MediaClientType type, const string& appName, const string& streamName);
    virtual ~HttpPsVodClient()  {}

public:
    bool start(const string& localIp, int localPort, const string& url, int timeout) override;
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
    shared_ptr<PsDemuxer> _demuxer;
};


#endif //HttpPsVodClient_H
