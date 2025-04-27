#ifndef Mp4Demuxer_H
#define Mp4Demuxer_H

#include "Common/Frame.h"
#include "Common/Track.h"
#include "Mp4Box.h"

#include <memory>

using namespace std;

class MP4Demuxer : public enable_shared_from_this<MP4Demuxer>
{
public:
    MP4Demuxer();
    ~MP4Demuxer();

public:
    virtual void write(const char* data, int size) {}
    virtual void read(char* data, int size) {}
    virtual void seek(uint64_t offset) {}
    virtual size_t tell() { return 0;}
    virtual void onFrame(const StreamBuffer::Ptr& frame, int trackIndex, int pts, int dts, bool keyframe) {}
    virtual void onTrackInfo(const TrackInfo::Ptr& trackInfo) {}
    virtual void onReady() {}

    void skip(int size);

    uint64_t read64BE();
    uint32_t read32BE();
    uint32_t read24BE();
    uint16_t read16BE();
    uint8_t read8BE();

public:
    bool init();
    int mov_reader_box(const struct mov_box_t* parent);
    int mov_reader_getinfo();
    int mov_reader_read(void* buffer, size_t bytes);
    int mov_reader_read2();
    int mov_reader_seek(int64_t* timestamp);

protected:
    int mov_index_build(mov_track_t* track);
    int mov_read_mdat(const struct mov_box_t* box);
    int mov_read_free(const struct mov_box_t* box);
    int mov_read_trak(const struct mov_box_t* box);
    int mov_read_dref(const struct mov_box_t* box);
    int mov_read_btrt(const struct mov_box_t* box);
    int mov_read_uuid(const struct mov_box_t* box);
    int mov_read_moof(const struct mov_box_t* box);
    int mov_read_mfro(const struct mov_box_t* box);
    int mov_read_default(const struct mov_box_t* box);
    void mov_reader_destroy();
    mov_track_t* mov_reader_next();
    uint64_t mov_reader_getduration();
    int mov_stss_seek(mov_track_t* track, int64_t *timestamp);
    int mov_sample_seek(mov_track_t* track, int64_t timestamp);
    int mov_read_av1c(const struct mov_box_t* box);
    int mov_read_avcc(const struct mov_box_t* box);
    int mov_read_coll(const struct mov_box_t* box);
    int mov_read_ctts(const struct mov_box_t* box);
    int mov_read_cslg(const struct mov_box_t* box);
    int mov_read_dops(const struct mov_box_t* box);
    int mov_read_elst(const struct mov_box_t* box);
    int mov_read_base_descr(int bytes, int* tag,  int* len);
    int mp4_read_es_descriptor(uint64_t bytes);
    int mp4_read_decoder_specific_info(int len);
    int mp4_read_decoder_config_descriptor(int len);
    int mp4_read_sl_config_descriptor();
    int mp4_read_tag(uint64_t bytes);
    int mov_read_esds(const struct mov_box_t* box);
    int mov_read_ftyp(const struct mov_box_t* box);
    int mov_read_hdlr(const struct mov_box_t* box);
    int mov_read_hvcc(const struct mov_box_t* box);
    int mov_read_leva(const struct mov_box_t* box);
    int mov_read_mdhd(const struct mov_box_t* box);
    int mov_read_mehd(const struct mov_box_t* box);
    int mov_read_mfhd(const struct mov_box_t* box);
    int mov_read_mvhd(const struct mov_box_t* box);
    int mp4_read_extra(const struct mov_box_t* box);
    int mov_read_sample_entry(struct mov_box_t* box, uint16_t* data_reference_index);
    int mov_read_audio(struct mov_sample_entry_t* entry);
    int mov_read_video(struct mov_sample_entry_t* entry);
    int mov_read_pasp(const struct mov_box_t* box);
    int mov_read_hint_sample_entry(struct mov_sample_entry_t* entry);
    int mov_read_meta_sample_entry(struct mov_sample_entry_t* entry);
    int mov_read_text_sample_entry(struct mov_sample_entry_t* entry);
    int mov_read_subtitle_sample_entry(struct mov_sample_entry_t* entry);
    int mov_read_stsd(const struct mov_box_t* box);
    int mov_read_sidx(const struct mov_box_t* box);
    int mov_read_smdm(const struct mov_box_t* box);
    int mov_read_vmhd(const struct mov_box_t* box);
    int mov_read_smhd(const struct mov_box_t* box);
    int mov_read_gmin(const struct mov_box_t* box);
    int mov_read_text(const struct mov_box_t* box);
    int mov_read_stco(const struct mov_box_t* box);
    int mov_read_stsc(const struct mov_box_t* box);
    int mov_read_stss(const struct mov_box_t* box);
    int mov_read_stsz(const struct mov_box_t* box);
    int mov_read_stts(const struct mov_box_t* box);
    int mov_read_stz2(const struct mov_box_t* box);
    int mov_read_tfdt(const struct mov_box_t* box);
    int mov_read_tfhd(const struct mov_box_t* box);
    int mov_read_tfra(const struct mov_box_t* box);
    int mov_read_tkhd(const struct mov_box_t* box);
    int mov_read_trex(const struct mov_box_t* box);
    int mov_read_trun(const struct mov_box_t* box);
    int mov_read_vpcc(const struct mov_box_t* box);
    void mov_apply_stco(mov_track_t* track);
    void mov_apply_elst(mov_track_t *track);
    int mov_read_tx3g(const struct mov_box_t* box);


    void mov_apply_elst_tfdt(mov_track_t *track);
    void mov_apply_stts(mov_track_t* track);
    void mov_apply_ctts(mov_track_t* track);
    void mov_apply_stss(mov_track_t* track);

    mov_track_t* mov_find_track(uint32_t track);
    mov_track_t* mov_fetch_track(uint32_t track);
    mov_track_t* mov_add_track();


private:
    mov_ftyp_t _ftyp;
	struct mov_mvhd_t _mvhd;
    int _header = 0;

    uint64_t _moof_offset = 0;
    uint64_t _implicit_offset = 0;
    
    mov_track_t* _track; // current stream
	vector<shared_ptr<mov_track_t>> _tracks;
	int _track_count = 0;

    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
};

#endif //Mp4Demuxer_H

