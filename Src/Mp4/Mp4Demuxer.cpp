#include "Mp4Demuxer.h"
#include "Util/String.h"
#include "Codec/AacTrack.h"
#include "Codec/H264Frame.h"
#include "Codec/H264Track.h"
#include "Codec/H265Frame.h"
#include "Codec/H265Track.h"

#include <functional>

using namespace std;

MP4Demuxer::MP4Demuxer()
{}

MP4Demuxer::~MP4Demuxer()
{}

uint64_t MP4Demuxer::read64BE()
{
    return ((uint64_t)read32BE()) << 32 | read32BE();
}

uint32_t MP4Demuxer::read32BE()
{
    char value[4];
    read(value, 4);
    return readUint32BE(value);
}

uint32_t MP4Demuxer::read24BE()
{
    char value[3];
    read(value, 3);
    return readUint24BE(value);
}

uint16_t MP4Demuxer::read16BE()
{
    char value[2];
    read(value, 2);
    return readUint16BE(value);
}

uint8_t MP4Demuxer::read8BE()
{
    char value[1];
    read(value, 1);
    return value[0];
}

void MP4Demuxer::skip(int size)
{
    uint64_t offset = tell();
    seek(offset + size);
}

void MP4Demuxer::init()
{
    _ftyp.major_brand = MOV_BRAND_MP41;
	_ftyp.minor_version = 0;
	_ftyp.brands_count = 0;
	_header = 0;

    int i, r;
	struct mov_box_t box;
	struct mov_track_t* track;

	box.type = MOV_ROOT;
	box.size = UINT64_MAX;
	r = mov_reader_box(&box);
	if (0 != r) {
        return ;
    }
	
	for (i = 0; i < _track_count; i++)
	{
		track = _tracks[i].get();
		mov_index_build(track);
		track->sample_offset = 0; // reset

		// fragment mp4
		if (0 == track->mdhd.duration && track->sample_count > 0)
			track->mdhd.duration = track->samples[track->sample_count - 1]->dts - track->samples[0]->dts;
		if (0 == track->tkhd.duration)
			track->tkhd.duration = track->mdhd.duration * _mvhd.timescale / track->mdhd.timescale;
		if (track->tkhd.duration > _mvhd.duration)
			_mvhd.duration = track->tkhd.duration; // maximum track duration
	}
}

int MP4Demuxer::mov_reader_box(const struct mov_box_t* parent)
{
    static struct mov_parse_t s_mov_parse_table[] = {
        { MOV_TAG('a', 'v', '1', 'C'), MOV_NULL, [this](const mov_box_t *box){return mov_read_av1c(box);} }, // av1-isobmff
        { MOV_TAG('a', 'v', 'c', 'C'), MOV_NULL, [this](const mov_box_t *box){return mov_read_avcc(box);} }, // ISO/IEC 14496-15:2010(E) avcC
        { MOV_TAG('b', 't', 'r', 't'), MOV_NULL, [this](const mov_box_t *box){return mov_read_btrt(box);} }, // ISO/IEC 14496-15:2010(E) 5.3.4.1.1 Definition
        { MOV_TAG('c', 'o', '6', '4'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stco(box);} },
        { MOV_TAG('C', 'o', 'L', 'L'), MOV_STBL, [this](const mov_box_t *box){return mov_read_coll(box);} },
        { MOV_TAG('c', 't', 't', 's'), MOV_STBL, [this](const mov_box_t *box){return mov_read_ctts(box);} },
        { MOV_TAG('c', 's', 'l', 'g'), MOV_STBL, [this](const mov_box_t *box){return mov_read_cslg(box);} },
        { MOV_TAG('d', 'i', 'n', 'f'), MOV_MINF, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('d', 'O', 'p', 's'), MOV_NULL, [this](const mov_box_t *box){return mov_read_dops(box);} },
        { MOV_TAG('d', 'r', 'e', 'f'), MOV_DINF, [this](const mov_box_t *box){return mov_read_dref(box);} },
        { MOV_TAG('e', 'd', 't', 's'), MOV_TRAK, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('e', 'l', 's', 't'), MOV_EDTS, [this](const mov_box_t *box){return mov_read_elst(box);} },
        { MOV_TAG('e', 's', 'd', 's'), MOV_NULL, [this](const mov_box_t *box){return mov_read_esds(box);} }, // ISO/IEC 14496-14:2003(E) mp4a/mp4v/mp4s
        { MOV_TAG('f', 'r', 'e', 'e'), MOV_NULL, [this](const mov_box_t *box){return mov_read_free(box);} },
        { MOV_TAG('f', 't', 'y', 'p'), MOV_ROOT, [this](const mov_box_t *box){return mov_read_ftyp(box);} },
        { MOV_TAG('g', 'm', 'i', 'n'), MOV_GMHD, [this](const mov_box_t *box){return mov_read_gmin(box);} }, // Apple QuickTime gmin
        { MOV_TAG('g', 'm', 'h', 'd'), MOV_MINF, [this](const mov_box_t *box){return mov_read_default(box);} }, // Apple QuickTime gmhd
        { MOV_TAG('h', 'd', 'l', 'r'), MOV_MDIA, [this](const mov_box_t *box){return mov_read_hdlr(box);} }, // Apple QuickTime minf also has hdlr
        { MOV_TAG('h', 'v', 'c', 'C'), MOV_NULL, [this](const mov_box_t *box){return mov_read_hvcc(box);} }, // ISO/IEC 14496-15:2014 hvcC
        { MOV_TAG('l', 'e', 'v', 'a'), MOV_MVEX, [this](const mov_box_t *box){return mov_read_leva(box);} },
        { MOV_TAG('m', 'd', 'a', 't'), MOV_ROOT, [this](const mov_box_t *box){return mov_read_mdat(box);} },
        { MOV_TAG('m', 'd', 'h', 'd'), MOV_MDIA, [this](const mov_box_t *box){return mov_read_mdhd(box);} },
        { MOV_TAG('m', 'd', 'i', 'a'), MOV_TRAK, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('m', 'e', 'h', 'd'), MOV_MVEX, [this](const mov_box_t *box){return mov_read_mehd(box);} },
        { MOV_TAG('m', 'f', 'h', 'd'), MOV_MOOF, [this](const mov_box_t *box){return mov_read_mfhd(box);} },
        { MOV_TAG('m', 'f', 'r', 'a'), MOV_ROOT, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('m', 'f', 'r', 'o'), MOV_MFRA, [this](const mov_box_t *box){return mov_read_mfro(box);} },
        { MOV_TAG('m', 'i', 'n', 'f'), MOV_MDIA, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('m', 'o', 'o', 'v'), MOV_ROOT, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('m', 'o', 'o', 'f'), MOV_ROOT, [this](const mov_box_t *box){return mov_read_moof(box);} },
        { MOV_TAG('m', 'v', 'e', 'x'), MOV_MOOV, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('m', 'v', 'h', 'd'), MOV_MOOV, [this](const mov_box_t *box){return mov_read_mvhd(box);} },
    //	{ MOV_TAG('n', 'm', 'h', 'd'), MOV_MINF, [this](const mov_box_t *box){return mov_read_default(box);} }, // ISO/IEC 14496-12:2015(E) 8.4.5.2 Null Media Header Box (p45)
        { MOV_TAG('p', 'a', 's', 'p'), MOV_NULL, [this](const mov_box_t *box){return mov_read_pasp(box);} },
        { MOV_TAG('s', 'i', 'd', 'x'), MOV_ROOT, [this](const mov_box_t *box){return mov_read_sidx(box);} },
        { MOV_TAG('s', 'k', 'i', 'p'), MOV_NULL, [this](const mov_box_t *box){return mov_read_free(box);} },
        { MOV_TAG('S', 'm', 'D', 'm'), MOV_MINF, [this](const mov_box_t *box){return mov_read_smdm(box);} },
        { MOV_TAG('s', 'm', 'h', 'd'), MOV_MINF, [this](const mov_box_t *box){return mov_read_smhd(box);} },
        { MOV_TAG('s', 't', 'b', 'l'), MOV_MINF, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('s', 't', 'c', 'o'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stco(box);} },
    //	{ MOV_TAG('s', 't', 'h', 'd'), MOV_MINF, [this](const mov_box_t *box){return mov_read_default(box);} }, // ISO/IEC 14496-12:2015(E) 12.6.2 Subtitle media header (p185)
        { MOV_TAG('s', 't', 's', 'c'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stsc(box);} },
        { MOV_TAG('s', 't', 's', 'd'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stsd(box);} },
        { MOV_TAG('s', 't', 's', 's'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stss(box);} },
        { MOV_TAG('s', 't', 's', 'z'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stsz(box);} },
        { MOV_TAG('s', 't', 't', 's'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stts(box);} },
        { MOV_TAG('s', 't', 'z', '2'), MOV_STBL, [this](const mov_box_t *box){return mov_read_stz2(box);} },
        { MOV_TAG('t', 'e', 'x', 't'), MOV_GMHD, [this](const mov_box_t *box){return mov_read_text(box);} },
        { MOV_TAG('t', 'f', 'd', 't'), MOV_TRAF, [this](const mov_box_t *box){return mov_read_tfdt(box);} },
        { MOV_TAG('t', 'f', 'h', 'd'), MOV_TRAF, [this](const mov_box_t *box){return mov_read_tfhd(box);} },
        { MOV_TAG('t', 'f', 'r', 'a'), MOV_MFRA, [this](const mov_box_t *box){return mov_read_tfra(box);} },
        { MOV_TAG('t', 'k', 'h', 'd'), MOV_TRAK, [this](const mov_box_t *box){return mov_read_tkhd(box);} },
        { MOV_TAG('t', 'r', 'a', 'k'), MOV_MOOV, [this](const mov_box_t *box){return mov_read_trak(box);} },
        { MOV_TAG('t', 'r', 'e', 'x'), MOV_MVEX, [this](const mov_box_t *box){return mov_read_trex(box);} },
        { MOV_TAG('t', 'r', 'a', 'f'), MOV_MOOF, [this](const mov_box_t *box){return mov_read_default(box);} },
        { MOV_TAG('t', 'r', 'u', 'n'), MOV_TRAF, [this](const mov_box_t *box){return mov_read_trun(box);} },
        { MOV_TAG('u', 'u', 'i', 'd'), MOV_NULL, [this](const mov_box_t *box){return mov_read_uuid(box);} },
        { MOV_TAG('v', 'm', 'h', 'd'), MOV_MINF, [this](const mov_box_t *box){return mov_read_vmhd(box);} },
        { MOV_TAG('v', 'p', 'c', 'C'), MOV_NULL, [this](const mov_box_t *box){return mov_read_vpcc(box);} },

        { 0, 0, NULL } // last
    };
	int i;
	uint64_t bytes = 0;
	struct mov_box_t box;
	function< int(const struct mov_box_t* box)> parse;

	while (bytes + 8 < parent->size)
	{
		uint64_t n = 8;
		box.size = read32BE();
		box.type = read32BE();

		if (1 == box.size)
		{
			// unsigned int(64) large size
			box.size = ((uint64_t)read32BE()) << 32 | read32BE();
			n += 8;
		}
		else if (0 == box.size)
		{
			if (0 == box.type)
				break; // all done
			box.size = UINT64_MAX;
		}

		if (UINT64_MAX == box.size)
		{
			bytes = parent->size;
		}
		else
		{
			bytes += box.size;
			box.size -= n;
		}

		if (bytes > parent->size)
			return -1;

		for (i = 0, parse = NULL; s_mov_parse_table[i].type && !parse; i++)
		{
			if (s_mov_parse_table[i].type == box.type)
			{
				// Apple QuickTime minf also has hdlr
				if(!s_mov_parse_table[i].parent || s_mov_parse_table[i].parent == parent->type)
					parse = s_mov_parse_table[i].parse;
			}
		}

		if (NULL == parse)
		{
			skip(box.size);
		}
		else
		{
			int r;
			uint64_t pos, pos2;
			pos = tell();
			r = parse(&box);
			assert(0 == r);
			if (0 != r) return r;
			pos2 = tell();
			assert(pos2 - pos == box.size);
			skip(box.size - (pos2 - pos));
		}
	}

	return 0;
}

