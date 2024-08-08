#ifndef TsMuxer_H
#define TsMuxer_H

#include <string>
#include <unordered_map>

#include "Common/Track.h"
#include "Net/Buffer.h"
#include "Common/Frame.h"
#include "Util/TimeClock.h"

class TsMuxer
{
public:
    using Ptr = shared_ptr<TsMuxer>;
    TsMuxer();
    ~TsMuxer();

public:
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();
    void stopEncode();
    void addTrackInfo(const shared_ptr<TrackInfo>& trackInfo);

    void setOnTsPacket(const function<void(const StreamBuffer::Ptr& pkt, int pts, int dts, bool keyframe)>& cb) {_onTsPacket = cb;}
    void onTsPacket(const StreamBuffer::Ptr& frame, int pts, int dts, bool keyframe);

private:
    int encode(const FrameBuffer::Ptr& frame);

    int make_pes_packet(const FrameBuffer::Ptr& frame);
	int mk_ts_pat_packet(char *buf, int handle);
	int mk_ts_pmt_packet(char *buf, int handle);
	int mk_pes_packet(char *buf, int bVideo, int length, int bDtsEn, unsigned long long pts, unsigned long long dts);

private:
    bool _first = true;
    bool _startEncode = false;
	uint8_t _patCounter = 0;
	uint8_t _continuityCounter = 0;
    int _lastVideoId = 0xE0;
    int _lastAudioId = 0xC0;
    int _audioCodec = 0;
    int _videoCodec = 0;
	int _streamPid = 0x100;
    unordered_map<int, int> _mapStreamId;
    unordered_map<int, int> _mapContinuity;
    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
    function<void(const StreamBuffer::Ptr& pkt, int pts, int dts, bool keyframe)> _onTsPacket;
};

#endif //PsMuxer_H

