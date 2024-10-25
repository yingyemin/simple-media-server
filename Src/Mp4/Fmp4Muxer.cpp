#include <cstdlib>
#include <string>
#include <algorithm>
#include <iostream>
#include <cctype>

#include "Fmp4Muxer.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

Fmp4Muxer::Fmp4Muxer(int flags)
    :MP4Muxer(0)
	,_flags(flags)
{
	_timeClock.start();
	_segmentBuffer = make_shared<StringBuffer>();
}

Fmp4Muxer::~Fmp4Muxer()
{}

void Fmp4Muxer::write(const char* data, int size)
{
    if (_segmentOffset + size > _segmentBuffer->size()) {
        //需要扩容
        _segmentBuffer->resize(_segmentOffset + size);
    }
    memcpy((uint8_t *) _segmentBuffer->data() + _segmentOffset, data, size);
    _segmentOffset += size;
}

void Fmp4Muxer::read(char* data, int size)
{
    data = _segmentBuffer->data() + _segmentOffset;
}

void Fmp4Muxer::seek(uint64_t offset)
{
    _segmentOffset = offset;
}

size_t Fmp4Muxer::tell()
{
    return _segmentOffset;
}

void Fmp4Muxer::setOnFmp4Header(const function<void(const Buffer::Ptr& buffer)>& cb)
{
	_onFmp4Header = cb;
}

void Fmp4Muxer::setOnFmp4Segment(const function<void(const Buffer::Ptr& buffer, bool keyframe)>& cb)
{
	_onFmp4Segment = cb;
}

Buffer::Ptr Fmp4Muxer::getFmp4Header()
{
	auto tmp = _segmentBuffer;
	_segmentOffset = 0;
	_segmentBuffer = make_shared<StringBuffer>();

	return tmp;
}

void Fmp4Muxer::startEncode()
{
    _startEncode = true;
	_timeClock.update();
}

void Fmp4Muxer::stopEncode()
{
    _startEncode = false;
	fmp4_writer_save_segment();
	if (_onFmp4Segment) {
		_onFmp4Segment(_segmentBuffer, _keyframe);
	}
	
	_keyframe = false;
	_segmentOffset = 0;
	_segmentBuffer = make_shared<StringBuffer>();
}

int Fmp4Muxer::inputFrame(int trackIndex, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags)
{
    return inputFrame_l(trackIndex, data, bytes, pts, dts, flags, 0);
}

