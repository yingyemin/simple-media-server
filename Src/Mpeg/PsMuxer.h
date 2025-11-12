#ifndef PsMuxer_H
#define PsMuxer_H

#include <string>
#include <unordered_map>

#include "Common/Track.h"
#include "Net/Buffer.h"
#include "Common/Frame.h"
#include "Util/TimeClock.h"
#include "Common/StampAdjust.h"

#define PS_HDR_LEN  14
#define SYS_HDR_LEN 18
#define PSM_HDR_LEN 24
#define PES_HDR_LEN 14
#define RTP_HDR_LEN 12
#define RTP_VERSION 2
#define RTP_MAX_PACKET_BUFF 1460
// #define PS_PES_PAYLOAD_SIZE 65522
#define PS_PES_PAYLOAD_SIZE 65000

class PsMuxer
{public:
    using Ptr = std::shared_ptr<PsMuxer>;
    PsMuxer();
    ~PsMuxer();

public:
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();
    void stopEncode();
    void addTrackInfo(const std::shared_ptr<TrackInfo>& trackInfo);

    void setOnPsFrame(const std::function<void(const FrameBuffer::Ptr& rtp)>& cb) {_onPsFrame = cb;}

private:
    int encode(const FrameBuffer::Ptr& frame);
    int makePsHeader(int dts, int index);
    int makeSysHeader(int index);
    int makePsmHeader(int index);
    int makePesHeader(int stream_id, int payload_len, 
                        unsigned long long pts, unsigned long long dts, int nSizePos);

private:
    bool _startEncode = false;
    bool _sendMetaFrame = false;
    int _lastVideoId = 0xE0;
    int _lastAudioId = 0xC0;
    int _audioCodec = 0;
    int _videoCodec = 0;
    FrameBuffer::Ptr _psFrame;
    std::unordered_map<int, int> _mapStreamId;
    std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
    std::unordered_map<int, std::shared_ptr<StampAdjust>> _mapStampAdjust;
    std::function<void(const FrameBuffer::Ptr& rtp)> _onPsFrame;
};

#endif //PsMuxer_H

