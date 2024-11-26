#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "HttpPsVodClient.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"
#include "Common/Define.h"

using namespace std;

HttpPsVodClient::HttpPsVodClient(MediaClientType type, const string& appName, const string& streamName)
    :HttpVodClient(type, appName, streamName)
{
}

void HttpPsVodClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    weak_ptr<HttpPsVodClient> wSelf = dynamic_pointer_cast<HttpPsVodClient>(shared_from_this());

    if (!_demuxer) {
        _demuxer = make_shared<PsDemuxer>();
        _demuxer->setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (self) {
                self->onFrame(frame);
            }
        });

        _demuxer->setOnTrackInfo([wSelf](const shared_ptr<TrackInfo>& trackInfo){
            auto self = wSelf.lock();
            if (self) {
                self->_mapTrackInfo[trackInfo->index_] = trackInfo;
                auto source = self->_source.lock();
                if (source) {
                    source->addTrack(trackInfo);
                }
            }
        });

        _demuxer->setOnReady([wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            auto source = self->_source.lock();
            if (source) {
                source->onReady();
            }
        });
    }
    _startDemuxer = true;
    
    HttpVodClient::start(localIp, localPort, url, timeout);
}

void HttpPsVodClient::stopDecode()
{
    // _decoder = nullptr;
    // _demuxer = nullptr;
    _startDemuxer = false;
}

void HttpPsVodClient::decode(const char* data, int len)
{
    if (!data || len == 0) {
        return ;
    }

    if (!_startDemuxer) {
        _demuxer = nullptr;
        return ;
    }
    
    if (_demuxer) {
        _demuxer->onPsStream((char*)data, len, 0, 0);
    }
    // FILE* fp = fopen("test.ps", "ab+");
    // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    // fclose(fp);
}