int Fmp4Muxer::inputFrame_l(int trackIndex, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags, int add_nalu_size)
{
	// logTrace << "Fmp4Muxer::inputFrame_l ============= ";
	if (true || flags || _timeClock.startToNow() > 50) {
		fmp4_writer_save_segment();
		_timeClock.update();
		if (_onFmp4Segment) {
			_onFmp4Segment(_segmentBuffer, _keyframe);
		}
		_keyframe = false;
		_segmentBuffer = make_shared<StringBuffer>();
		_segmentOffset = 0;
	}

	if (flags) {
		_keyframe = true;
	}
	
    int64_t duration;
	struct mov_track_t* track;
	struct mov_sample_t* sample;

	if(add_nalu_size){
        bytes += 4;
	}

	if (trackIndex < 0 || _mapTrackInfo.find(trackIndex) == _mapTrackInfo.end())
        return -ENOMEM;

	track = _tracks[_mapTrackInfo[trackIndex]].get();

    duration = dts > track->last_dts && INT64_MIN != track->last_dts ? dts - track->last_dts : 0;
#if 1
    track->turn_last_duration = duration;
#else
    track->turn_last_duration = track->turn_last_duration > 0 ? track->turn_last_duration * 7 / 8 + duration / 8 : duration;
#endif
    
	if (MOV_VIDEO == track->handler_type && (flags & MOV_AV_FLAG_KEYFREAME) )
		fmp4_write_fragment(); // fragment per video keyframe

	// if (track->sample_count + 1 >= track->sample_offset)
	// {
	// 	void* ptr = realloc(track->samples, sizeof(struct mov_sample_t) * (track->sample_offset + 1024));
	// 	if (NULL == ptr) return -ENOMEM;
	// 	track->samples = (struct mov_sample_t*)ptr;
	// 	track->sample_offset += 1024;
	// }

	static int64_t firstDts = -1;
	static int64_t firstPts = -1;

	if (firstDts == -1) {
		firstDts = dts;
	}

	if (firstPts == -1) {
		firstPts = pts;
	}

	pts = (pts - firstPts) * track->mdhd.timescale / 1000;
	dts = (dts - firstDts) * track->mdhd.timescale / 1000;

	logInfo << "pts ============= " << pts;
	logInfo << "dts ============= " << dts;

    auto samplePtr = make_shared<mov_sample_t>();
    track->samples.push_back(samplePtr);

	sample = samplePtr.get();
	sample->sample_description_index = 1;
	sample->bytes = (uint32_t)bytes;
	sample->flags = flags;
	sample->pts = pts;
	sample->dts = dts;
	sample->offset = _mdatSize;

	cout << "add track sample: " << _mapTrackInfo[trackIndex] << endl;
	cout << "track->sample_count: " << track->sample_count + 1 << endl;
	cout << "track->sample size: " << track->samples.size() << endl;
	sample->data = malloc(bytes);
	if (NULL == sample->data)
		return -ENOMEM;

    if (!add_nalu_size) {
        memcpy(sample->data, data, bytes);
    } else {
        uint8_t nalu_size_buf[4] = {0};
        size_t nalu_size = bytes - 4;
        nalu_size_buf[0] = (uint8_t) ((nalu_size >> 24) & 0xFF);
        nalu_size_buf[1] = (uint8_t) ((nalu_size >> 16) & 0xFF);
        nalu_size_buf[2] = (uint8_t) ((nalu_size >> 8) & 0xFF);
        nalu_size_buf[3] = (uint8_t) ((nalu_size >> 0) & 0xFF);
        memcpy(sample->data, nalu_size_buf, 4);
        memcpy((char *)sample->data + 4, data, nalu_size);
    }

    if (INT64_MIN == track->start_dts)
        track->start_dts = sample->dts;
	_mdatSize += bytes; // update media data size
	track->sample_count += 1;
    track->last_dts = sample->dts;
	return 0;
}

void Fmp4Muxer::addAudioTrack(const shared_ptr<TrackInfo>& trackInfo)
{
    // struct mov_t* mov;
    // struct mov_track_t* track;

    // // mov = &writer->mov;
    // track = mov_add_track();
    // if (NULL == track)
    //     return -ENOMEM;

    // if (0 != mov_add_audio(track, &mov->mvhd, 1000, object, channel_count, bits_per_sample, sample_rate, extra_data, extra_data_size))
    //     return -ENOMEM;

    // mov->mvhd.next_track_ID++;
    // return mov->track_count++;

    auto track = make_shared<mov_track_t>();
    track->start_dts = INT64_MIN;
    track->last_dts = INT64_MIN;

    auto audio = track->stsd.current = make_shared<mov_sample_entry_t>();
    track->stsd.entries.push_back(audio);

    audio->data_reference_index = 1;
    audio->object_type_indication = getAudioObject(trackInfo->codec_);
    audio->stream_type = MP4_STREAM_AUDIO;
    audio->u.audio.channelcount = trackInfo->channel_;
    audio->u.audio.samplesize = (uint16_t)trackInfo->bitPerSample_;
    audio->u.audio.samplerate = (trackInfo->samplerate_ > 56635 ? 0 : trackInfo->samplerate_) << 16;

    track->tag = mov_object_to_tag(audio->object_type_indication);
    track->handler_type = MOV_AUDIO;
    track->handler_descr = "SoundHandler";
    track->stsd.entry_count = 1;
    track->offset = 0;

    track->tkhd.flags = MOV_TKHD_FLAG_TRACK_ENABLE | MOV_TKHD_FLAG_TRACK_IN_MOVIE;
    track->tkhd.track_ID = _mvhd.next_track_ID;
    track->tkhd.creation_time = _mvhd.creation_time;
    track->tkhd.modification_time = _mvhd.modification_time;
    track->tkhd.width = 0;
    track->tkhd.height = 0;
    track->tkhd.volume = 0x0100;
    track->tkhd.duration = 0; // placeholder

    track->mdhd.creation_time = track->tkhd.creation_time;
    track->mdhd.modification_time = track->tkhd.modification_time;
    track->mdhd.timescale = 1000; //sample_rate
    track->mdhd.language = 0x55c4;
    track->mdhd.duration = 0; // placeholder

    audio->extra_data = trackInfo->getConfig();
    // if (audio->extra_data.empty())
    //     return ;
    // memcpy(audio->extra_data, extra_data, extra_data_size);
	audio->extra_data_size = audio->extra_data.size();

    _mvhd.next_track_ID++;
    _tracks.push_back(track);
	_mapTrackInfo[trackInfo->index_] = _trackCount;
    _trackCount++;
    _track = track;
}