int MP4Demuxer::mov_index_build(struct mov_track_t* track)
{
	void* p;
	uint32_t i, j;
	struct mov_stbl_t* stbl = &track->stbl;

	if (stbl->stss_count > 0 || MOV_VIDEO != track->handler_type)
		return 0;

	for (i = 0; i < track->sample_count; i++)
	{
		if (track->samples[i]->flags & MOV_AV_FLAG_KEYFREAME)
			++stbl->stss_count;
	}

	// p = realloc(stbl->stss, sizeof(stbl->stss[0]) * stbl->stss_count);
	// if (!p) return ENOMEM;
	// stbl->stss = p;

    stbl->stss.resize(stbl->stss_count);

	for (j = i = 0; i < track->sample_count && j < stbl->stss_count; i++)
	{
		if (track->samples[i]->flags & MOV_AV_FLAG_KEYFREAME)
			stbl->stss[j++] = i + 1; // uint32_t sample_number, start from 1
	}
	assert(j == stbl->stss_count);
	return 0;
}


// 8.1.1 Media Data Box (p28)
int MP4Demuxer::mov_read_mdat(const struct mov_box_t* box)
{
	skip(box->size);
	return 0;
}

// 8.1.2 Free Space Box (p28)
int MP4Demuxer::mov_read_free(const struct mov_box_t* box)
{
	// Container: File or other box
	skip(box->size);
	return 0;
}

// 8.3.1 Track Box (p31)
// Box Type : 'trak' 
// Container : Movie Box('moov') 
// Mandatory : Yes 
// Quantity : One or more
int MP4Demuxer::mov_read_trak(const struct mov_box_t* box)
{
	int r;

    _track = NULL;
	r = mov_reader_box(box);
	if (0 == r)
	{
        _track->tfdt_dts = 0;
        if (_track->sample_count > 0)
        {
            mov_apply_stco(_track.get());
            mov_apply_elst(_track.get());
            mov_apply_stts(_track.get());
            mov_apply_ctts(_track.get());
			mov_apply_stss(_track.get());

            _track->tfdt_dts = _track->samples[_track->sample_count - 1]->dts;
        }
	}

	return r;
}

