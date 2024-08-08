#ifndef MP4Muxer_H
#define MP4Muxer_H

#include "Common/Frame.h"
#include "Mp4Box.h"
#include "Common/Track.h"

#include <memory>
#include <vector>
#include <unordered_map>

using namespace std;

class MP4Muxer : public enable_shared_from_this<MP4Muxer>
{
public:
    MP4Muxer(int flags);
    ~MP4Muxer();

public:
    virtual void write(const char* data, int size) {}
    virtual void read(char* data, int size) {}
    virtual void seek(int offset) {}
    virtual size_t tell() { return 0;}

public:
    void init();
    void stop();
    void addAudioTrack(const shared_ptr<TrackInfo>& trackInfo);
    void addVideoTrack(const shared_ptr<TrackInfo>& trackInfo);
    void inputFrame(const FrameBuffer::Ptr& frame, int track, bool keyframe);

private:
    void write8BE(uint8_t value);
    void write16BE(uint16_t value);
    void write24BE(uint32_t value);
    void write32BE(uint32_t value);
    void write64BE(uint64_t value);

private:
    size_t write_ftyp();
    void writeMdatSize(uint64_t offset, size_t size);
    size_t writeMoov();
    int moveMoov(uint64_t to, uint64_t from, size_t bytes);
    size_t mov_stco_size(const struct mov_track_t* track, uint64_t offset);
    size_t mov_write_mvhd();
    size_t mov_write_iods();
    size_t mov_write_trak();
    size_t mov_write_udta();
    size_t mov_write_tkhd();
    size_t mov_write_edts();
    size_t mov_write_mdia();
    size_t mov_write_elst();
    size_t mov_write_mdhd();
    size_t mov_write_hdlr();
    size_t mov_write_minf();
    size_t mov_write_vmhd();
    size_t mov_write_smhd();
    size_t mov_write_nmhd();
    size_t mov_write_dinf();
    size_t mov_write_stbl();
    size_t mov_write_dref();
    size_t mov_write_stsd();
    size_t mov_write_stts(uint32_t count);
    size_t mov_write_ctts(uint32_t count);
    uint32_t mov_build_stts(struct mov_track_t* track);
    uint32_t mov_build_ctts(struct mov_track_t* track);
    size_t mov_write_stss();
    uint32_t mov_build_stco(struct mov_track_t* track);
    size_t mov_write_stsc();
    size_t mov_write_stsz();
    size_t mov_write_stco(uint32_t count);
    size_t mov_write_video(const struct mov_sample_entry_t* entry);
    size_t mov_write_audio(const struct mov_sample_entry_t* entry);
    size_t mov_write_avcc();
    size_t mov_write_esds();
    size_t mp4_write_es_descriptor();
    uint32_t mov_write_base_descr(uint8_t tag, uint32_t len);
    int mp4_write_decoder_config_descriptor();
    size_t mp4_write_sl_config_descriptor();
    int mp4_write_decoder_specific_info();
    size_t mov_write_hvcc();
    size_t mov_write_av1c();
    size_t mov_write_vpcc();
    size_t mov_write_dops();

private:
    uint64_t _mdatSize;
    uint64_t _mdatOffset;

    mov_ftyp_t _ftyp;
	struct mov_mvhd_t _mvhd;

    int _flags;
	int _header;
	uint64_t _moof_offset; // last moof offset(from file begin)
    uint64_t _implicit_offset;

    shared_ptr<mov_track_t> _track; // current stream
	vector<shared_ptr<mov_track_t>> _tracks;
	int _track_count;

    const void* _udta;
	uint64_t _udta_size;

    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
};

#endif //MP4Muxer_H