void Fmp4Muxer::addVideoTrack(const shared_ptr<TrackInfo>& trackInfo)
{
    auto track = make_shared<mov_track_t>();
    track->start_dts = INT64_MIN;
    track->last_dts = INT64_MIN;

    auto video = track->stsd.current = make_shared<mov_sample_entry_t>();
    track->stsd.entries.push_back(video);

	int width, height, fps;
	trackInfo->getWidthAndHeight(width, height, fps);

    video->data_reference_index = 1;
    video->object_type_indication = getVideoObject(trackInfo->codec_);;
    video->stream_type = MP4_STREAM_VISUAL;
    video->u.visual.width = width;
    video->u.visual.height = height;
    video->u.visual.depth = 0x0018;
    video->u.visual.frame_count = 1;
    video->u.visual.horizresolution = 0x00480000;
    video->u.visual.vertresolution = 0x00480000;

    track->tag = mov_object_to_tag(video->object_type_indication);
    track->handler_type = MOV_VIDEO;
    track->handler_descr = "VideoHandler";
    track->stsd.entry_count = 1;
    track->offset = 0;

    track->tkhd.flags = MOV_TKHD_FLAG_TRACK_ENABLE | MOV_TKHD_FLAG_TRACK_IN_MOVIE;
    track->tkhd.track_ID = _mvhd.next_track_ID;
    track->tkhd.creation_time = _mvhd.creation_time;
    track->tkhd.modification_time = _mvhd.modification_time;
    track->tkhd.width = width << 16;
    track->tkhd.height = height << 16;
    track->tkhd.volume = 0;
    track->tkhd.duration = 0; // placeholder

    track->mdhd.creation_time = track->tkhd.creation_time;
    track->mdhd.modification_time = track->tkhd.modification_time;
    track->mdhd.timescale = 1000; //mov->mvhd.timescale
    track->mdhd.language = 0x55c4;
    track->mdhd.duration = 0; // placeholder

	video->extra_data = trackInfo->getConfig();
    // if (video->extra_data.empty())
    //     return ;
    // memcpy(video->extra_data, extra_data, extra_data_size);
	video->extra_data_size = video->extra_data.size();

    _mvhd.next_track_ID++;
    _tracks.push_back(track);
	_mapTrackInfo[trackInfo->index_] = _trackCount;
    _trackCount++;
    _track = track;
}