int MP4Demuxer::mov_read_dref(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	for (i = 0; i < entry_count; i++)
	{
		uint32_t size = read32BE();
		/*uint32_t type = */read32BE();
		/*uint32_t vern = */read32BE(); /* version + flags */
		skip(size-12);
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_btrt(const struct mov_box_t* box)
{
	// ISO/IEC 14496-15:2010(E)
	// 5.3.4 AVC Video Stream Definition (p19)
	read32BE(); /* bufferSizeDB */
	read32BE(); /* maxBitrate */
	read32BE(); /* avgBitrate */
	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_uuid(const struct mov_box_t* box)
{
	uint8_t usertype[16] = { 0 };
	if(box->size > 16) 
	{
		read((char*)usertype, sizeof(usertype));
		skip(box->size - 16);
	}
	return 0;
}

int MP4Demuxer::mov_read_moof(const struct mov_box_t* box)
{
    // 8.8.7 Track Fragment Header Box (p71)
    // If base-data-offset-present not provided and if the default-base-is-moof flag is not set,
    // the base-data-offset for the first track in the movie fragment is the position of
    // the first byte of the enclosing Movie Fragment Box, for second and subsequent track fragments, 
    // the default is the end of the data defined by the preceding track fragment.
	_moof_offset = _implicit_offset = tell() - 8 /*box size */;
	return mov_reader_box(box);
}

// 8.8.11 Movie Fragment Random Access Offset Box (p75)
int MP4Demuxer::mov_read_mfro(const struct mov_box_t* box)
{
	(void)box;
	read32BE(); /* version & flags */
	read32BE(); /* size */
	return 0;
}

int MP4Demuxer::mov_read_default(const struct mov_box_t* box)
{
	return mov_reader_box(box);
}

void MP4Demuxer::mov_reader_destroy()
{
	_tracks.clear();
}

struct mov_track_t* MP4Demuxer::mov_reader_next()
{
	int i;
	int64_t dts, best_dts = 0;
	struct mov_track_t* track = NULL;
	struct mov_track_t* track2;

	for (i = 0; i < _track_count; i++)
	{
		track2 = _tracks[i].get();
		assert(track2->sample_offset <= track2->sample_count);
		if (track2->sample_offset >= track2->sample_count)
			continue;

		dts = track2->samples[track2->sample_offset]->dts * 1000 / track2->mdhd.timescale;
		//if (NULL == track || dts < best_dts)
		//if (NULL == track || track->samples[track->sample_offset].offset > track2->samples[track2->sample_offset].offset)
		if (NULL == track || (dts < best_dts && best_dts - dts > AV_TRACK_TIMEBASE) || track2->samples[track2->sample_offset]->offset < track->samples[track->sample_offset]->offset)
		{
			track = track2;
			best_dts = dts;
		}
	}

	return track;
}

int MP4Demuxer::mov_reader_read(void* buffer, size_t bytes)
{
	struct mov_track_t* track;
	struct mov_sample_t* sample;

	track = mov_reader_next();
	if (NULL == track || 0 == track->mdhd.timescale)
	{
		return 0; // EOF
	}

	assert(track->sample_offset < track->sample_count);
	sample = track->samples[track->sample_offset].get();
	if (bytes < sample->bytes)
		return ENOMEM;

	seek(sample->offset);
	read((char*)buffer, sample->bytes);
	// if (mov_buffer_error(&reader->mov.io))
	// {
	// 	return mov_buffer_error(&reader->mov.io);
	// }

	track->sample_offset++; //mark as read
	assert(sample->sample_description_index > 0);

    auto frame = make_shared<StreamBuffer>();
    frame->assign((char*)buffer, sample->bytes);
    onFrame(frame, track->tkhd.track_ID, sample->pts * 1000 / track->mdhd.timescale, sample->dts * 1000 / track->mdhd.timescale, sample->flags);

	return 1;
}

int MP4Demuxer::mov_reader_read2()
{
    struct mov_track_t* track;
    struct mov_sample_t* sample;

    track = mov_reader_next();
    if (NULL == track || 0 == track->mdhd.timescale)
    {
        return 0; // EOF
    }

    assert(track->sample_offset < track->sample_count);
    sample = track->samples[track->sample_offset].get();

    auto frame = make_shared<StreamBuffer>();
    frame->setCapacity(sample->bytes + 1);
    char *buffer = frame->data();

    seek(sample->offset);
    read(buffer, sample->bytes);
    // if (mov_buffer_error(&reader->mov.io))
    // {
    //     return mov_buffer_error(&reader->mov.io);
    // }

    track->sample_offset++; //mark as read
    assert(sample->sample_description_index > 0);
    onFrame(frame, track->tkhd.track_ID, sample->pts * 1000 / track->mdhd.timescale, sample->dts * 1000 / track->mdhd.timescale, sample->flags);
    return 1;
}

int MP4Demuxer::mov_reader_seek(int64_t* timestamp)
{
	int i;
	struct mov_track_t* track;

	// seek video track(s)
	for (i = 0; i < _track_count; i++)
	{
		track = _tracks[i].get();
		if (MOV_VIDEO == track->handler_type && track->stbl.stss_count > 0)
		{
			if (0 != mov_stss_seek(track, timestamp))
				return -1;
		}
	}

	// seek other track(s)
	for (i = 0; i < _track_count; i++)
	{
		track = _tracks[i].get();
		if (MOV_VIDEO == track->handler_type && track->stbl.stss_count > 0)
			continue; // seek done

		mov_sample_seek(track, *timestamp);
	}

	return 0;
}

int MP4Demuxer::mov_reader_getinfo(struct mov_reader_trackinfo_t *ontrack, void* param)
{
	int i;
	uint32_t j;
	struct mov_track_t* track;
    struct mov_sample_entry_t* entry;

	for (i = 0; i < _track_count; i++)
	{
		track = _tracks[i].get();
		for (j = 0; j < track->stsd.entry_count && j < 1 /* only the first */; j++)
		{
            entry = track->stsd.entries[j].get();
			switch (track->handler_type)
			{
			case MOV_VIDEO:
                if (entry->object_type_indication == MOV_OBJECT_H264) {
                    auto trackInfo = make_shared<H264Track>();
                    trackInfo->index_ = track->tkhd.track_ID;
                    trackInfo->_width = entry->u.visual.width;
                    trackInfo->_height = entry->u.visual.height;
                    trackInfo->_avcc = entry->extra_data;
                    trackInfo->codec_ = "h264";
                    trackInfo->payloadType_ = 96;
                    trackInfo->trackType_ = "video";
                    trackInfo->samplerate_ = 90000;

                    _mapTrackInfo.emplace(trackInfo->index_, trackInfo);
                } else if (entry->object_type_indication == MOV_OBJECT_HEVC) {
                    auto trackInfo = make_shared<H265Track>();
                    trackInfo->index_ = track->tkhd.track_ID;
                    trackInfo->_width = entry->u.visual.width;
                    trackInfo->_height = entry->u.visual.height;
                    trackInfo->_hvcc = entry->extra_data;
                    trackInfo->codec_ = "h265";
                    trackInfo->payloadType_ = 96;
                    trackInfo->trackType_ = "video";
                    trackInfo->samplerate_ = 90000;
                }
				break;

			case MOV_AUDIO:
                if (entry->object_type_indication == MOV_OBJECT_AAC) {
                    auto trackInfo = make_shared<AacTrack>();
                    trackInfo->index_ = track->tkhd.track_ID;
                    trackInfo->channel_ = entry->u.audio.channelcount;
                    trackInfo->codec_ = "aac";
                    trackInfo->payloadType_ = 97;
                    trackInfo->trackType_ = "audio";
                    trackInfo->samplerate_ = entry->u.audio.samplerate;
                    trackInfo->bitPerSample_ = entry->u.audio.samplesize;

                    _mapTrackInfo.emplace(trackInfo->index_, trackInfo);
                }
				break;

			// case MOV_SUBT:
			// case MOV_TEXT:
			// 	if (ontrack->onsubtitle) ontrack->onsubtitle(param, track->tkhd.track_ID, MOV_OBJECT_TEXT, entry->extra_data, entry->extra_data_size);
			// 	break;

			default:
				break;
			}
		}	
	}
	return 0;
}

uint64_t MP4Demuxer::mov_reader_getduration()
{
	return 0 != _mvhd.timescale ? _mvhd.duration * 1000 / _mvhd.timescale : 0;
}

#define DIFF(a, b) ((a) > (b) ? ((a) - (b)) : ((b) - (a)))

int MP4Demuxer::mov_stss_seek(struct mov_track_t* track, int64_t *timestamp)
{
	int64_t clock;
	size_t start, end, mid;
	size_t idx, prev, next;
	struct mov_sample_t* sample;

	idx = mid = start = 0;
	end = track->stbl.stss_count;
	assert(track->stbl.stss_count > 0);
	clock = *timestamp * track->mdhd.timescale / 1000; // mvhd timescale

	while (start < end)
	{
		mid = (start + end) / 2;
		idx = track->stbl.stss[mid];

		if (idx < 1 || idx > track->sample_count)
		{
			// start from 1
			assert(0);
			return -1;
		}
		idx -= 1;
		sample = track->samples[idx].get();
		
		if (sample->dts > clock)
			end = mid;
		else if (sample->dts < clock)
			start = mid + 1;
		else
			break;
	}

	prev = track->stbl.stss[mid > 0 ? mid - 1 : mid] - 1;
	next = track->stbl.stss[mid + 1 < track->stbl.stss_count ? mid + 1 : mid] - 1;
	if (DIFF(track->samples[prev]->dts, clock) < DIFF(track->samples[idx]->dts, clock))
		idx = prev;
	if (DIFF(track->samples[next]->dts, clock) < DIFF(track->samples[idx]->dts, clock))
		idx = next;

	*timestamp = track->samples[idx]->dts * 1000 / track->mdhd.timescale;
	track->sample_offset = idx;
	return 0;
}

int MP4Demuxer::mov_sample_seek(struct mov_track_t* track, int64_t timestamp)
{
	size_t prev, next;
	size_t start, end, mid;
	struct mov_sample_t* sample;

	if (track->sample_count < 1)
		return -1;

	sample = NULL;
	mid = start = 0;
	end = track->sample_count;
	timestamp = timestamp * track->mdhd.timescale / 1000; // mvhd timecale

	while (start < end)
	{
		mid = (start + end) / 2;
		sample = track->samples[mid].get();
		
		if (sample->dts > timestamp)
			end = mid;
		else if (sample->dts < timestamp)
			start = mid + 1;
		else
			break;
	}

	prev = mid > 0 ? mid - 1 : mid;
	next = mid + 1 < track->sample_count ? mid + 1 : mid;
	if (DIFF(track->samples[prev]->dts, timestamp) < DIFF(track->samples[mid]->dts, timestamp))
		mid = prev;
	if (DIFF(track->samples[next]->dts, timestamp) < DIFF(track->samples[mid]->dts, timestamp))
		mid = next;

	track->sample_offset = mid;
	return 0;
}

int MP4Demuxer::mov_read_av1c(const struct mov_box_t* box)
{
	struct mov_track_t* track = _track.get();
	struct mov_sample_entry_t* entry = track->stsd.current.get();
	if (entry->extra_data_size < box->size)
	{
		// void* p = realloc(entry->extra_data, (size_t)box->size);
		// if (NULL == p) return ENOMEM;
		// entry->extra_data = p;
        entry->extra_data.resize(box->size);
	}

	read((char*)entry->extra_data.data(), box->size);
	entry->extra_data_size = (int)box->size;
	return 0;
}

int MP4Demuxer::mov_read_avcc(const struct mov_box_t* box)
{
	struct mov_track_t* track = _track.get();
	struct mov_sample_entry_t* entry = track->stsd.current.get();
	if (entry->extra_data_size < box->size)
	{
		// void* p = realloc(entry->extra_data, (size_t)box->size);
		// if (NULL == p) return ENOMEM;
		// entry->extra_data = p;
        entry->extra_data.resize(box->size);
	}

	read((char*)entry->extra_data.data(), box->size);
	entry->extra_data_size = (int)box->size;
	return 0;
}

// 8.6.1.3 Composition Time to Sample Box (p47)
int MP4Demuxer::mov_read_ctts(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	struct mov_stbl_t* stbl = &_track->stbl;

	read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	assert(0 == stbl->ctts_count && stbl->ctts.empty()); // duplicated CTTS atom
	if (stbl->ctts_count < entry_count)
	{
		// void* p = realloc(stbl->ctts, sizeof(struct mov_stts_t) * entry_count);
		// if (NULL == p) return ENOMEM;
		// stbl->ctts = (struct mov_stts_t*)p;        

        for (int i = 0; i < entry_count; ++i) {
            auto it = make_shared<mov_stts_t>();
            stbl->ctts.emplace_back(it);
        }
	}
	stbl->ctts_count = entry_count;

	for (i = 0; i < entry_count; i++)
	{
		stbl->ctts[i]->sample_count = read32BE();
		stbl->ctts[i]->sample_delta = read32BE(); // parse at int32_t
	}

	(void)box;
	return 0;
}

// 8.6.1.4 Composition to Decode Box (p53)
int MP4Demuxer::mov_read_cslg(const struct mov_box_t* box)
{
	uint8_t version;
//	struct mov_stbl_t* stbl = &_track->stbl;

	version = (uint8_t)read8BE(); /* version */
	read24BE(); /* flags */

	if (0 == version)
	{
		read32BE(); /* compositionToDTSShift */
		read32BE(); /* leastDecodeToDisplayDelta */
		read32BE(); /* greatestDecodeToDisplayDelta */
		read32BE(); /* compositionStartTime */
		read32BE(); /* compositionEndTime */
	}
	else
	{
		read64BE();
		read64BE();
		read64BE();
		read64BE();
		read64BE();
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_dops(const struct mov_box_t* box)
{
    struct mov_track_t* track = _track.get();
    struct mov_sample_entry_t* entry = track->stsd.current.get();
    if(box->size >= 10)
    {
        if (entry->extra_data_size < box->size + 8)
        {
            // void* p = realloc(entry->extra_data, (size_t)box->size + 8);
            // if (NULL == p) return ENOMEM;
            // entry->extra_data = p;

            entry->extra_data.resize(box->size + 8);
        }
        
        memcpy((char*)entry->extra_data.data(), "OpusHead", 8);
        entry->extra_data[8] = 1; // OpusHead version
        read8BE(); // version 0
        entry->extra_data[9] = read8BE(); // channel
        entry->extra_data[11] = read8BE(); // PreSkip (MSB -> LSB)
        entry->extra_data[10] = read8BE();
        entry->extra_data[15] = read8BE(); // InputSampleRate (LSB -> MSB)
        entry->extra_data[14] = read8BE();
        entry->extra_data[13] = read8BE();
        entry->extra_data[12] = read8BE();
        entry->extra_data[17] = read8BE(); // OutputGain (LSB -> MSB)
        entry->extra_data[16] = read8BE();
        read((char*)entry->extra_data.data() + 18, (size_t)box->size - 10);
        entry->extra_data_size = (int)box->size + 8;
    }
    return 0;
}

int MP4Demuxer::mov_read_elst(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	uint32_t version;
	struct mov_track_t* track = _track.get();

	version = read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	assert(0 == track->elst_count && track->elst.empty());
	if (track->elst_count < entry_count)
	{
		// void* p = realloc(track->elst, sizeof(struct mov_elst_t) * entry_count);
		// if (NULL == p) return ENOMEM;
		// track->elst = (struct mov_elst_t*)p;        

        for (int i = 0; i < entry_count; ++i) {
            auto it = make_shared<mov_elst_t>();
            track->elst.emplace_back(it);
        }
	}
	track->elst_count = entry_count;

	for (i = 0; i < entry_count; i++)
	{
		if (1 == version)
		{
			track->elst[i]->segment_duration = read64BE();
			track->elst[i]->media_time = (int64_t)read64BE();
		}
		else
		{
			assert(0 == version);
			track->elst[i]->segment_duration = read32BE();
			track->elst[i]->media_time = (int32_t)read32BE();
		}
		track->elst[i]->media_rate_integer = (int16_t)read16BE();
		track->elst[i]->media_rate_fraction = (int16_t)read16BE();
	}

	(void)box;
	return 0;
}


int MP4Demuxer::mov_read_base_descr(int bytes, int* tag,  int* len)
{
	int i;
	uint32_t c;

	*tag = read8BE();
	*len = 0;
	c = 0x80;
	for (i = 0; i < 4 && i + 1 < bytes && 0 != (c & 0x80); i++)
	{
		c = read8BE();
		*len = (*len << 7) | (c & 0x7F);
		//if (0 == (c & 0x80))
		//	break;
	}
	return 1 + i;
}

// ISO/IEC 14496-1:2010(E) 7.2.6.5 ES_Descriptor (p47)
/*
class ES_Descriptor extends BaseDescriptor : bit(8) tag=ES_DescrTag {
	bit(16) ES_ID;
	bit(1) streamDependenceFlag;
	bit(1) URL_Flag;
	bit(1) OCRstreamFlag;
	bit(5) streamPriority;
	if (streamDependenceFlag)
		bit(16) dependsOn_ES_ID;
	if (URL_Flag) {
		bit(8) URLlength;
		bit(8) URLstring[URLlength];
	}
	if (OCRstreamFlag)
		bit(16) OCR_ES_Id;
	DecoderConfigDescriptor decConfigDescr;
	if (ODProfileLevelIndication==0x01) //no SL extension.
	{
		SLConfigDescriptor slConfigDescr;
	}
	else // SL extension is possible.
	{
		SLConfigDescriptor slConfigDescr;
	}
	IPI_DescrPointer ipiPtr[0 .. 1];
	IP_IdentificationDataSet ipIDS[0 .. 255];
	IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
	LanguageDescriptor langDescr[0 .. 255];
	QoS_Descriptor qosDescr[0 .. 1];
	RegistrationDescriptor regDescr[0 .. 1];
	ExtensionDescriptor extDescr[0 .. 255];
}
*/
int MP4Demuxer::mp4_read_es_descriptor(uint64_t bytes)
{
	uint64_t p1, p2;
	p1 = tell();
	/*uint32_t ES_ID = */read16BE();
	uint32_t flags = read8BE();
	if (flags & 0x80) //streamDependenceFlag
		read16BE();
	if (flags & 0x40) { //URL_Flag
		uint32_t n = read8BE();
		skip(n);
	}

	if (flags & 0x20) //OCRstreamFlag
		read16BE();

	p2 = tell();
	return mp4_read_tag(bytes - (p2 - p1));
}

// ISO/IEC 14496-1:2010(E) 7.2.6.7 DecoderSpecificInfo (p51)
/*
abstract class DecoderSpecificInfo extends BaseDescriptor : bit(8)
	tag=DecSpecificInfoTag
{
	// empty. To be filled by classes extending this class.
}
*/
int MP4Demuxer::mp4_read_decoder_specific_info(int len)
{
	struct mov_track_t* track = _track.get();
	struct mov_sample_entry_t* entry = track->stsd.current.get();
	if (entry->extra_data_size < len)
	{
		// void* p = realloc(entry->extra_data, len);
		// if (NULL == p) return ENOMEM;
		// entry->extra_data = p;

        entry->extra_data.resize(len);
	}

	read((char*)entry->extra_data.data(), len);
	entry->extra_data_size = len;
	return 0;
}

// ISO/IEC 14496-1:2010(E) 7.2.6.6 DecoderConfigDescriptor (p48)
/*
class DecoderConfigDescriptor extends BaseDescriptor : bit(8) tag=DecoderConfigDescrTag {
	bit(8) objectTypeIndication;
	bit(6) streamType;
	bit(1) upStream;
	const bit(1) reserved=1;
	bit(24) bufferSizeDB;
	bit(32) maxBitrate;
	bit(32) avgBitrate;
	DecoderSpecificInfo decSpecificInfo[0 .. 1];
	profileLevelIndicationIndexDescriptor profileLevelIndicationIndexDescr[0..255];
}
*/
int MP4Demuxer::mp4_read_decoder_config_descriptor(int len)
{
	struct mov_sample_entry_t* entry = _track->stsd.current.get();
	entry->object_type_indication = (uint8_t)read8BE(); /* objectTypeIndication */
	entry->stream_type = (uint8_t)read8BE() >> 2; /* stream type */
	/*uint32_t bufferSizeDB = */read24BE(); /* buffer size db */
	/*uint32_t max_rate = */read32BE(); /* max bit-rate */
	/*uint32_t bit_rate = */read32BE(); /* avg bit-rate */
	return mp4_read_tag((uint64_t)len - 13); // mp4_read_decoder_specific_info
}

// ISO/IEC 14496-1:2010(E) 7.3.2.3 SL Packet Header Configuration (p92)
/*
class SLConfigDescriptor extends BaseDescriptor : bit(8) tag=SLConfigDescrTag {
	bit(8) predefined;
	if (predefined==0) {
		bit(1) useAccessUnitStartFlag;
		bit(1) useAccessUnitEndFlag;
		bit(1) useRandomAccessPointFlag;
		bit(1) hasRandomAccessUnitsOnlyFlag;
		bit(1) usePaddingFlag;
		bit(1) useTimeStampsFlag;
		bit(1) useIdleFlag;
		bit(1) durationFlag;
		bit(32) timeStampResolution;
		bit(32) OCRResolution;
		bit(8) timeStampLength; // must be �� 64
		bit(8) OCRLength; // must be �� 64
		bit(8) AU_Length; // must be �� 32
		bit(8) instantBitrateLength;
		bit(4) degradationPriorityLength;
		bit(5) AU_seqNumLength; // must be �� 16
		bit(5) packetSeqNumLength; // must be �� 16
		bit(2) reserved=0b11;
	}
	if (durationFlag) {
		bit(32) timeScale;
		bit(16) accessUnitDuration;
		bit(16) compositionUnitDuration;
	}
	if (!useTimeStampsFlag) {
		bit(timeStampLength) startDecodingTimeStamp;
		bit(timeStampLength) startCompositionTimeStamp;
	}
}

class ExtendedSLConfigDescriptor extends SLConfigDescriptor : bit(8)
tag=ExtSLConfigDescrTag {
	SLExtensionDescriptor slextDescr[1..255];
}
*/
int MP4Demuxer::mp4_read_sl_config_descriptor()
{
	int flags = 0;
	int predefined = read8BE();
	if (0 == predefined)
	{
		flags = read8BE();
		/*uint32_t timeStampResolution = */read32BE();
		/*uint32_t OCRResolution = */read32BE();
		/*int timeStampLength = */read8BE();
		/*int OCRLength = */read8BE();
		/*int AU_Length = */read8BE();
		/*int instantBitrateLength = */read8BE();
		/*uint16_t length = */read16BE();
	}
	else if (1 == predefined) // null SL packet header
	{
		flags = 0x00;
		//int TimeStampResolution = 1000;
		//int timeStampLength = 32;
	}
	else if (2 == predefined) // Reserved for use in MP4 files
	{ 
		// Table 14 �� Detailed predefined SLConfigDescriptor values (p93)
		flags = 0x04;
	}

	// durationFlag
	if (flags & 0x01)
	{
		/*uint32_t timeScale = */read32BE();
		/*uint16_t accessUnitDuration = */read16BE();
		/*uint16_t compositionUnitDuration = */read16BE();
	}

	// useTimeStampsFlag
	if (0 == (flags & 0x04))
	{
		//uint64_t startDecodingTimeStamp = 0; // file_reader_rb8(timeStampLength / 8)
		//uint64_t startCompositionTimeStamp = 0; // file_reader_rb8(timeStampLength / 8)
	}
	return 0;
}

int MP4Demuxer::mp4_read_tag(uint64_t bytes)
{
	int tag, len;
	uint64_t p1, p2, offset;

	for (offset = 0; offset < bytes; offset += len)
	{
		tag = len = 0;
		offset += mov_read_base_descr((int)(bytes - offset), &tag, &len);
		if (offset + len > bytes)
			break;

		p1 = tell();
		switch (tag)
		{
		case ISO_ESDescrTag:
			mp4_read_es_descriptor(len);
			break;

		case ISO_DecoderConfigDescrTag:
			mp4_read_decoder_config_descriptor(len);
			break;

		case ISO_DecSpecificInfoTag:
			mp4_read_decoder_specific_info(len);
			break;

		case ISO_SLConfigDescrTag:
			mp4_read_sl_config_descriptor();
			break;

		default:
			break;
		}

		p2 = tell();
		skip(len - (p2 - p1));
	}

	return 0;
}

// ISO/IEC 14496-14:2003(E) 5.6 Sample Description Boxes (p15)
int MP4Demuxer::mov_read_esds(const struct mov_box_t* box)
{
	read8BE(); /* version */
	read24BE(); /* flags */
	return mp4_read_tag(box->size - 4);
}

int MP4Demuxer::mov_read_ftyp(const struct mov_box_t* box)
{
	if(box->size < 8) return -1;

	_ftyp.major_brand = read32BE();
	_ftyp.minor_version = read32BE();

	for(_ftyp.brands_count = 0; _ftyp.brands_count < N_BRAND && (uint64_t)_ftyp.brands_count * 4 < box->size - 8; ++_ftyp.brands_count)
	{
		_ftyp.compatible_brands[_ftyp.brands_count] = read32BE();
	}

	assert(box->size == 4 * (uint64_t)_ftyp.brands_count + 8);
	skip(box->size - 4 * (uint64_t)_ftyp.brands_count - 8 ); // skip compatible_brands
	return 0;
}

int MP4Demuxer::mov_read_hdlr(const struct mov_box_t* box)
{
	struct mov_track_t* track = _track.get();

	read8BE(); /* version */
	read24BE(); /* flags */
	//uint32_t pre_defined = read32BE();
	skip(4);
	track->handler_type = read32BE();
	// const unsigned int(32)[3] reserved = 0;
	skip(12);
	// string name;
	skip(box->size - 24); // String name
	return 0;
}

int MP4Demuxer::mov_read_hvcc(const struct mov_box_t* box)
{
	struct mov_track_t* track = _track.get();
	struct mov_sample_entry_t* entry = track->stsd.current.get();
	if (entry->extra_data_size < box->size)
	{
		// void* p = realloc(entry->extra_data, (size_t)box->size);
		// if (NULL == p) return ENOMEM;
		// entry->extra_data = p;

        entry->extra_data.resize(box->size);
	}

	read((char*)entry->extra_data.data(), (size_t)box->size);
	entry->extra_data_size = (int)box->size;
	return 0;
}

int MP4Demuxer::mov_read_leva(const struct mov_box_t* box)
{
	unsigned int i, level_count;
	unsigned int assignment_type;

	read32BE(); /* version & flags */
	level_count = read8BE(); /* level_count */
	for (i = 0; i < level_count; i++)
	{
		read32BE(); /* track_id */
		assignment_type = read8BE(); /* padding_flag & assignment_type */
		assignment_type &= 0x7F; // 7-bits

		if (0 == assignment_type)
		{
			read32BE(); /* grouping_type */
		}
		else if (1 == assignment_type)
		{
			read32BE(); /* grouping_type */
			read32BE(); /* grouping_type_parameter */
		}
		else if (4 == assignment_type)
		{
			read32BE(); /* sub_track_id */
		}
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_mdhd(const struct mov_box_t* box)
{
	uint32_t val;
	struct mov_mdhd_t* mdhd = &_track->mdhd;

	mdhd->version = read8BE();
	mdhd->flags = read24BE();

	if (1 == mdhd->version)
	{
		mdhd->creation_time = read64BE();
		mdhd->modification_time = read64BE();
		mdhd->timescale = read32BE();
		mdhd->duration = read64BE();
	}
	else
	{
		assert(0 == mdhd->version);
		mdhd->creation_time = read32BE();
		mdhd->modification_time = read32BE();
		mdhd->timescale = read32BE();
		mdhd->duration = read32BE();
	}

	val = read32BE();
	mdhd->language = (val >> 16) & 0x7FFF;
	mdhd->pre_defined = val & 0xFFFF;

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_mehd(const struct mov_box_t* box)
{
    unsigned int version;
    uint64_t fragment_duration;
    version = read8BE(); /* version */
    read24BE(); /* flags */

    if (1 == version)
        fragment_duration = read64BE(); /* fragment_duration*/
    else
        fragment_duration = read32BE(); /* fragment_duration*/

    (void)box;
    //assert(fragment_duration <= _mvhd.duration);
    return 0;
}

int MP4Demuxer::mov_read_mfhd(const struct mov_box_t* box)
{
    (void)box;
    read32BE(); /* version & flags */
    read32BE(); /* sequence_number */
    return 0;
}

int MP4Demuxer::mov_read_mvhd(const struct mov_box_t* box)
{
	int i;
	struct mov_mvhd_t* mvhd = &_mvhd;

	mvhd->version = read8BE();
	mvhd->flags = read24BE();

	if (1 == mvhd->version)
	{
		mvhd->creation_time = read64BE();
		mvhd->modification_time = read64BE();
		mvhd->timescale = read32BE();
		mvhd->duration = read64BE();
	}
	else
	{
		assert(0 == mvhd->version);
		mvhd->creation_time = read32BE();
		mvhd->modification_time = read32BE();
		mvhd->timescale = read32BE();
		mvhd->duration = read32BE();
	}

	mvhd->rate = read32BE();
	mvhd->volume = (uint16_t)read16BE();
	//mvhd->reserved = read16BE();
	//mvhd->reserved2[0] = read32BE();
	//mvhd->reserved2[1] = read32BE();
	skip(10);
	for (i = 0; i < 9; i++)
		mvhd->matrix[i] = read32BE();
#if 0
	for (i = 0; i < 6; i++)
		mvhd->pre_defined[i] = read32BE();
#else
	read32BE(); /* preview time */
	read32BE(); /* preview duration */
	read32BE(); /* poster time */
	read32BE(); /* selection time */
	read32BE(); /* selection duration */
	read32BE(); /* current time */
#endif
	mvhd->next_track_ID = read32BE();

	(void)box;
	return 0;
}

int MP4Demuxer::mp4_read_extra(const struct mov_box_t* box)
{
	int r;
	uint64_t p1, p2;
	p1 = tell();
	r = mov_reader_box(box);
	p2 = tell();
	skip(box->size - (p2 - p1));
	return r;
}

/*
aligned(8) abstract class SampleEntry (unsigned int(32) format) 
	extends Box(format){ 
	const unsigned int(8)[6] reserved = 0; 
	unsigned int(16) data_reference_index; 
}
*/
int MP4Demuxer::mov_read_sample_entry(struct mov_box_t* box, uint16_t* data_reference_index)
{
	box->size = read32BE();
	box->type = read32BE();
	skip(6); // const unsigned int(8)[6] reserved = 0;
	*data_reference_index = (uint16_t)read16BE(); // ref [dref]
	return 0;
}

/*
class AudioSampleEntry(codingname) extends SampleEntry (codingname){ 
	const unsigned int(32)[2] reserved = 0; 
	template unsigned int(16) channelcount = 2; 
	template unsigned int(16) samplesize = 16; 
	unsigned int(16) pre_defined = 0; 
	const unsigned int(16) reserved = 0 ; 
	template unsigned int(32) samplerate = { default samplerate of media}<<16; 
}
*/
int MP4Demuxer::mov_read_audio(struct mov_sample_entry_t* entry)
{
	uint16_t qtver;
	struct mov_box_t box;
	mov_read_sample_entry(&box, &entry->data_reference_index);
    entry->object_type_indication = mov_tag_to_object(box.type);
    entry->stream_type = MP4_STREAM_AUDIO;
	_track->tag = box.type;

#if 0
	// const unsigned int(32)[2] reserved = 0;
	skip(8);
#else
	qtver = read16BE(); /* version */
	read16BE(); /* revision level */
	read32BE(); /* vendor */
#endif

    entry->u.audio.channelcount = (uint16_t)read16BE();
    entry->u.audio.samplesize = (uint16_t)read16BE();

#if 0
	// unsigned int(16) pre_defined = 0; 
	// const unsigned int(16) reserved = 0 ;
	skip(4);
#else
	read16BE(); /* audio cid */
	read16BE(); /* packet size = 0 */
#endif

    entry->u.audio.samplerate = read32BE(); // { default samplerate of media}<<16;

	// audio extra(avc1: ISO/IEC 14496-14:2003(E))
	box.size -= 36;

	// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-124774
	if (1 == qtver && box.size >= 16)
	{
		// Sound Sample Description (Version 1)
		read32BE(); // Samples per packet
		read32BE(); // Bytes per packet
		read32BE(); // Bytes per frame
		read32BE(); // Bytes per sample
		box.size -= 16;
	}
	else if (2 == qtver && box.size >= 36)
	{
		// Sound Sample Description (Version 2)
		read32BE(); // sizeOfStructOnly
		read64BE(); // audioSampleRate
		read32BE(); // numAudioChannels
		read32BE(); // always7F000000
		read32BE(); // constBitsPerChannel
		read32BE(); // formatSpecificFlags
		read32BE(); // constBytesPerAudioPacket
		read32BE(); // constLPCMFramesPerAudioPacket
		box.size -= 36;
	}

	return mp4_read_extra(&box);
}

/*
class VisualSampleEntry(codingname) extends SampleEntry (codingname){ 
	unsigned int(16) pre_defined = 0; 
	const unsigned int(16) reserved = 0; 
	unsigned int(32)[3] pre_defined = 0; 
	unsigned int(16) width; 
	unsigned int(16) height; 
	template unsigned int(32) horizresolution = 0x00480000; // 72 dpi 
	template unsigned int(32) vertresolution = 0x00480000; // 72 dpi 
	const unsigned int(32) reserved = 0; 
	template unsigned int(16) frame_count = 1; 
	string[32] compressorname; 
	template unsigned int(16) depth = 0x0018; 
	int(16) pre_defined = -1; 
	// other boxes from derived specifications 
	CleanApertureBox clap; // optional 
	PixelAspectRatioBox pasp; // optional 
}
class AVCSampleEntry() extends VisualSampleEntry ('avc1'){
	AVCConfigurationBox config;
	MPEG4BitRateBox (); // optional
	MPEG4ExtensionDescriptorsBox (); // optional
}
class AVC2SampleEntry() extends VisualSampleEntry ('avc2'){
	AVCConfigurationBox avcconfig;
	MPEG4BitRateBox bitrate; // optional
	MPEG4ExtensionDescriptorsBox descr; // optional
	extra_boxes boxes; // optional
}
*/
int MP4Demuxer::mov_read_video(struct mov_sample_entry_t* entry)
{
	struct mov_box_t box;
	mov_read_sample_entry(&box, &entry->data_reference_index);
    entry->object_type_indication = mov_tag_to_object(box.type);
    entry->stream_type = MP4_STREAM_VISUAL;
	_track->tag = box.type;
#if 1
	 //unsigned int(16) pre_defined = 0; 
	 //const unsigned int(16) reserved = 0;
	 //unsigned int(32)[3] pre_defined = 0;
	skip(16);
#else
	read16BE(); /* version */
	read16BE(); /* revision level */
	read32BE(); /* vendor */
	read32BE(); /* temporal quality */
	read32BE(); /* spatial quality */
#endif
    entry->u.visual.width = (uint16_t)read16BE();
    entry->u.visual.height = (uint16_t)read16BE();
    entry->u.visual.horizresolution = read32BE(); // 0x00480000 - 72 dpi
    entry->u.visual.vertresolution = read32BE(); // 0x00480000 - 72 dpi
	// const unsigned int(32) reserved = 0;
	read32BE(); /* data size, always 0 */
    entry->u.visual.frame_count = (uint16_t)read16BE();

	//string[32] compressorname;
	//uint32_t len = read8BE();
	//skip(len);
	skip(32);

    entry->u.visual.depth = (uint16_t)read16BE();
	// int(16) pre_defined = -1;
	skip(2);

	// video extra(avc1: ISO/IEC 14496-15:2010(E))
	box.size -= 86;
	return mp4_read_extra(&box);
}

/*
class PixelAspectRatioBox extends Box(�pasp�){
	unsigned int(32) hSpacing;
	unsigned int(32) vSpacing;
}
*/
int MP4Demuxer::mov_read_pasp(const struct mov_box_t* box)
{
	read32BE();
	read32BE();

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_hint_sample_entry(struct mov_sample_entry_t* entry)
{
	struct mov_box_t box;
	mov_read_sample_entry(&box, &entry->data_reference_index);
	skip(box.size - 16);
	_track->tag = box.type;
	return 0;
}

int MP4Demuxer::mov_read_meta_sample_entry(struct mov_sample_entry_t* entry)
{
	struct mov_box_t box;
	mov_read_sample_entry(&box, &entry->data_reference_index);
	skip(box.size - 16);
	_track->tag = box.type;
	return 0;
}

// ISO/IEC 14496-12:2015(E) 12.5 Text media (p184)
/*
class PlainTextSampleEntry(codingname) extends SampleEntry (codingname) {
}
class SimpleTextSampleEntry(codingname) extends PlainTextSampleEntry ('stxt') {
	string content_encoding; // optional
	string mime_format;
	BitRateBox (); // optional
	TextConfigBox (); // optional
}
*/
int MP4Demuxer::mov_read_text_sample_entry(struct mov_sample_entry_t* entry)
{
	struct mov_box_t box;
	mov_read_sample_entry(&box, &entry->data_reference_index);
	if (MOV_TEXT == box.type)
	{
		// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-69835
		//read32BE(); /* display flags */
		//read32BE(); /* text justification */
		//read16BE(); /* background color: 48-bit RGB color */
		//read16BE();
		//read16BE();
		//read64BE(); /* default text box (top, left, bottom, right) */
		//read64BE(); /* reserved */
		//read16BE(); /* font number */
		//read16BE(); /* font face */
		//read8BE(); /* reserved */
		//read16BE(); /* reserved */
		//read16BE(); /* foreground  color: 48-bit RGB color */
		//read16BE();
		//read16BE();
		////read16BE(); /* text name */
		skip(box.size - 16);
	}
	else
	{
		skip(box.size - 16);
	}

	_track->tag = box.type;
	return 0;
}

// ISO/IEC 14496-12:2015(E) 12.6 Subtitle media (p185)
/*
class SubtitleSampleEntry(codingname) extends SampleEntry (codingname) {
}
class XMLSubtitleSampleEntry() extends SubtitleSampleEntry('stpp') {
	string namespace;
	string schema_location; // optional
	string auxiliary_mime_types;
	// optional, required if auxiliary resources are present
	BitRateBox (); // optional
}
class TextSubtitleSampleEntry() extends SubtitleSampleEntry('sbtt') {
	string content_encoding; // optional
	string mime_format;
	BitRateBox (); // optional
	TextConfigBox (); // optional
}
class TextSampleEntry() extends SampleEntry('tx3g') {
	unsigned int(32) displayFlags;
	signed int(8) horizontal-justification;
	signed int(8) vertical-justification;
	unsigned int(8) background-color-rgba[4];
	BoxRecord default-text-box;
	StyleRecord default-style;
	FontTableBox font-table;
	DisparityBox default-disparity;
}
*/

int MP4Demuxer::mov_read_subtitle_sample_entry(struct mov_sample_entry_t* entry)
{
	struct mov_box_t box;
	mov_read_sample_entry(&box, &entry->data_reference_index);
	if (box.type == MOV_TAG('t', 'x', '3', 'g'))
	{
		mov_read_tx3g(&box);
	}
	else
	{
		skip(box.size - 16);
	}

    entry->object_type_indication = MOV_OBJECT_TEXT;
    entry->stream_type = MP4_STREAM_VISUAL;
	_track->tag = box.type;
	return 0;
}

int MP4Demuxer::mov_read_stsd(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	struct mov_track_t* track = _track.get();

	read8BE();
	read24BE();
	entry_count = read32BE();

	if (track->stsd.entry_count < entry_count)
	{
		// void* p = realloc(track->stsd.entries, sizeof(track->stsd.entries[0]) * entry_count);
		// if (NULL == p) return ENOMEM;
		// track->stsd.entries = (struct mov_sample_entry_t*)p;

        for (int i = 0; i < entry_count; ++i) {
            auto entry = make_shared<mov_sample_entry_t>();
            track->stsd.entries.push_back(entry);
        }
	}

	track->stsd.entry_count = entry_count;
	for (i = 0; i < entry_count; i++)
	{
        track->stsd.current = track->stsd.entries[i];
		// memset(track->stsd.current, 0, sizeof(*track->stsd.current));
		if (MOV_AUDIO == track->handler_type)
		{
			mov_read_audio(track->stsd.entries[i].get());
		}
		else if (MOV_VIDEO == track->handler_type)
		{
			mov_read_video(track->stsd.entries[i].get());
		}
		else if (MOV_HINT == track->handler_type)
		{
			mov_read_hint_sample_entry(track->stsd.entries[i].get());
		}
		else if (MOV_META == track->handler_type)
		{
			mov_read_meta_sample_entry(track->stsd.entries[i].get());
		}
		else if (MOV_CLCP == track->handler_type)
		{
			mov_read_meta_sample_entry(track->stsd.entries[i].get());
		}
		else if (MOV_TEXT == track->handler_type)
		{
			mov_read_text_sample_entry(track->stsd.entries[i].get());
		}
		else if (MOV_SUBT == track->handler_type)
		{
			mov_read_subtitle_sample_entry(track->stsd.entries[i].get());
		}
		else if (MOV_ALIS == track->handler_type)
		{
			mov_read_meta_sample_entry(track->stsd.entries[i].get());
		}
		else
		{
			assert(0); // ignore
			mov_read_meta_sample_entry(track->stsd.entries[i].get());
		}
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_sidx(const struct mov_box_t* box)
{
	unsigned int version;
	unsigned int i, reference_count;

	version = read8BE(); /* version */
	read24BE(); /* flags */
	read32BE(); /* reference_ID */
	read32BE(); /* timescale */

	if (0 == version)
	{
		read32BE(); /* earliest_presentation_time */
		read32BE(); /* first_offset */
	}
	else
	{
		read64BE(); /* earliest_presentation_time */
		read64BE(); /* first_offset */
	}

	read16BE(); /* reserved */
	reference_count = read16BE(); /* reference_count */
	for (i = 0; i < reference_count; i++)
	{
		read32BE(); /* reference_type & referenced_size */
		read32BE(); /* subsegment_duration */
		read32BE(); /* starts_with_SAP & SAP_type & SAP_delta_time */
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_smdm(const struct mov_box_t* box)
{
    read8BE(); // version
    read24BE(); // flags
    
    read16BE(); // primaryRChromaticity_x, 0.16 fixed-point Red X chromaticity coordinate as defined by CIE 1931
    read16BE(); // primaryRChromaticity_y
    read16BE(); // primaryGChromaticity_x
    read16BE(); // primaryGChromaticity_y
    read16BE(); // primaryBChromaticity_x
    read16BE(); // primaryBChromaticity_y
    read16BE(); // whitePointChromaticity_x
    read16BE(); // whitePointChromaticity_y
    read32BE(); // luminanceMax, 24.8 fixed point Maximum luminance, represented in candelas per square meter (cd/m²)
    read32BE(); // luminanceMin
    
    return 0;
}

int MP4Demuxer::mov_read_coll(const struct mov_box_t* box)
{
    read8BE(); // version
    read24BE(); // flags
    
    read16BE(); // maxCLL, Maximum Content Light Level as specified in CEA-861.3, Appendix A.
    read16BE(); // maxFALL, Maximum Frame-Average Light Level as specified in CEA-861.3, Appendix A.
    return 0;
}

int MP4Demuxer::mov_read_vmhd(const struct mov_box_t* box)
{
	read8BE(); /* version */
	read24BE(); /* flags */
	read16BE(); /* graphicsmode */
	// template unsigned int(16)[3] opcolor = {0, 0, 0};
	skip(6);

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_smhd(const struct mov_box_t* box)
{
	read8BE(); /* version */
	read24BE(); /* flags */
	read16BE(); /* balance */
	//const unsigned int(16) reserved = 0;
	skip(2);

	(void)box;
	return 0;
}

// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25675
/*
Size: A 32-bit integer that specifies the number of bytes in this base media info atom.
Type: A 32-bit integer that identifies the atom type; this field must be set to 'gmin'.
Version: A 1-byte specification of the version of this base media information header atom.
Flags: A 3-byte space for base media information flags. Set this field to 0.
Graphics mode: A 16-bit integer that specifies the transfer mode. The transfer mode specifies which Boolean operation QuickDraw should perform when drawing or transferring an image from one location to another. See Graphics Modes for more information about graphics modes supported by QuickTime.
Opcolor: Three 16-bit values that specify the red, green, and blue colors for the transfer mode operation indicated in the graphics mode field.
Balance: A 16-bit integer that specifies the sound balance of this media. Sound balance is the setting that controls the mix of sound between the two speakers of a computer. This field is normally set to 0. See Balance for more information about balance values.
Reserved: Reserved for use by Apple. A 16-bit integer. Set this field to 0
*/
int MP4Demuxer::mov_read_gmin(const struct mov_box_t* box)
{
	read8BE(); /* version */
	read24BE(); /* flags */
	read16BE(); /* graphics mode */
	read16BE(); /* opcolor red*/
	read16BE(); /* opcolor green*/
	read16BE(); /* opcolor blue*/
	read16BE(); /* balance */
	read16BE(); /* reserved */

	(void)box;
	return 0;
}

// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-SW90
/*
Size:A 32-bit integer that specifies the number of bytes in this text media information atom.
Type:A 32-bit integer that identifies the atom type; this field must be set to 'text'.
Matrix structure:A matrix structure associated with this text media
*/
int MP4Demuxer::mov_read_text(const struct mov_box_t* box)
{
	int i;
	// Matrix structure
	for (i = 0; i < 9; i++)
		read32BE();

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_stco(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	struct mov_stbl_t* stbl = &_track->stbl;

	read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	assert(0 == stbl->stco_count && stbl->stco.empty());
	if (stbl->stco_count < entry_count)
	{
		// void* p = realloc(stbl->stco, sizeof(stbl->stco[0]) * entry_count);
		// if (NULL == p) return ENOMEM;
		// stbl->stco = p;

        stbl->stco.resize(entry_count);
	}
	stbl->stco_count = entry_count;

	if (MOV_TAG('s', 't', 'c', 'o') == box->type)
	{
		for (i = 0; i < entry_count; i++)
			stbl->stco[i] = read32BE(); // chunk_offset
	}
	else if (MOV_TAG('c', 'o', '6', '4') == box->type)
	{
		for (i = 0; i < entry_count; i++)
			stbl->stco[i] = read64BE(); // chunk_offset
	}
	else
	{
		i = 0;
		assert(0);
	}

	stbl->stco_count = i;
	return 0;
}

int MP4Demuxer::mov_read_stsc(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	struct mov_stbl_t* stbl = &_track->stbl;

	read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	assert(0 == stbl->stsc_count && stbl->stsc.empty()); // duplicated STSC atom
	if (stbl->stsc_count < entry_count)
	{
		// void* p = realloc(stbl->stsc, sizeof(struct mov_stsc_t) * (entry_count + 1/*stco count*/));
		// if (NULL == p) return ENOMEM;
		// stbl->stsc = (struct mov_stsc_t*)p;

        for (int i = 0; i < entry_count; ++i) {
            auto entry = make_shared<mov_stsc_t>();
            stbl->stsc.push_back(entry);
        }
	}
	stbl->stsc_count = entry_count;

	for (i = 0; i < entry_count; i++)
	{
		stbl->stsc[i]->first_chunk = read32BE();
		stbl->stsc[i]->samples_per_chunk = read32BE();
		stbl->stsc[i]->sample_description_index = read32BE();
	}

	(void)box;
	return 0;
}

// 8.6.2 Sync Sample Box (p50)
int MP4Demuxer::mov_read_stss(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	struct mov_stbl_t* stbl = &_track->stbl;

	read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	assert(0 == stbl->stss_count && stbl->stss.empty());
	if (stbl->stss_count < entry_count)
	{
		// void* p = realloc(stbl->stss, sizeof(stbl->stss[0]) * entry_count);
		// if (NULL == p) return ENOMEM;
		// stbl->stss = p;

        stbl->stss.resize(entry_count);
	}
	stbl->stss_count = entry_count;

	for (i = 0; i < entry_count; i++)
		stbl->stss[i] = read32BE(); // uint32_t sample_number

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_stsz(const struct mov_box_t* box)
{
	uint32_t i = 0, sample_size, sample_count;
	struct mov_track_t* track = _track.get();

	read8BE(); /* version */
	read24BE(); /* flags */
	sample_size = read32BE();
	sample_count = read32BE();

	assert(0 == track->sample_count && track->samples.empty()); // duplicated STSZ atom
	if (track->sample_count < sample_count)
	{
		// void* p = realloc(track->samples, sizeof(struct mov_sample_t) * (sample_count + 1));
		// if (NULL == p) return ENOMEM;
		// track->samples = (struct mov_sample_t*)p;
		// memset(track->samples, 0, sizeof(struct mov_sample_t) * (sample_count + 1));

        for (int i = 0; i < sample_count + 1; ++i) {
            auto entry = make_shared<mov_sample_t>();
            track->samples.push_back(entry);
        }
	}
	track->sample_count = sample_count;

	if (0 == sample_size)
	{
		for (i = 0; i < sample_count; i++)
			track->samples[i]->bytes = read32BE(); // uint32_t entry_size
	}
	else
	{
		for (i = 0; i < sample_count; i++)
			track->samples[i]->bytes = sample_size;
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_stts(const struct mov_box_t* box)
{
	uint32_t i, entry_count;
	struct mov_stbl_t* stbl = &_track->stbl;

	read8BE(); /* version */
	read24BE(); /* flags */
	entry_count = read32BE();

	assert(0 == stbl->stts_count && stbl->stts.empty()); // duplicated STTS atom
	if (stbl->stts_count < entry_count)
	{
		// void* p = realloc(stbl->stts, sizeof(struct mov_stts_t) * entry_count);
		// if (NULL == p) return ENOMEM;
		// stbl->stts = (struct mov_stts_t*)p;        

        for (int i = 0; i < entry_count; ++i) {
            auto entry = make_shared<mov_stts_t>();
            stbl->stts.push_back(entry);
        }
	}
	stbl->stts_count = entry_count;

	for (i = 0; i < entry_count; i++)
	{
		stbl->stts[i]->sample_count = read32BE();
		stbl->stts[i]->sample_delta = read32BE();
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_stz2(const struct mov_box_t* box)
{
	uint32_t i, v, field_size, sample_count;
	struct mov_track_t* track = _track.get();

	read8BE(); /* version */
	read24BE(); /* flags */
	// unsigned int(24) reserved = 0;
	read24BE(); /* reserved */
	field_size = read8BE();
	sample_count = read32BE();

	assert(4 == field_size || 8 == field_size || 16 == field_size);
	assert(0 == track->sample_count && track->samples.empty()); // duplicated STSZ atom
	if (track->sample_count < sample_count)
	{
		// void* p = realloc(track->samples, sizeof(struct mov_sample_t) * (sample_count + 1));
		// if (NULL == p) return ENOMEM;
		// track->samples = (struct mov_sample_t*)p;
		// memset(track->samples, 0, sizeof(struct mov_sample_t) * (sample_count + 1));

        for (int i = 0; i < sample_count + 1; ++i) {
            auto entry = make_shared<mov_sample_t>();
            track->samples.push_back(entry);
        }
	}
	track->sample_count = sample_count;

	if (4 == field_size)
	{
		for (i = 0; i < sample_count/2; i++)
		{
			v = read8BE();
			track->samples[i * 2]->bytes = (v >> 4) & 0x0F;
			track->samples[i * 2 + 1]->bytes = v & 0x0F;
		}
		if (sample_count % 2)
		{
			v = read8BE();
			track->samples[i * 2]->bytes = (v >> 4) & 0x0F;
		}
	}
	else if (8 == field_size)
	{
		for (i = 0; i < sample_count; i++)
			track->samples[i]->bytes = read8BE();
	}
	else if (16 == field_size)
	{
		for (i = 0; i < sample_count; i++)
			track->samples[i]->bytes = read16BE();
	}
	else
	{
		i = 0;
		assert(0);
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_tfdt(const struct mov_box_t* box)
{
    unsigned int version;
    version = read8BE(); /* version */
    read24BE(); /* flags */

    if (1 == version)
        _track->tfdt_dts = read64BE(); /* baseMediaDecodeTime */
    else
        _track->tfdt_dts = read32BE(); /* baseMediaDecodeTime */

    // baseMediaDecodeTime + ELST start offset
    mov_apply_elst_tfdt(_track.get());

	(void)box;
    return 0;
}

int MP4Demuxer::mov_read_tfhd(const struct mov_box_t* box)
{
	uint32_t flags;
	uint32_t track_ID;

	read8BE(); /* version */
	flags = read24BE(); /* flags */
	track_ID = read32BE(); /* track_ID */
	
	_track.reset(mov_find_track(track_ID));
	if (NULL == _track)
		return -1;

	_track->tfhd.flags = flags;

	if (MOV_TFHD_FLAG_BASE_DATA_OFFSET & flags)
		_track->tfhd.base_data_offset = read64BE(); /* base_data_offset*/
	else if(MOV_TFHD_FLAG_DEFAULT_BASE_IS_MOOF & flags)
		_track->tfhd.base_data_offset = _moof_offset; /* default-base-is-moof */
	else
		_track->tfhd.base_data_offset = _implicit_offset;

	if (MOV_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX & flags)
		_track->tfhd.sample_description_index = read32BE(); /* sample_description_index*/
	else
		_track->tfhd.sample_description_index = _track->trex.default_sample_description_index;

	if (MOV_TFHD_FLAG_DEFAULT_DURATION & flags)
		_track->tfhd.default_sample_duration = read32BE(); /* default_sample_duration*/
	else
		_track->tfhd.default_sample_duration = _track->trex.default_sample_duration;

	if (MOV_TFHD_FLAG_DEFAULT_SIZE & flags)
		_track->tfhd.default_sample_size = read32BE(); /* default_sample_size*/
	else
		_track->tfhd.default_sample_size = _track->trex.default_sample_size;

	if (MOV_TFHD_FLAG_DEFAULT_FLAGS & flags)
		_track->tfhd.default_sample_flags = read32BE(); /* default_sample_flags*/
	else
		_track->tfhd.default_sample_flags = _track->trex.default_sample_flags;

	if (MOV_TFHD_FLAG_DURATION_IS_EMPTY & flags)
		(void)box; /* duration-is-empty*/
	return 0;
}

int MP4Demuxer::mov_read_tfra(const struct mov_box_t* box)
{
	unsigned int version;
	uint32_t track_ID;
	uint32_t length_size_of;
	uint32_t i, j, number_of_entry;
	uint32_t traf_number, trun_number, sample_number;
	struct mov_track_t* track;

	version = read8BE(); /* version */
	read24BE(); /* flags */
	track_ID = read32BE(); /* track_ID */

	track = mov_find_track(track_ID);
	if (NULL == track)
		return -1;

	length_size_of = read32BE(); /* length_size_of XXX */
	number_of_entry = read32BE(); /* number_of_entry */
	for (i = 0; i < number_of_entry; i++)
	{
		if (1 == version)
		{
			read64BE(); /* time */
			read64BE(); /* moof_offset */
		}
		else
		{
			read32BE(); /* time */
			read32BE(); /* moof_offset */
		}

		for (traf_number = 0, j = 0; j < ((length_size_of >> 4) & 0x03) + 1; j++)
			traf_number = (traf_number << 8) | read8BE(); /* traf_number */

		for (trun_number = 0, j = 0; j < ((length_size_of >> 2) & 0x03) + 1; j++)
			trun_number = (trun_number << 8) | read8BE(); /* trun_number */

		for (sample_number = 0, j = 0; j < (length_size_of & 0x03) + 1; j++)
			sample_number = (sample_number << 8) | read8BE(); /* sample_number */
	}

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_tkhd(const struct mov_box_t* box)
{
	int i;
    uint8_t version;
    uint32_t flags;
    uint64_t creation_time;
    uint64_t modification_time;
    uint64_t duration;
    uint32_t track_ID;
	struct mov_tkhd_t* tkhd;
    struct mov_track_t* track;

	version = read8BE();
	flags = read24BE();

	if (1 == version)
	{
		creation_time = read64BE();
		modification_time = read64BE();
		track_ID = read32BE();
		/*reserved = */read32BE();
		duration = read64BE();
	}
	else
	{
		assert(0 == version);
		creation_time = read32BE();
		modification_time = read32BE();
		track_ID = read32BE();
		/*reserved = */read32BE();
		duration = read32BE();
	}
    skip(8); // const unsigned int(32)[2] reserved = 0;

    track = mov_fetch_track(track_ID);
    if (NULL == track) return -1;

    _track.reset(track);
    tkhd = &_track->tkhd;
	tkhd->version = version;
    tkhd->flags = flags;
    tkhd->duration = duration;
    tkhd->creation_time = creation_time;
    tkhd->modification_time = modification_time;

	tkhd->layer = (int16_t)read16BE();
	tkhd->alternate_group = (int16_t)read16BE();
	tkhd->volume = (int16_t)read16BE();
	skip(2); // const unsigned int(16) reserved = 0;
	for (i = 0; i < 9; i++)
		tkhd->matrix[i] = read32BE();
	tkhd->width = read32BE();
	tkhd->height = read32BE();

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_trex(const struct mov_box_t* box)
{
	uint32_t track_ID;
	struct mov_track_t* track;

	(void)box;
	read32BE(); /* version & flags */
	track_ID = read32BE(); /* track_ID */

	track = mov_fetch_track(track_ID);
    if (NULL == track) return -1;

	track->trex.default_sample_description_index = read32BE(); /* default_sample_description_index */
	track->trex.default_sample_duration = read32BE(); /* default_sample_duration */
	track->trex.default_sample_size = read32BE(); /* default_sample_size */
	track->trex.default_sample_flags = read32BE(); /* default_sample_flags */
	return 0;
}

int MP4Demuxer::mov_read_trun(const struct mov_box_t* box)
{
	unsigned int version;
	uint32_t flags;
	uint32_t i, sample_count;
	uint64_t data_offset;
	uint32_t first_sample_flags;
	uint32_t sample_duration, sample_size, sample_flags;
	int64_t sample_composition_time_offset;
	struct mov_track_t* track;
	struct mov_sample_t* sample;

	version = read8BE(); /* version */
	flags = read24BE(); /* flags */
	sample_count = read32BE(); /* sample_count */

	track = _track.get();
	if (track->sample_count + sample_count + 1 > track->sample_offset)
	{
		// void* p = realloc(track->samples, sizeof(struct mov_sample_t) * (track->sample_count + 2*sample_count + 1));
		// if (NULL == p) return ENOMEM;
		// track->samples = (struct mov_sample_t*)p;
		// memset(track->samples + track->sample_count,  0, sizeof(struct mov_sample_t) * (2 * sample_count + 1));
        
        
		track->sample_offset = track->sample_count + 2 * sample_count + 1;
        for (int i = 0; i < track->sample_offset; ++i) {
            auto entry = make_shared<mov_sample_t>();
            track->samples.push_back(entry);
        }
	}

	data_offset = track->tfhd.base_data_offset;
	if (MOV_TRUN_FLAG_DATA_OFFSET_PRESENT & flags)
		data_offset += (int32_t)read32BE(); /* data_offset */

	if (MOV_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT & flags)
		first_sample_flags = read32BE(); /* first_sample_flags */
	else
		first_sample_flags = track->tfhd.flags;

	sample = track->samples[track->sample_count].get();
	for (i = 0; i < sample_count; i++)
	{
		if (MOV_TRUN_FLAG_SAMPLE_DURATION_PRESENT & flags)
			sample_duration = read32BE(); /* sample_duration*/
		else
			sample_duration = track->tfhd.default_sample_duration;

		if (MOV_TRUN_FLAG_SAMPLE_SIZE_PRESENT & flags)
			sample_size = read32BE(); /* sample_size*/
		else
			sample_size = track->tfhd.default_sample_size;

		if (MOV_TRUN_FLAG_SAMPLE_FLAGS_PRESENT & flags)
			sample_flags = read32BE(); /* sample_flags*/
		else
			sample_flags = i ? track->tfhd.default_sample_flags : first_sample_flags;

		if (MOV_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT & flags)
		{
			sample_composition_time_offset = read32BE(); /* sample_composition_time_offset*/
			if (1 == version)
				sample_composition_time_offset = (int32_t)sample_composition_time_offset;
		}
		else
			sample_composition_time_offset = 0;

		sample[i].offset = data_offset;
		sample[i].bytes = sample_size;
		sample[i].dts = track->tfdt_dts;
		sample[i].pts = sample[i].dts + sample_composition_time_offset;
		sample[i].flags = (sample_flags & (MOV_TREX_FLAG_SAMPLE_IS_NO_SYNC_SAMPLE | 0x01000000)) ? 0 : MOV_AV_FLAG_KEYFREAME;
		sample[i].sample_description_index = track->tfhd.sample_description_index;

		data_offset += sample_size;
		track->tfdt_dts += sample_duration;
	}
	track->sample_count += sample_count;
    _implicit_offset = data_offset;

	(void)box;
	return 0;
}

int MP4Demuxer::mov_read_vpcc(const struct mov_box_t* box)
{
    struct mov_track_t* track = _track.get();
    struct mov_sample_entry_t* entry = track->stsd.current.get();
    if(box->size < 4)
        return -1;
    if (entry->extra_data_size < box->size-4)
    {
        // void* p = realloc(entry->extra_data, (size_t)box->size-4);
        // if (NULL == p) return ENOMEM;
        // entry->extra_data = p;

        entry->extra_data.resize(box->size-4);
    }

    read8BE(); /* version */
    read24BE(); /* flags */
    read((char*)entry->extra_data.data(), box->size-4);
    entry->extra_data_size = (int)box->size - 4;
    return 0;
}

void MP4Demuxer::mov_apply_stco(struct mov_track_t* track)
{
    uint32_t i, j, k;
    uint64_t n, chunk_offset;
    struct mov_stbl_t* stbl = &track->stbl;

    // sample offset
    assert(stbl->stsc_count > 0 && stbl->stco_count > 0);
    stbl->stsc[stbl->stsc_count]->first_chunk = stbl->stco_count + 1; // fill stco count
    for (i = 0, n = 0; i < stbl->stsc_count; i++)
    {
        assert(stbl->stsc[i]->first_chunk <= stbl->stco_count);
        for (j = stbl->stsc[i]->first_chunk; j < stbl->stsc[i + 1]->first_chunk; j++)
        {
            chunk_offset = stbl->stco[j - 1]; // chunk start from 1
            for (k = 0; k < stbl->stsc[i]->samples_per_chunk; k++, n++)
            {
                track->samples[n]->sample_description_index = stbl->stsc[i]->sample_description_index;
                track->samples[n]->offset = chunk_offset;
                track->samples[n]->data = NULL;
                chunk_offset += track->samples[n]->bytes;
                assert(track->samples[n]->bytes > 0);
                assert(0 == n || track->samples[n - 1]->offset + track->samples[n - 1]->bytes <= track->samples[n]->offset);
            }
        }
    }

    assert(n == track->sample_count);
}

void MP4Demuxer::mov_apply_elst(struct mov_track_t *track)
{
    size_t i;

    // edit list
    track->samples[0]->dts = 0;
    track->samples[0]->pts = 0;
    for (i = 0; i < track->elst_count; i++)
    {
        if (-1 == track->elst[i]->media_time)
        {
            track->samples[0]->dts = track->elst[i]->segment_duration;
            track->samples[0]->pts = track->samples[0]->dts;
        }
    }
}

void MP4Demuxer::mov_apply_elst_tfdt(struct mov_track_t *track)
{
    size_t i;

    for (i = 0; i < track->elst_count; i++)
    {
        if (-1 == track->elst[i]->media_time)
        {
            track->tfdt_dts += track->elst[i]->segment_duration;
        }
    }
}

void MP4Demuxer::mov_apply_stts(struct mov_track_t* track)
{
    size_t i, j, n;
    struct mov_stbl_t* stbl = &track->stbl;

    for (i = 0, n = 1; i < stbl->stts_count; i++)
    {
        for (j = 0; j < stbl->stts[i]->sample_count; j++, n++)
        {
            track->samples[n]->dts = track->samples[n - 1]->dts + stbl->stts[i]->sample_delta;
            track->samples[n]->pts = track->samples[n]->dts;
        }
    }
    assert(n - 1 == track->sample_count); // see more mov_read_stsz
}

void MP4Demuxer::mov_apply_ctts(struct mov_track_t* track)
{
    size_t i, j, n;
    int32_t delta, dts_shift;
    struct mov_stbl_t* stbl = &track->stbl;

    // make sure pts >= dts
    dts_shift = 0;
    for (i = 0; i < stbl->ctts_count; i++)
    {
        delta = (int32_t)stbl->ctts[i]->sample_delta;
        if (delta < 0 && dts_shift > delta && delta != -1 /* see more cslg box*/)
            dts_shift = delta;
    }
    assert(dts_shift <= 0);

    // sample cts/pts
    for (i = 0, n = 0; i < stbl->ctts_count; i++)
    {
        for (j = 0; j < stbl->ctts[i]->sample_count; j++, n++)
            track->samples[n]->pts += (int64_t)((int32_t)stbl->ctts[i]->sample_delta - dts_shift); // always as int, fixed mp4box delta version error
    }
    assert(0 == stbl->ctts_count || n == track->sample_count);
}

void MP4Demuxer::mov_apply_stss(struct mov_track_t* track)
{
	size_t i, j;
	struct mov_stbl_t* stbl = &track->stbl;

	for (i = 0; i < stbl->stss_count; i++)
	{
		j = stbl->stss[i]; // start from 1
		if (j > 0 && j <= track->sample_count)
			track->samples[j - 1]->flags |= MOV_AV_FLAG_KEYFREAME;
	}
}

struct mov_track_t* MP4Demuxer::mov_find_track(uint32_t track)
{
    int i;
    for (i = 0; i < _track_count; i++)
    {
        if (_tracks[i]->tkhd.track_ID == track)
            return _tracks[i].get();
    }
    return NULL;
}

struct mov_track_t* MP4Demuxer::mov_fetch_track(uint32_t track)
{
    struct mov_track_t* t;
    t = mov_find_track(track);
    if (NULL == t)
    {
        t = mov_add_track();
        if (NULL != t)
        {
            ++_track_count;
            t->tkhd.track_ID = track;
        }
    }
    return t;
}

struct mov_track_t* MP4Demuxer::mov_add_track()
{
    auto track = make_shared<mov_track_t>();
    _tracks.push_back(track);
    track->start_dts = INT64_MIN;
    track->last_dts = INT64_MIN;

    track->stsd.current = make_shared<mov_sample_entry_t>();
    track->stsd.entries.push_back(track->stsd.current);
    track->stsd.entry_count = track->stsd.entries.size();

    return track.get();
}

int MP4Demuxer::mov_read_tx3g(const struct mov_box_t* box)
{
	struct mov_track_t* track = _track.get();
	struct mov_sample_entry_t* entry = track->stsd.current.get();
	if (entry->extra_data_size < box->size)
	{
		// void* p = realloc(entry->extra_data, (size_t)box->size);
		// if (NULL == p) return ENOMEM;
		// entry->extra_data = p;

        entry->extra_data.resize(box->size);
	}

	read((char*)entry->extra_data.data(), box->size);
	entry->extra_data_size = (int)box->size;
	return 0;
}