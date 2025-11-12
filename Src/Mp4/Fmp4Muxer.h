#ifndef Fmp4Muxer_H
#define Fmp4Muxer_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Mp4Muxer.h"
#include "Util/File.h"
#include "Util/TimeClock.h"

// using namespace std;

class Fmp4Muxer : public MP4Muxer
{
public:
    using Ptr = std::shared_ptr<Fmp4Muxer>;

    Fmp4Muxer(int flags);
    ~Fmp4Muxer();

public:
    void write(const char* data, int size);
    void read(char* data, int size);
    void seek(uint64_t offset);
    size_t tell();

    void init();
    void addAudioTrack(const std::shared_ptr<TrackInfo>& trackInfo);
    void addVideoTrack(const std::shared_ptr<TrackInfo>& trackInfo);
    int inputFrame(int idx, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags);
    int inputFrame_l(int idx, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags, int add_nalu_size);
    
    int fmp4_writer_save_segment();
    int fmp4_writer_init_segment();
    Buffer::Ptr getFmp4Header();

    void startEncode();
    void stopEncode();

public:
    void setOnFmp4Header(const std::function<void(const Buffer::Ptr& buffer)>& cb);
    void setOnFmp4Segment(const std::function<void(const Buffer::Ptr& buffer, bool keyframe)>& cb);

private:
    size_t fmp4_write_mvex();
    size_t fmp4_write_traf(uint32_t moof);
    size_t fmp4_write_moof(uint32_t fragment, uint32_t moof);
    size_t fmp4_write_moov();
    size_t fmp4_write_sidx();
    int fmp4_write_mfra();
    int fmp4_add_fragment_entry(mov_track_t* track, uint64_t time, uint64_t offset);
    int fmp4_write_fragment();
    int fmp4_writer_init();
    void fmp4_writer_destroy();
    int fmp4_writer_add_udta(const void* data, size_t size);
    size_t mov_write_trex();
    size_t mov_write_tfhd();
    size_t mov_write_tfdt();
    size_t mov_write_trun(uint32_t from, uint32_t count, uint32_t moof);
    size_t mov_write_mfhd(uint32_t fragment);
    size_t mov_write_sidx(uint64_t offset);
    size_t mov_write_tfra();
    size_t mov_write_styp();

private:
    bool _startEncode = false;
    bool _keyframe = false;
    uint64_t _segmentOffset = 0;
    StringBuffer::Ptr _segmentBuffer;
    TimeClock _timeClock;

    size_t _mdatSize = 0;
	int _hasMoov = 0;
    int _trackCount = 0;
    int _moofOffset = 0;
    int _flags = 0;
    int _header = 0;

	uint32_t _fragInterleave = 0;
	uint32_t _fragmentId = 0; // start from 1
	uint32_t _sn = 0; // sample sn

    const void* _udta = nullptr;
	uint64_t _udta_size = 0;

    // mov_ftyp_t _ftyp;
    // mov_mvhd_t _mvhd;
    // shared_ptr<mov_track_t> _track; // current stream
    std::vector<std::shared_ptr<mov_track_t>> _tracks;
    std::unordered_map<int, int> _mapTrackInfo;

    std::function<void(const Buffer::Ptr& buffer, bool keyframe)> _onFmp4Segment;
    std::function<void(const Buffer::Ptr& buffer)> _onFmp4Header;
};


#endif //Mp4FileWriter_H