size_t Fmp4Muxer::fmp4_write_mvex()
{
	int i;
	size_t size;
	uint64_t offset;

	size = 8 /* Box */;
	offset = tell();
	write32BE(0); /* size */
	write("mvex", 4);

	//size += fmp4_write_mehd(mov);
	for (i = 0; i < _trackCount; i++)
	{
		_track = _tracks[i];
		size += mov_write_trex();
	}
	//size += mov_write_leva(mov);

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t Fmp4Muxer::mov_write_trex()
{
	write32BE(12 + 20); /* size */
	write("trex", 4);
	write32BE(0); /* version & flags */
	write32BE(_track->tkhd.track_ID); /* track_ID */
	write32BE(1); /* default_sample_description_index */
	write32BE(0); /* default_sample_duration */
	write32BE(0); /* default_sample_size */
	write32BE(0); /* default_sample_flags */
	return 32;
}

size_t Fmp4Muxer::fmp4_write_traf(uint32_t moof)
{
	uint32_t i, start;
	size_t size;
	uint64_t offset;
    struct mov_track_t* track;

	size = 8 /* Box */;
	offset = tell();
	write32BE(0); /* size */
	write("traf", 4);

    track = _track.get();
	track->tfhd.flags = MOV_TFHD_FLAG_DEFAULT_FLAGS /*| MOV_TFHD_FLAG_BASE_DATA_OFFSET*/;
    track->tfhd.flags |= MOV_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX;
    // ISO/IEC 23009-1:2014(E) 6.3.4.2 General format type (p93)
	// The 'moof' boxes shall use movie-fragment relative addressing for media data that 
	// does not use external data references, the flag 'default-base-is-moof' shall be set, 
	// and data-offset shall be used, i.e. base-data-offset-present shall not be used.
	//if (mov->flags & MOV_FLAG_SEGMENT)
	{
		//track->tfhd.flags &= ~MOV_TFHD_FLAG_BASE_DATA_OFFSET;
		track->tfhd.flags |= MOV_TFHD_FLAG_DEFAULT_BASE_IS_MOOF;
	}
	track->tfhd.base_data_offset = _moofOffset;
    track->tfhd.sample_description_index = 1;
	track->tfhd.default_sample_flags = MOV_AUDIO == track->handler_type ? MOV_TREX_FLAG_SAMPLE_DEPENDS_ON_I_PICTURE : (MOV_TREX_FLAG_SAMPLE_IS_NO_SYNC_SAMPLE| MOV_TREX_FLAG_SAMPLE_DEPENDS_ON_NOT_I_PICTURE);
    if (track->sample_count > 0)
    {
        track->tfhd.flags |= MOV_TFHD_FLAG_DEFAULT_DURATION | MOV_TFHD_FLAG_DEFAULT_SIZE;
        track->tfhd.default_sample_duration = track->sample_count > 1 ? (uint32_t)(track->samples[1]->dts - track->samples[0]->dts) : (uint32_t)track->turn_last_duration;
        track->tfhd.default_sample_size = track->samples[0]->bytes;
    }
    else
    {
        track->tfhd.flags |= MOV_TFHD_FLAG_DURATION_IS_EMPTY;
        track->tfhd.default_sample_duration = 0; // not set
        track->tfhd.default_sample_size = 0; // not set
    }

	size += mov_write_tfhd();
	// ISO/IEC 23009-1:2014(E) 6.3.4.2 General format type (p93)
	// Each 'traf' box shall contain a 'tfdt' box.
    size += mov_write_tfdt();

	for (start = 0, i = 1; i < track->sample_count; i++)
	{
        if (track->samples[i - 1]->offset + track->samples[i - 1]->bytes != track->samples[i]->offset)
        {
            size += mov_write_trun(start, i-start, moof);
            start = i;
        }
	}
    size += mov_write_trun(start, i-start, moof);

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t Fmp4Muxer::mov_write_trun(uint32_t from, uint32_t count, uint32_t moof)
{
    uint32_t flags;
	uint32_t delta;
	uint64_t offset;
	uint32_t size, i;
	const struct mov_sample_t* sample;
	const struct mov_track_t* track = _track.get();

    if (count < 1) return 0;
    assert(from + count <= track->sample_count);
    flags = MOV_TRUN_FLAG_DATA_OFFSET_PRESENT;
    if (track->samples[from]->flags & MOV_AV_FLAG_KEYFREAME)
        flags |= MOV_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT;

    for (i = from; i < from + count; i++)
    {
        sample = track->samples[i].get();
        if (sample->bytes != track->tfhd.default_sample_size)
            flags |= MOV_TRUN_FLAG_SAMPLE_SIZE_PRESENT;
        if ((uint32_t)(i + 1 < track->sample_count ? track->samples[i + 1]->dts - track->samples[i]->dts : track->turn_last_duration) != track->tfhd.default_sample_duration)
            flags |= MOV_TRUN_FLAG_SAMPLE_DURATION_PRESENT;
        if (sample->pts != sample->dts)
            flags |= MOV_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT;
    }

	size = 12/* full box */ + 4/* sample count */;

	offset = tell();
	write32BE(0); /* size */
	write("trun", 4);
	write8BE(1); /* version */
	write24BE(flags); /* flags */
	write32BE(count); /* sample_count */

    assert(flags & MOV_TRUN_FLAG_DATA_OFFSET_PRESENT);
	if (flags & MOV_TRUN_FLAG_DATA_OFFSET_PRESENT)
	{
		write32BE(moof + (uint32_t)track->samples[from]->offset);
		size += 4;
	}

	if (flags & MOV_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT)
	{
		write32BE(MOV_TREX_FLAG_SAMPLE_DEPENDS_ON_I_PICTURE); /* first_sample_flags */
		size += 4;
	}

	assert(from + count <= track->sample_count);
	for (i = from; i < from + count; i++)
	{
		sample = track->samples[i].get();
		if (flags & MOV_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
		{
            delta = (uint32_t)(i + 1 < track->sample_count ? track->samples[i + 1]->dts - track->samples[i]->dts : track->turn_last_duration);
			logInfo << "delta: " << delta;
			write32BE(delta); /* sample_duration */
			size += 4;
		}

		if (flags & MOV_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
		{
			write32BE((uint32_t)sample->bytes); /* sample_size */
			size += 4;
		}

		assert(0 == (flags & MOV_TRUN_FLAG_SAMPLE_FLAGS_PRESENT));
//		write32BE(0); /* sample_flags */

		if (flags & MOV_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT)
		{
			write32BE((int32_t)(sample->pts - sample->dts)); /* sample_composition_time_offset */
			size += 4;
		}
	}

	writeMdatSize(offset, size);
	return size;
}

size_t Fmp4Muxer::mov_write_tfhd()
{
	size_t size;
	uint64_t offset;

	size = 12 + 4 /* track_ID */;

	offset = tell();
	write32BE(0); /* size */
	write("tfhd", 4);
	write8BE(0); /* version */
	write24BE(_track->tfhd.flags); /* flags */
	write32BE(_track->tkhd.track_ID); /* track_ID */

	if (MOV_TFHD_FLAG_BASE_DATA_OFFSET & _track->tfhd.flags)
	{
		write64BE(_track->tfhd.base_data_offset); /* base_data_offset*/
		size += 8;
	}

	if (MOV_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX & _track->tfhd.flags)
	{
		write32BE(_track->stsd.entries[0]->data_reference_index); /* sample_description_index*/
		size += 4;
	}

	if (MOV_TFHD_FLAG_DEFAULT_DURATION & _track->tfhd.flags)
	{
		write32BE(_track->tfhd.default_sample_duration); /* default_sample_duration*/
		size += 4;
	}

	if (MOV_TFHD_FLAG_DEFAULT_SIZE & _track->tfhd.flags)
	{
		write32BE(_track->tfhd.default_sample_size); /* default_sample_size*/
		size += 4;
	}

	if (MOV_TFHD_FLAG_DEFAULT_FLAGS & _track->tfhd.flags)
	{
		write32BE(_track->tfhd.default_sample_flags); /* default_sample_flags*/
		size += 4;
	}

	writeMdatSize(offset, size);
	return size;
}

size_t Fmp4Muxer::mov_write_tfdt()
{
    uint8_t version;
    uint64_t baseMediaDecodeTime;

    if (_track->sample_count < 1)
        return 0;

    baseMediaDecodeTime = _track->samples[0]->dts - _track->start_dts;
    version = baseMediaDecodeTime > INT32_MAX ? 1 : 0;

	logInfo << "baseMediaDecodeTime: " << baseMediaDecodeTime;

    write32BE(0 == version ? 16 : 20); /* size */
    write("tfdt", 4);
    write8BE(version); /* version */
    write24BE(0); /* flags */

    if (1 == version)
        write64BE(baseMediaDecodeTime); /* baseMediaDecodeTime */
    else
        write32BE((uint32_t)baseMediaDecodeTime); /* baseMediaDecodeTime */

    return 0 == version ? 16 : 20;
}

size_t Fmp4Muxer::fmp4_write_moof(uint32_t fragment, uint32_t moof)
{
	int i;
	size_t size, j;
	uint64_t offset;
	uint64_t n;

	size = 8 /* Box */;
	offset = tell();
	write32BE(0); /* size */
	write("moof", 4);

	size += mov_write_mfhd(fragment);

	n = 0;
	for (i = 0; i < _trackCount; i++)
	{
		_track = _tracks[i];

		// rewrite offset, write only one trun
		// 2017/10/17 Dale Curtis SHA-1: a5fd8aa45b11c10613e6e576033a6b5a16b9cbb9 (libavformat/mov.c)
		for (j = 0; j < _track->sample_count; j++)
		{
			_track->samples[j]->offset = n;
			n += _track->samples[j]->bytes;
		}

		if (_track->sample_count > 0)
			size += fmp4_write_traf(moof);
	}

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t Fmp4Muxer::mov_write_mfhd(uint32_t fragment)
{
    write32BE(16); /* size */
    write("mfhd", 4);
    write32BE(0); /* version & flags */
    write32BE(fragment); /* sequence_number */
    return 16;
}

size_t Fmp4Muxer::fmp4_write_moov()
{
	int i;
	size_t size;
	uint32_t count;
	uint64_t offset;

	size = 8 /* Box */;
	offset = tell();
	write32BE(0); /* size */
    write("moov", 4);

	size += mov_write_mvhd();
//	size += fmp4_write_iods(mov);
	for (i = 0; i < _trackCount; i++)
	{
		_track = _tracks[i];
		count = _track->sample_count;
		_track->sample_count = 0;
		size += mov_write_trak();
		_track->sample_count = count; // restore sample count
	}

	size += fmp4_write_mvex();
	size += mov_write_udta();
	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t Fmp4Muxer::fmp4_write_sidx()
{
	int i;
	for (i = 0; i < _trackCount; i++)
	{
		_track = _tracks[i];
        mov_write_sidx(52 * (uint64_t)(_trackCount - i - 1)); /* first_offset */
	}

	return 52 * _trackCount;
}

size_t Fmp4Muxer::mov_write_sidx(uint64_t offset)
{
    uint32_t duration = 0;
    uint64_t earliest_presentation_time = 0;
    const struct mov_track_t* track = _track.get();

    if (track->sample_count > 0)
    {
        earliest_presentation_time = track->samples[0]->pts;
        duration = (uint32_t)(track->samples[track->sample_count - 1]->dts - track->samples[0]->dts) + (uint32_t)track->turn_last_duration;
    }
    else
    {
        duration = 0;
        earliest_presentation_time = 0;
    }

	logInfo << "earliest_presentation_time == " << earliest_presentation_time;
	logInfo << "duration == " << duration;

    write32BE(52); /* size */
    write("sidx", 4);
    write8BE(1); /* version */
    write24BE(0); /* flags */

    write32BE(track->tkhd.track_ID); /* reference_ID */
    write32BE(track->mdhd.timescale); /* timescale */
    write64BE(earliest_presentation_time); /* earliest_presentation_time */
    write64BE(offset); /* first_offset */
    write16BE(0); /* reserved */
    write16BE(1); /* reference_count */

    write32BE(0); /* reference_type & referenced_size */
    write32BE(duration); /* subsegment_duration */
    write32BE((1U/*starts_with_SAP*/ << 31) | (1 /*SAP_type*/ << 28) | 0 /*SAP_delta_time*/);
    
    return 52;
}

int Fmp4Muxer::fmp4_write_mfra()
{
	int i;
	uint64_t mfra_offset;
	uint64_t mfro_offset;

	// mfra
	mfra_offset = tell();
	write32BE(0); /* size */
    write("mfra", 4);

	// tfra
	for (i = 0; i < _trackCount; i++)
	{
		_track = _tracks[i];
		mov_write_tfra();
	}

	// mfro
	mfro_offset = tell();
	write32BE(16); /* size */
	write("mfro", 4);
	write32BE(0); /* version & flags */
	write32BE((uint32_t)(mfro_offset - mfra_offset + 16));

	writeMdatSize(mfra_offset, (size_t)(mfro_offset - mfra_offset + 16));
	return (int)(mfro_offset - mfra_offset + 16);
}

size_t Fmp4Muxer::mov_write_tfra()
{
	uint32_t i, size;
	const struct mov_track_t* track = _track.get();

	size = 12/* full box */ + 12/* base */ + track->frag_count * 19/* index */;

	write32BE(size); /* size */
	write("tfra", 4);
	write8BE(1); /* version */
	write24BE(0); /* flags */

	write32BE(track->tkhd.track_ID); /* track_ID */
	write32BE(0); /* length_size_of_traf_num/trun/sample */
	write32BE(track->frag_count); /* number_of_entry */

	for (i = 0; i < track->frag_count; i++)
	{
		logInfo << "track->frags[i]->time: " << track->frags[i]->time;
		write64BE(track->frags[i]->time);
		write64BE(track->frags[i]->offset); /* moof_offset */
		write8BE(1); /* traf number */
		write8BE(1); /* trun number */
		write8BE(1); /* sample number */
	}

	return size;
}

int Fmp4Muxer::fmp4_add_fragment_entry(struct mov_track_t* track, uint64_t time, uint64_t offset)
{
	// if (track->frag_count >= track->frag_capacity)
	// {
	// 	// void* p = realloc(track->frags, sizeof(struct mov_fragment_t) * (track->frag_capacity + 64));
	// 	// if (!p) return ENOMEM;
    //     auto p = make_shared<mov_fragment_t>();
	// 	track->frags.push_back(p);
	// 	track->frag_capacity += 64;
	// }

	auto p = make_shared<mov_fragment_t>();
	p->time = time;
	p->offset = offset;
	track->frags.push_back(p);

	// track->frags[track->frag_count]->time = time;
	// track->frags[track->frag_count]->offset = offset;
	++track->frag_count;
	return 0;
}

int Fmp4Muxer::fmp4_write_fragment()
{
	int i;
	size_t n;
	size_t refsize;
	struct mov_t* mov;

	if (_mdatSize < 1)
		return 0; // empty

	// write moov
	if (!_hasMoov)
	{
		// write ftyp/stype
		if (_flags & MOV_FLAG_SEGMENT)
		{
			mov_write_styp();
		}
		else
		{
			write_ftyp();
			fmp4_write_moov();
		}

		_hasMoov = 1;
	}

	if (_flags & MOV_FLAG_SEGMENT)
	{
		// ISO/IEC 23009-1:2014(E) 6.3.4.2 General format type (p93)
		// Each Media Segment may contain one or more 'sidx' boxes. 
		// If present, the first 'sidx' box shall be placed before any 'moof' box 
		// and the first Segment Index box shall document the entire Segment.
		fmp4_write_sidx();
	}

	// moof
	_moofOffset = tell();
	refsize = fmp4_write_moof(++_fragmentId, 0); // start from 1
    // rewrite moof with trun data offset
    seek(_moofOffset);
    fmp4_write_moof(_fragmentId, (uint32_t)refsize+8);
    refsize += _mdatSize + 8/*mdat box*/;

	// add mfra entry
	for (i = 0; i < _trackCount; i++)
	{
		_track = _tracks[i];
		if (_track->sample_count > 0)
			fmp4_add_fragment_entry(_track.get(), _track->samples[0]->dts, _moofOffset);

		// hack: write sidx referenced_size
		if (_flags & MOV_FLAG_SEGMENT)
			writeMdatSize(_moofOffset - 52 * (uint64_t)(_trackCount - i) + 40, (0 << 31) | (refsize & 0x7fffffff));

		cout << "_track->offset = 0" << endl;
		_track->offset = 0; // reset
	}

	// mdat
    if (_mdatSize + 8 <= UINT32_MAX)
    {
        write32BE((uint32_t)_mdatSize + 8); /* size */
        write("mdat", 4);
    }
    else
    {
        write32BE(1);
        write("mdat", 4);
        write64BE(_mdatSize + 16);
    }

	// interleave write samples
    n = 0;
	while(n < _mdatSize)
	{
		for (i = 0; i < _trackCount; i++)
		{
			_track = _tracks[i];
			while (_track->offset < _track->sample_count && n == _track->samples[_track->offset]->offset)
            {
				cout << "_track i: " << i << endl;
				cout << "_track->offset: " << _track->offset << endl;
				cout << "_track->sample_count: " << _track->sample_count << endl;
                write((char*)_track->samples[_track->offset]->data, _track->samples[_track->offset]->bytes);
                free(_track->samples[_track->offset]->data); // free av packet memory
                n += _track->samples[_track->offset]->bytes;
                ++_track->offset;
            }
		}
	}

    // clear track samples(don't free samples memory)
	for (i = 0; i < _trackCount; i++)
	{
		_tracks[i]->sample_count = 0;
		_tracks[i]->offset = 0;
		_tracks[i]->samples.clear();
	}
	_mdatSize = 0;

	return 0;
}

size_t Fmp4Muxer::mov_write_styp()
{
	int size, i;

	size = 8/* box */ + 8/* item */ + _ftyp.brands_count * 4 /* compatible brands */;

	write32BE(size); /* size */
	write("styp", 4);
	write32BE(_ftyp.major_brand);
	write32BE(_ftyp.minor_version);

	for (i = 0; i < _ftyp.brands_count; i++)
		write32BE(_ftyp.compatible_brands[i]);

	return size;
}

int Fmp4Muxer::fmp4_writer_init()
{
	if (_flags & MOV_FLAG_SEGMENT)
	{
		_ftyp.major_brand = MOV_BRAND_MSDH;
		_ftyp.minor_version = 0;
		_ftyp.brands_count = 4;
		_ftyp.compatible_brands[0] = MOV_BRAND_ISOM;
		_ftyp.compatible_brands[1] = MOV_BRAND_MP42;
		_ftyp.compatible_brands[2] = MOV_BRAND_MSDH;
		_ftyp.compatible_brands[3] = MOV_BRAND_MSIX;
		_header = 0;
	}
	else
	{
		_ftyp.major_brand = MOV_BRAND_ISOM;
		_ftyp.minor_version = 1;
		_ftyp.brands_count = 4;
		_ftyp.compatible_brands[0] = MOV_BRAND_ISOM;
		_ftyp.compatible_brands[1] = MOV_BRAND_MP42;
		_ftyp.compatible_brands[2] = MOV_BRAND_AVC1;
		_ftyp.compatible_brands[3] = MOV_BRAND_DASH;
		_header = 0;
	}
	return 0;
}

void Fmp4Muxer::init()
{
	// struct mov_t* mov;
	// struct fmp4_writer_t* writer;
	// writer = (struct fmp4_writer_t*)calloc(1, sizeof(struct fmp4_writer_t));
	// if (NULL == writer)
	// 	return NULL;

	_fragInterleave = 5;

	// mov = &writer->mov;
	// _flags = flags;
	_mvhd.next_track_ID = 1;
	_mvhd.creation_time = time(NULL) + 0x7C25B080; // 1970 based -> 1904 based;
	_mvhd.modification_time = _mvhd.creation_time;
	_mvhd.timescale = 1000;
	_mvhd.duration = 0; // placeholder
	fmp4_writer_init();

	// mov->io.param = param;
	// memcpy(&mov->io.io, buffer, sizeof(mov->io.io));
	// return writer;
}

void Fmp4Muxer::fmp4_writer_destroy()
{
	int i;
	// struct mov_t* mov;
	// mov = &writer->mov;

	fmp4_writer_save_segment();

	// for (i = 0; i < _trackCount; i++)
    //     mov_free_track(_tracks[i]);
	// if (mov->tracks)
	// 	free(mov->tracks);
	// free(writer);
}

int Fmp4Muxer::fmp4_writer_add_udta(const void* data, size_t size)
{
	_udta = data;
	_udta_size = size;
	return 0;
}

int Fmp4Muxer::fmp4_writer_save_segment()
{
	// logTrace << "Fmp4Muxer::fmp4_writer_save_segment()";
	int i;
	// struct mov_t* mov;
	// mov = &writer->mov;

	// flush fragment
	fmp4_write_fragment();
	_hasMoov = 0; // clear moov flags

	// write mfra
	if (0 == (_flags & MOV_FLAG_SEGMENT))
	{
		fmp4_write_mfra();
		for (i = 0; i < _trackCount; i++) {
			_tracks[i]->frag_count = 0; // don't free frags memory
			_tracks[i]->frags.clear();
		}
	}

	return 0;
}

int Fmp4Muxer::fmp4_writer_init_segment()
{
	// struct mov_t* mov;
	// mov = &writer->mov;
	write_ftyp();
	fmp4_write_moov();
	return 0;
}
