#include "Mp4Muxer.h"
#include "Util/String.h"
#include "Log/Logger.h"

#include <sys/time.h>

int getAudioObject(const string& codec, int samplerate)
{
    if (codec == "aac") {
        return MOV_OBJECT_AAC;
    } else if (codec == "g711a") {
        return MOV_OBJECT_G711a;
    } else if (codec == "g711u") {
        return MOV_OBJECT_G711u;
    } else if (codec == "mp3") {
		if (samplerate > 24000) {
			return MOV_OBJECT_MP1A;
		}
        
		return MOV_OBJECT_MP3;
    }

    return 0;
}

int getVideoObject(const string& codec)
{
    if (codec == "opus") {
        return MOV_OBJECT_OPUS;
    } else if (codec == "h264") {
        return MOV_OBJECT_H264;
    } else if (codec == "h265") {
        return MOV_OBJECT_HEVC;
    }

    return 0;
}

MP4Muxer::MP4Muxer(int flags)
    :_flags(flags)
{
}

MP4Muxer::~MP4Muxer()
{}

void MP4Muxer::write64BE(uint64_t value)
{
    char strValue[8];
    writeUint32BE(strValue, value >> 32);
    write(strValue, 4);
    writeUint32BE(strValue, value);
    write(strValue, 4);
}

void MP4Muxer::write32BE(uint32_t value)
{
    char strValue[4];
    writeUint32BE(strValue, value);
    write(strValue, 4);
}

void MP4Muxer::write24BE(uint32_t value)
{
    char strValue[3];
    writeUint24BE(strValue, value);
    write(strValue, 3);
}

void MP4Muxer::write16BE(uint16_t value)
{
    char strValue[2];
    writeUint16BE(strValue, value);
    write(strValue, 2);
}

void MP4Muxer::write8BE(uint8_t value)
{
    write((char*)&value, 1);
}

void MP4Muxer::init()
{
    _ftyp.major_brand = MOV_BRAND_ISOM;
	_ftyp.minor_version = 0x200;
	_ftyp.brands_count = 4;
	_ftyp.compatible_brands[0] = MOV_BRAND_ISOM;
	_ftyp.compatible_brands[1] = MOV_BRAND_ISO2;
	_ftyp.compatible_brands[2] = MOV_BRAND_AVC1;
	_ftyp.compatible_brands[3] = MOV_BRAND_MP41;

	_header = 0;

    _mvhd.next_track_ID = 1;
	_mvhd.creation_time = time(NULL) + 0x7C25B080; // 1970 based -> 1904 based;
	_mvhd.modification_time = _mvhd.creation_time;
	_mvhd.timescale = 1000;
	_mvhd.duration = 0; // placeholder

    write_ftyp();

    write32BE(8);

    write("free", 4);

    _mdatOffset = tell();

    // mdat的长度，先占位，赋值为0
    write32BE(0);

    write("mdat", 4);
}

size_t MP4Muxer::write_ftyp()
{
    int size, i;

	size = 8/* box */ + 8/* item */ + _ftyp.brands_count * 4 /* compatible brands */;

	write32BE(size); /* size */
	write("ftyp", 4);
	write32BE(_ftyp.major_brand);
	write32BE(_ftyp.minor_version);

	for (i = 0; i < _ftyp.brands_count; i++)
		write32BE(_ftyp.compatible_brands[i]);

	return size;
}

void MP4Muxer::addAudioTrack(const shared_ptr<TrackInfo>& trackInfo)
{
    auto track = make_shared<mov_track_t>();
    track->start_dts = INT64_MIN;
    track->last_dts = INT64_MIN;

    auto audio = track->stsd.current = make_shared<mov_sample_entry_t>();
    track->stsd.entries.push_back(audio);

    audio->data_reference_index = 1;
    audio->object_type_indication = getAudioObject(trackInfo->codec_, trackInfo->samplerate_);
    audio->stream_type = MP4_STREAM_AUDIO;
    audio->u.audio.channelcount = trackInfo->channel_;
    audio->u.audio.samplesize = (uint16_t)trackInfo->bitPerSample_ * (uint16_t)trackInfo->channel_;
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
	_mapTrackInfo[trackInfo->index_] = _track_count;
    _track_count++;
    _track = track;
}

void MP4Muxer::addVideoTrack(const shared_ptr<TrackInfo>& trackInfo)
{
    auto track = make_shared<mov_track_t>();
    track->start_dts = INT64_MIN;
    track->last_dts = INT64_MIN;

    auto video = track->stsd.current = make_shared<mov_sample_entry_t>();
    track->stsd.entries.push_back(video);

    video->data_reference_index = 1;
    video->object_type_indication = getVideoObject(trackInfo->codec_);;
    video->stream_type = MP4_STREAM_VISUAL;
    video->u.visual.width = trackInfo->_width;
    video->u.visual.height = trackInfo->_height;
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
    track->tkhd.width = trackInfo->_width << 16;
    track->tkhd.height = trackInfo->_height << 16;
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
	_mapTrackInfo[trackInfo->index_] = _track_count;
    _track_count++;
    _track = track;
}

void MP4Muxer::inputFrame(const FrameBuffer::Ptr& frame, int trackIndex, bool keyframe)
{
    if (trackIndex < 0 || _mapTrackInfo.find(trackIndex) == _mapTrackInfo.end())
        return ;

    _track = _tracks[_mapTrackInfo[trackIndex]];

    // if (_track->sample_count + 1 >= _track->sample_offset) {
    //     void *ptr = realloc(mov->track->samples, sizeof(struct mov_sample_t) * (mov->track->sample_offset + 1024));
    //     if (NULL == ptr) return -ENOMEM;
    //     mov->track->samples = ptr;
    //     mov->track->sample_offset += 1024;
    // }

    auto pts = frame->_pts * _track->mdhd.timescale / 1000;
    auto dts = frame->_dts * _track->mdhd.timescale / 1000;

    auto sample = make_shared<mov_sample_t>(); 
    _track->sample_count++;
    sample->sample_description_index = 1;
    sample->bytes = (uint32_t) frame->size() - frame->startSize() + 4;
    sample->flags = keyframe;
    sample->data = NULL;
    sample->pts = pts;
    sample->dts = dts;

    sample->offset = tell();
    _track->samples.push_back(sample);

    // if (!add_nalu_size) {
    //     mov_buffer_write(data, bytes);
    // } else {
        // uint8_t nalu_size_buf[4] = {0};
        // size_t nalu_size = sample->bytes;
        // nalu_size_buf[0] = (uint8_t) ((nalu_size >> 24) & 0xFF);
        // nalu_size_buf[1] = (uint8_t) ((nalu_size >> 16) & 0xFF);
        // nalu_size_buf[2] = (uint8_t) ((nalu_size >> 8) & 0xFF);
        // nalu_size_buf[3] = (uint8_t) ((nalu_size >> 0) & 0xFF);
        write32BE(sample->bytes - 4);
        // mov_buffer_write(nalu_size_buf, 4);
        write(frame->data() + frame->startSize(), sample->bytes - 4);
    // }

    if (INT64_MIN == _track->start_dts)
        _track->start_dts = sample->dts;
    _mdatSize += sample->bytes; // update media data size
	logInfo << "_mdatSize: =========== " << _mdatSize;
    // return mov_buffer_error(&mov->io);
}

void MP4Muxer::writeMdatSize(uint64_t offset, size_t size)
{
	uint64_t offset2;
    // assert(size < UINT32_MAX);
    // 保存当前位置
	offset2 = tell();

    // 切到mdat的开始位置
	seek(offset);
	write32BE(size);

	seek(offset2);
}

void MP4Muxer::stop()
{
	int i;
	uint64_t offset, offset2;
	std::shared_ptr<mov_track_t> track;

	// finish mdat box
	if (_mdatSize + 8 <= UINT32_MAX)
	{
		writeMdatSize(_mdatOffset, (uint32_t)(_mdatSize + 8)); /* update size */
	}
	else
	{
		offset2 = tell();
		_mdatOffset -= 8; // overwrite free box
		seek(_mdatOffset);
		write32BE(1);
		write("mdat", 4);
		write32BE((_mdatSize + 16) >> 32);
		write32BE((_mdatSize + 16));
		seek(offset2);
	}

	// finish sample info
	for (i = 0; i < _track_count; i++)
	{
		track = _tracks[i];
		if(track->sample_count < 1)
			continue;

		// pts in ms
		track->mdhd.duration = track->samples[track->sample_count - 1]->dts - track->samples[0]->dts;
		//track->mdhd.duration = track->mdhd.duration * track->mdhd.timescale / 1000;
		track->tkhd.duration = track->mdhd.duration * _mvhd.timescale / track->mdhd.timescale;
		if (track->tkhd.duration > _mvhd.duration)
			_mvhd.duration = track->tkhd.duration; // maximum track duration
	}

	// write moov box
	offset = tell();
	writeMoov();
	offset2 = tell();
	
	if (MOV_FLAG_FASTSTART & _flags)
	{
		// check stco -> co64
		uint64_t co64 = 0;
		for (i = 0; i < _track_count; i++)
		{
			co64 += mov_stco_size(_tracks[i].get(), offset2 - offset);
		}

		if (co64)
		{
			uint64_t sz;
			do
			{
				sz = co64;
				co64 = 0;
				for (i = 0; i < _track_count; i++)
				{
					co64 += mov_stco_size(_tracks[i].get(), offset2 - offset + sz);
				}
			} while (sz != co64);
		}

		// rewrite moov
		for (i = 0; i < _track_count; i++)
			_tracks[i]->offset += (offset2 - offset) + co64;

		seek(offset);
		writeMoov();
		assert(tell() == offset2 + co64);
		offset2 = tell();

		moveMoov(_mdatOffset, offset, (size_t)(offset2 - offset));
	}

    _track_count = 0;
    _tracks.clear();
}

size_t MP4Muxer::writeMoov()
{
	int i;
	size_t size;
	uint64_t offset;

	size = 8 /* Box */;
	offset = tell();
	write32BE(0); /* size */
	write("moov", 4);

	size += mov_write_mvhd();
	size += mov_write_iods();
	for(i = 0; i < _track_count; i++)
	{
		_track = _tracks[i];
		if (_track->sample_count < 1)
			continue;
		size += mov_write_trak();
	}

	size += mov_write_udta();
	writeMdatSize(offset, size); /* update size */
	return size;
}

int MP4Muxer::moveMoov(uint64_t to, uint64_t from, size_t bytes)
{
	uint8_t* ptr;
	uint64_t i, j;
	void* buffer[2];

	assert(bytes < INT32_MAX);
	ptr = (uint8_t*)malloc((size_t)(bytes * 2));
	if (NULL == ptr)
		return -ENOMEM;
	buffer[0] = ptr;
	buffer[1] = ptr + bytes;

	seek(from);
	read((char*)buffer[0], bytes);
    seek(to);
    read((char*)buffer[1], bytes);

	j = 0;
	for (i = to; i < from; i += bytes)
	{
		seek(i);
		write((char*)buffer[j], bytes);
        // MSDN: fopen https://msdn.microsoft.com/en-us/library/yeby3zcb.aspx
        // When the "r+", "w+", or "a+" access type is specified, both reading and 
        // writing are enabled (the file is said to be open for "update"). 
        // However, when you switch from reading to writing, the input operation 
        // must encounter an EOF marker. If there is no EOF, you must use an intervening 
        // call to a file positioning function. The file positioning functions are 
        // fsetpos, fseek, and rewind. 
        // When you switch from writing to reading, you must use an intervening 
        // call to either fflush or to a file positioning function.
        seek(i+bytes);
        read((char*)buffer[j], bytes);
        j ^= 1;
	}

    seek(i);
	write((char*)buffer[j], bytes - (size_t)(i - from));

	free(ptr);
	return 0;
}

size_t MP4Muxer::mov_stco_size(const mov_track_t* track, uint64_t offset)
{
	size_t i, j;
	uint64_t co64;
	const struct mov_sample_t* sample;

	if (track->sample_count < 1)
		return 0;

	sample = track->samples[track->sample_count - 1].get();
	co64 = sample->offset + track->offset;
	if (co64 > UINT32_MAX || co64 + offset <= UINT32_MAX)
		return 0;

	for (i = 0, j = 0; i < track->sample_count; i++)
	{
		sample = track->samples[i].get();
		if (0 != sample->first_chunk)
			j++;
	}

	return j * 4;
}

size_t MP4Muxer::mov_write_mvhd()
{
//	int rotation = 0; // 90/180/270
	const struct mov_mvhd_t* mvhd = &_mvhd;

	write32BE(108); /* size */
	write("mvhd", 4);
	write32BE(0); /* version & flags */

	write32BE((uint32_t)mvhd->creation_time); /* creation_time */
	write32BE((uint32_t)mvhd->modification_time); /* modification_time */
	write32BE(mvhd->timescale); /* timescale */
	write32BE((uint32_t)mvhd->duration); /* duration */

	write32BE(0x00010000); /* rate 1.0 */
	write16BE(0x0100); /* volume 1.0 = normal */
	write16BE(0); /* reserved */
	write32BE(0); /* reserved */
	write32BE(0); /* reserved */

	// matrix
	write32BE(0x00010000); /* u */
	write32BE(0);
	write32BE(0);
	write32BE(0); /* v */
	write32BE(0x00010000);
	write32BE(0);
	write32BE(0); /* w */
	write32BE(0);
	write32BE(0x40000000);

	write32BE(0); /* reserved (preview time) */
	write32BE(0); /* reserved (preview duration) */
	write32BE(0); /* reserved (poster time) */
	write32BE(0); /* reserved (selection time) */
	write32BE(0); /* reserved (selection duration) */
	write32BE(0); /* reserved (current time) */

	write32BE(mvhd->next_track_ID); /* Next track id */

	return 108;
}

size_t MP4Muxer::mov_write_iods()
{
    size_t size = 12 /* full box */ + 12 /* InitialObjectDescriptor */;

    write32BE(24); /* size */
    write("iods", 4);
    write32BE(0); /* version & flags */
    
    write8BE(0x10); // ISO_MP4_IOD_Tag
    write8BE((uint8_t)(0x80 | (7 >> 21)));
    write8BE((uint8_t)(0x80 | (7 >> 14)));
    write8BE((uint8_t)(0x80 | (7 >> 7)));
    write8BE((uint8_t)(0x7F & 7));

    write16BE(0x004f); // objectDescriptorId 1
    write8BE(0xff); // No OD capability required
    write8BE(0xff);
    write8BE(0xFF);
    write8BE(0xFF); // no visual capability required
    write8BE(0xff);

    return size;
}

size_t MP4Muxer::mov_write_trak()
{
    size_t size;
    uint64_t offset;

    size = 8 /* Box */;
    offset = tell();
    write32BE(0); /* size */
    write("trak", 4);

    size += mov_write_tkhd();
    //size += mov_write_tref(mov);
    size += mov_write_edts();
    size += mov_write_mdia();

    writeMdatSize(offset, size); /* update size */
    return size;
}

size_t MP4Muxer::mov_write_udta()
{
	if (!_udta || _udta_size < 1)
		return 0;

	write32BE(8 + (uint32_t)_udta_size);
	write("udta", 4);
	write((char*)_udta, _udta_size);
	return 8 + (size_t)_udta_size;
}

size_t MP4Muxer::mov_write_tkhd()
{
//	int rotation = 0; // 90/180/270
	uint16_t group = 0;
	const struct mov_tkhd_t* tkhd = &_track->tkhd;

	write32BE(92); /* size */
	write("tkhd", 4);
	write8BE(0); /* version */
	write24BE(tkhd->flags); /* flags */

	write32BE((uint32_t)tkhd->creation_time); /* creation_time */
	write32BE((uint32_t)tkhd->modification_time); /* modification_time */
	write32BE(tkhd->track_ID); /* track_ID */
	write32BE(0); /* reserved */
	write32BE((uint32_t)tkhd->duration); /* duration */

	write32BE(0); /* reserved */
	write32BE(0); /* reserved */
	write16BE(tkhd->layer); /* layer */
	write16BE(group); /* alternate_group */
	//mov_buffer_w16(AVSTREAM_AUDIO == track->stream_type ? 0x0100 : 0); /* volume */
	write16BE(tkhd->volume); /* volume */
	write16BE(0); /* reserved */

	// matrix
	//for (i = 0; i < 9; i++)
	//	file_reader_rb32(mov->fp, tkhd->matrix[i]);
	write32BE(0x00010000); /* u */
	write32BE(0);
	write32BE(0);
	write32BE(0); /* v */
	write32BE(0x00010000);
	write32BE(0);
	write32BE(0); /* w */
	write32BE(0);
	write32BE(0x40000000);

	write32BE(tkhd->width /*track->av.video.width * 0x10000U*/); /* width */
	write32BE(tkhd->height/*track->av.video.height * 0x10000U*/); /* height */
	return 92;
}

size_t MP4Muxer::mov_write_edts()
{
    size_t size;
    uint64_t offset;

    if (_track->sample_count < 1)
        return 0;

    size = 8 /* Box */;
    offset = tell();
    write32BE(0); /* size */
    write("edts", 4);

    size += mov_write_elst();

    writeMdatSize(offset, size); /* update size */
    return size;
}

size_t MP4Muxer::mov_write_mdia()
{
    size_t size;
    uint64_t offset;

    size = 8 /* Box */;
    offset = tell();
    write32BE(0); /* size */
    write("mdia", 4);

    size += mov_write_mdhd();
    size += mov_write_hdlr();
    size += mov_write_minf();

    writeMdatSize(offset, size); /* update size */
    return size;
}

size_t MP4Muxer::mov_write_elst()
{
	uint32_t size;
	int64_t time;
	int64_t delay;
	uint8_t version;
	const mov_track_t* track = _track.get();

    assert(track->start_dts == track->samples[0]->dts);
	version = track->tkhd.duration > UINT32_MAX ? 1 : 0;

    // in media time scale units, in composition time
	time = track->samples[0]->pts - track->samples[0]->dts;
    // in units of the timescale in the Movie Header Box
	delay = track->samples[0]->pts * _mvhd.timescale / track->mdhd.timescale;
	if (delay > UINT32_MAX)
		version = 1;

	time = time < 0 ? 0 : time;
	size = 12/* full box */ + 4/* entry count */ + (delay > 0 ? 2 : 1) * (version ? 20 : 12);

	write32BE(size); /* size */
	write("elst", 4);
	write8BE(version); /* version */
	write24BE(0); /* flags */
	write32BE(delay > 0 ? 2 : 1); /* entry count */

	if (delay > 0)
	{
		if (1 == version)
		{
			write64BE((uint64_t)delay); /* segment_duration */
			write64BE((uint64_t)-1); /* media_time */
		}
		else
		{
			write32BE((uint32_t)delay);
			write32BE((uint32_t)-1);
		}

		write16BE(1); /* media_rate_integer */
		write16BE(0); /* media_rate_fraction */
	}

	/* duration */
	if (version == 1) 
	{
		write64BE(track->tkhd.duration);
		write64BE(time);
	}
	else 
	{
		write32BE((uint32_t)track->tkhd.duration);
		write32BE((uint32_t)time);
	}
	write16BE(1); /* media_rate_integer */
	write16BE(0); /* media_rate_fraction */

	return size;
}

size_t MP4Muxer::mov_write_mdhd()
{
	const struct mov_mdhd_t* mdhd = &_track->mdhd;
	
	write32BE(32); /* size */
	write("mdhd", 4);
	write32BE(0); /* version 1 & flags */

	write32BE((uint32_t)mdhd->creation_time); /* creation_time */
	write32BE((uint32_t)mdhd->modification_time); /* modification_time */
	write32BE(mdhd->timescale); /* timescale */
	write32BE((uint32_t)mdhd->duration); /* duration */
	
	write16BE((uint16_t)mdhd->language); /* ISO-639-2/T language code */
	write16BE(0); /* pre_defined (quality) */
	return 32;
}

size_t MP4Muxer::mov_write_hdlr()
{
	const mov_track_t* track = _track.get();

	write32BE(33 + (uint32_t)strlen(track->handler_descr)); /* size */
	write("hdlr", 4);
	write32BE(0); /* Version & flags */

	write32BE(0); /* pre_defined */
	write32BE(track->handler_type); /* handler_type */

	write32BE(0); /* reserved */
	write32BE(0); /* reserved */
	write32BE(0); /* reserved */

	write(track->handler_descr, (uint64_t)strlen(track->handler_descr)+1); /* name */
	return 33 + strlen(track->handler_descr);
}

size_t MP4Muxer::mov_write_minf()
{
    size_t size;
    uint64_t offset;
    const mov_track_t* track = _track.get();

    size = 8 /* Box */;
    offset = tell();
    write32BE(0); /* size */
    write("minf", 4);

    if (MOV_VIDEO == track->handler_type)
    {
        size += mov_write_vmhd();
    }
    else if (MOV_AUDIO == track->handler_type)
    {
        size += mov_write_smhd();
    }
    else if (MOV_SUBT == track->handler_type)
    {
        size += mov_write_nmhd();
    }
    else
    {
        assert(0);
    }

    size += mov_write_dinf();
    size += mov_write_stbl();
    writeMdatSize(offset, size); /* update size */
    return size;
}

size_t MP4Muxer::mov_write_vmhd()
{
	write32BE(20); /* size (always 0x14) */
	write("vmhd", 4);
	write32BE(0x01); /* version & flags */
	write64BE(0); /* reserved (graphics mode = copy) */
	return 20;
}

size_t MP4Muxer::mov_write_smhd()
{
	write32BE(16); /* size */
	write("smhd", 4);
	write32BE(0); /* version & flags */
	write16BE(0); /* reserved (balance, normally = 0) */
	write16BE(0); /* reserved */
	return 16;
}

size_t MP4Muxer::mov_write_nmhd()
{
	write32BE(12); /* size */
	write("nmhd", 4);
	write32BE(0); /* version & flags */
	return 12;
}

size_t MP4Muxer::mov_write_dinf()
{
	size_t size;
	uint64_t offset;

	size = 8 /* Box */;
	offset = tell();
	write32BE(0); /* size */
	write("dinf", 4);

	size += mov_write_dref();

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t MP4Muxer::mov_write_stbl()
{
    size_t size;
    uint32_t count;
    uint64_t offset;
    mov_track_t* track;
    track = (mov_track_t*)_track.get();

    size = 8 /* Box */;
    offset = tell();
    write32BE(0); /* size */
    write("stbl", 4);

    size += mov_write_stsd();

    count = mov_build_stts(track);
    size += mov_write_stts(count);
    if (track->tkhd.width > 0 && track->tkhd.height > 0)
        size += mov_write_stss(); // video only
    count = mov_build_ctts(track);
    if (track->sample_count > 0 && (count > 1 || track->samples[0]->samples_per_chunk != 0))
        size += mov_write_ctts(count);

    count = mov_build_stco(track);
    size += mov_write_stsc();
    size += mov_write_stsz();
    size += mov_write_stco(count);

    writeMdatSize(offset, size); /* update size */
    return size;
}

size_t MP4Muxer::mov_write_dref()
{
	write32BE(28); /* size */
	write("dref", 4);
	write32BE(0); /* version & flags */
	write32BE(1); /* entry count */

	write32BE(12); /* size */
	//FIXME add the alis and rsrc atom
	write("url ", 4);
	write32BE(1); /* version & flags */

	return 28;
}

size_t MP4Muxer::mov_write_stsd()
{
	uint32_t i;
	size_t size;
	uint64_t offset;
	const mov_track_t* track = _track.get();

	size = 12 /* full box */ + 4 /* entry count */;

	offset = tell();
	write32BE(0); /* size */
	write("stsd", 4);
	write32BE(0); /* version & flags */
	write32BE(track->stsd.entry_count); /* entry count */

	for (i = 0; i < track->stsd.entry_count; i++)
	{
        ((mov_track_t*)track)->stsd.current = track->stsd.entries[i];

		if (MOV_VIDEO == track->handler_type)
		{
			size += mov_write_video(track->stsd.entries[i].get());
		}
		else if (MOV_AUDIO == track->handler_type)
		{
			size += mov_write_audio(track->stsd.entries[i].get());
		}
		// else if (MOV_SUBT == track->handler_type || MOV_TEXT == track->handler_type)
		// {
		// 	size += mov_write_subtitle(mov, &track->stsd.entries[i]);
		// }
		else
		{
			assert(0);
		}
	}

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t MP4Muxer::mov_write_stts(uint32_t count)
{
	uint32_t size, i;
	const struct mov_sample_t* sample;
	const mov_track_t* track = _track.get();

	size = 12/* full box */ + 4/* entry count */ + count * 8/* entry */;

	write32BE(size); /* size */
	write("stts", 4);
	write32BE(0); /* version & flags */
	write32BE(count); /* entry count */

	for (i = 0; i < track->sample_count; i++)
	{
		sample = track->samples[i].get();
		if(0 == sample->first_chunk)
			continue;
		write32BE(sample->first_chunk); // count
		write32BE(sample->samples_per_chunk); // delta * timescale / 1000
	}

	return size;
}

size_t MP4Muxer::mov_write_ctts(uint32_t count)
{
	uint32_t size, i;
	const struct mov_sample_t* sample;
	const mov_track_t* track = _track.get();

	size = 12/* full box */ + 4/* entry count */ + count * 8/* entry */;

	write32BE(size); /* size */
	write("ctts", 4);
	write8BE((track->flags & MOV_TRACK_FLAG_CTTS_V1) ? 1 : 0); /* version */
	write24BE(0); /* flags */
	write32BE(count); /* entry count */

	for (i = 0; i < track->sample_count; i++)
	{
		sample = track->samples[i].get();
		if(0 == sample->first_chunk)
			continue;
		write32BE(sample->first_chunk); // count
		write32BE(sample->samples_per_chunk); // offset * timescale / 1000
	}

	return size;
}

uint32_t MP4Muxer::mov_build_stts(mov_track_t* track)
{
    size_t i;
    uint32_t delta, count = 0;
    struct mov_sample_t* sample = NULL;

    for (i = 0; i < track->sample_count; i++)
    {
		if (i < (track->sample_count - 1)) {
			assert(track->samples[i + 1]->dts >= track->samples[i]->dts || i + 1 == track->sample_count);
		}
        delta = (uint32_t)(i < (track->sample_count - 1) && track->samples[i + 1]->dts > track->samples[i]->dts ? track->samples[i + 1]->dts - track->samples[i]->dts : 1);
        if (NULL != sample && delta == sample->samples_per_chunk)
        {
            track->samples[i]->first_chunk = 0;
            assert(sample->first_chunk > 0);
            ++sample->first_chunk; // compress
        }
        else
        {
            sample = track->samples[i].get();
            sample->first_chunk = 1;
            sample->samples_per_chunk = delta;
            ++count;
        }
    }
    return count;
}

uint32_t MP4Muxer::mov_build_ctts(mov_track_t* track)
{
    size_t i;
    uint32_t delta;
    uint32_t count = 0;
    struct mov_sample_t* sample = NULL;

    for (i = 0; i < track->sample_count; i++)
    {
        delta = (uint32_t)(track->samples[i]->pts - track->samples[i]->dts);
        if (i > 0 && delta == sample->samples_per_chunk)
        {
            track->samples[i]->first_chunk = 0;
            assert(sample->first_chunk > 0);
            ++sample->first_chunk; // compress
        }
        else
        {
            sample = track->samples[i].get();
            sample->first_chunk = 1;
            sample->samples_per_chunk = delta;
			++count;

			// fixed: firefox version 51 don't support version 1
			if (track->samples[i]->pts < track->samples[i]->dts)
				track->flags |= MOV_TRACK_FLAG_CTTS_V1;
        }
    }

    return count;
}

size_t MP4Muxer::mov_write_stss()
{
	uint64_t offset;
	uint64_t offset2;
	uint32_t size, i, j;
	const struct mov_sample_t* sample;
	const mov_track_t* track = _track.get();

	size = 12/* full box */ + 4/* entry count */;

	offset = tell();
	write32BE(0); /* size */
	write("stss", 4);
	write32BE(0); /* version & flags */
	write32BE(0); /* entry count */

	for (i = 0, j = 0; i < track->sample_count; i++)
	{
		sample = track->samples[i].get();
		if (sample->flags & MOV_AV_FLAG_KEYFREAME)
		{
			++j;
			write32BE(i + 1); // start from 1
		}
	}

	size += j * 4/* entry */;
	offset2 = tell();
	seek(offset);
	write32BE(size); /* size */
	seek(offset + 12);
	write32BE(j); /* entry count */
	seek(offset2);
	return size;
}

uint32_t MP4Muxer::mov_build_stco(mov_track_t* track)
{
    size_t i;
    size_t bytes = 0;
    uint32_t count = 0;
    struct mov_sample_t* sample = NULL;

    assert(track->stsd.entry_count > 0);
    for (i = 0; i < track->sample_count; i++)
    {
        if (NULL != sample
            && sample->offset + bytes == track->samples[i]->offset
            && sample->sample_description_index == track->samples[i]->sample_description_index)
        {
            track->samples[i]->first_chunk = 0; // mark invalid value
            bytes += track->samples[i]->bytes;
            ++sample->samples_per_chunk;
        }
        else
        {
            sample = track->samples[i].get();
            sample->first_chunk = ++count; // chunk start from 1
            sample->samples_per_chunk = 1;
            bytes = sample->bytes;
        }
    }

    return count;
}

size_t MP4Muxer::mov_write_stsc()
{
	uint64_t offset;
	uint64_t offset2;
	uint32_t size, i, entry;
	const struct mov_sample_t* chunk = NULL;
	const struct mov_sample_t* sample = NULL;
	const mov_track_t* track = _track.get();

	size = 12/* full box */ + 4/* entry count */;

	offset = tell();
	write32BE(0); /* size */
	write("stsc", 4);
	write32BE(0); /* version & flags */
	write32BE(0); /* entry count */

	for (i = 0, entry = 0; i < track->sample_count; i++)
	{
		sample = track->samples[i].get();
		if (0 == sample->first_chunk || 
			(chunk && chunk->samples_per_chunk == sample->samples_per_chunk 
				&& chunk->sample_description_index == sample->sample_description_index))
			continue;

		++entry;
		chunk = sample;
		write32BE(sample->first_chunk);
		write32BE(sample->samples_per_chunk);
		write32BE(sample->sample_description_index);
	}

	size += entry * 12/* entry size*/;
	offset2 = tell();
	seek(offset);
	write32BE(size); /* size */
	seek(offset + 12);
	write32BE(entry); /* entry count */
	seek(offset2);
	return size;
}

size_t MP4Muxer::mov_write_stsz()
{
	uint32_t size, i;
	const mov_track_t* track = _track.get();

	for(i = 1; i < track->sample_count; i++)
	{
		if(track->samples[i]->bytes != track->samples[i-1]->bytes)
			break;
	}

	size = 12/* full box */ + 8 + (i < track->sample_count ? 4 * track->sample_count : 0);
	write32BE(size); /* size */
	write("stsz", 4);
	write32BE(0); /* version & flags */

	if(i < track->sample_count)
	{
		write32BE(0);
		write32BE(track->sample_count);
		for(i = 0; i < track->sample_count; i++)
			write32BE(track->samples[i]->bytes);
	}
	else
	{
		write32BE(track->sample_count < 1 ? 0 : track->samples[0]->bytes);
		write32BE(track->sample_count);
	}

	return size;
}

size_t MP4Muxer::mov_write_stco(uint32_t count)
{
	int co64;
	uint32_t size, i;
	const struct mov_sample_t* sample;
	const mov_track_t* track = _track.get();

	sample = track->sample_count > 0 ? track->samples[track->sample_count - 1].get() : NULL;
	co64 = (sample && sample->offset + track->offset > UINT32_MAX) ? 1 : 0;
	size = 12/* full box */ + 4/* entry count */ + count * (co64 ? 8 : 4);

	write32BE(size); /* size */
	write(co64 ? "co64" : "stco", 4);
	write32BE(0); /* version & flags */
	write32BE(count); /* entry count */

	for (i = 0; i < track->sample_count; i++)
	{
		sample = track->samples[i].get();
		if(0 == sample->first_chunk)
			continue;

		if(0 == co64)
			write32BE((uint32_t)(sample->offset + track->offset));
		else
			write64BE(sample->offset + track->offset);
	}

	return size;
}

size_t MP4Muxer::mov_write_video(const struct mov_sample_entry_t* entry)
{
	size_t size;
	uint64_t offset;
    char compressorname[32];
    memset(compressorname, 0, sizeof(compressorname));
	assert(1 == entry->data_reference_index);

	size = 8 /* Box */ + 8 /* SampleEntry */ + 70 /* VisualSampleEntry */;

	offset = tell();
	write32BE(0); /* size */
	write32BE(_track->tag); // "h264"

	write32BE(0); /* Reserved */
	write16BE(0); /* Reserved */
	write16BE(entry->data_reference_index); /* Data-reference index */

	write16BE(0); /* Reserved / Codec stream version */
	write16BE(0); /* Reserved / Codec stream revision (=0) */
	write32BE(0); /* Reserved */
	write32BE(0); /* Reserved */
	write32BE(0); /* Reserved */

	write16BE(entry->u.visual.width); /* Video width */
	write16BE(entry->u.visual.height); /* Video height */
	write32BE(0x00480000); /* Horizontal resolution 72dpi */
	write32BE(0x00480000); /* Vertical resolution 72dpi */
	write32BE(0); /* reserved / Data size (= 0) */
	write16BE(1); /* Frame count (= 1) */

	// ISO 14496-15:2017 AVCC \012AVC Coding
	// ISO 14496-15:2017 HVCC \013HEVC Coding
	//mov_buffer_w8(0 /*strlen(compressor_name)*/); /* compressorname */
	write(compressorname, 32); // fill empty

    // ISO/IEC 14496-15:2017 4.5 Template field used (19)
    // 0x18 - the video sequence is in color with no alpha
    // 0x28 - the video sequence is in grayscale with no alpha
    // 0x20 - the video sequence has alpha (gray or color)
	write16BE(0x18); /* Reserved */
	write16BE(0xffff); /* Reserved */

	if(MOV_OBJECT_H264 == entry->object_type_indication)
		size += mov_write_avcc();
	else if(MOV_OBJECT_MP4V == entry->object_type_indication)
		size += mov_write_esds();
	else if (MOV_OBJECT_HEVC == entry->object_type_indication)
		size += mov_write_hvcc();
	else if (MOV_OBJECT_AV1 == entry->object_type_indication)
		size += mov_write_av1c();
    else if (MOV_OBJECT_VP8 == entry->object_type_indication || MOV_OBJECT_VP9 == entry->object_type_indication)
        size += mov_write_vpcc();

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t MP4Muxer::mov_write_audio(const struct mov_sample_entry_t* entry)
{
	size_t size;
	uint64_t offset;

	size = 8 /* Box */ + 8 /* SampleEntry */ + 20 /* AudioSampleEntry */;

	offset = tell();
	write32BE(0); /* size */
	write32BE(_track->tag); // "mp4a"

	write32BE(0); /* Reserved */
	write16BE(0); /* Reserved */
	write16BE(1); /* Data-reference index */

	/* SoundDescription */
	write16BE(0); /* Version */
	write16BE(0); /* Revision level */
	write32BE(0); /* Reserved */

	write16BE(entry->u.audio.channelcount); /* channelcount */
	write16BE(entry->u.audio.samplesize); /* samplesize */

	write16BE(0); /* pre_defined */
	write16BE(0); /* reserved / packet size (= 0) */

	// https://www.opus-codec.org/docs/opus_in_isobmff.html
	// 4.3 Definitions of Opus sample
	// OpusSampleEntry: 
	// 1. The samplesize field shall be set to 16.
	// 2. The samplerate field shall be set to 48000<<16.
	write32BE(entry->u.audio.samplerate); /* samplerate */

	if(MOV_OBJECT_AAC == entry->object_type_indication
		 || MOV_OBJECT_MP3 == entry->object_type_indication 
		 || MOV_OBJECT_MP1A == entry->object_type_indication)
		size += mov_write_esds();
    else if(MOV_OBJECT_OPUS == entry->object_type_indication)
        size += mov_write_dops();

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t MP4Muxer::mov_write_avcc()
{
	const mov_track_t* track = _track.get();
	const struct mov_sample_entry_t* entry = track->stsd.current.get();
	write32BE(entry->extra_data_size + 8); /* size */
	write("avcC", 4);
	if (entry->extra_data_size > 0)
		write(entry->extra_data.data(), entry->extra_data_size);
	return entry->extra_data_size + 8;
}

size_t MP4Muxer::mov_write_esds()
{
	size_t size;
	uint64_t offset;

	size = 12 /* full box */;
	offset = tell();
	write32BE(0); /* size */
	write("esds", 4);
	write32BE(0); /* version & flags */

	size += mp4_write_es_descriptor();

	writeMdatSize(offset, size); /* update size */
	return size;
}

size_t MP4Muxer::mp4_write_es_descriptor()
{
	uint32_t size = 3; // mp4_write_decoder_config_descriptor
	const struct mov_sample_entry_t* entry = _track->stsd.current.get();
	size += 5 + 13 + (entry->extra_data_size > 0 ? entry->extra_data_size + 5 : 0); // mp4_write_decoder_config_descriptor
	size += 5 + 1; // mp4_write_sl_config_descriptor

	size += mov_write_base_descr(ISO_ESDescrTag, size);
	write16BE((uint16_t)_track->tkhd.track_ID); // ES_ID
	write8BE(0x00); // flags (= no flags)

	mp4_write_decoder_config_descriptor();
	mp4_write_sl_config_descriptor();
	return size;
}

uint32_t MP4Muxer::mov_write_base_descr(uint8_t tag, uint32_t len)
{
	write8BE(tag);
	write8BE((uint8_t)(0x80 | (len >> 21)));
	write8BE((uint8_t)(0x80 | (len >> 14)));
	write8BE((uint8_t)(0x80 | (len >> 7)));
	write8BE((uint8_t)(0x7F & len));
	return 5;
}

int MP4Muxer::mp4_write_decoder_config_descriptor()
{
	const struct mov_sample_entry_t* entry = _track->stsd.current.get();
	int size = 13 + (entry->extra_data_size > 0 ? entry->extra_data_size + 5 : 0);
	mov_write_base_descr(ISO_DecoderConfigDescrTag, size);
	write8BE(entry->object_type_indication);
	write8BE(0x01/*reserved*/ | (entry->stream_type << 2));
	write24BE(0); /* buffer size db */
	write32BE(88360); /* max bit-rate */
	write32BE(88360); /* avg bit-rate */

	if (entry->extra_data_size > 0)
		mp4_write_decoder_specific_info();

	return size;
}

size_t MP4Muxer::mp4_write_sl_config_descriptor()
{
	size_t size = 1;
	size += mov_write_base_descr(ISO_SLConfigDescrTag, 1);
	write8BE(0x02);
	return size;
}

int MP4Muxer::mp4_write_decoder_specific_info()
{
	const struct mov_sample_entry_t* entry = _track->stsd.current.get();
	mov_write_base_descr(ISO_DecSpecificInfoTag, entry->extra_data_size);
	write(entry->extra_data.data(), entry->extra_data_size);
	return entry->extra_data_size;
}

size_t MP4Muxer::mov_write_hvcc()
{
	const mov_track_t* track = _track.get();
	const struct mov_sample_entry_t* entry = track->stsd.current.get();
	write32BE(entry->extra_data_size + 8); /* size */
	write("hvcC", 4);
	if (entry->extra_data_size > 0)
		write(entry->extra_data.data(), entry->extra_data_size);
	return entry->extra_data_size + 8;
}

size_t MP4Muxer::mov_write_av1c()
{
	const mov_track_t* track = _track.get();
	const struct mov_sample_entry_t* entry = track->stsd.current.get();
	write32BE(entry->extra_data_size + 8); /* size */
	write("av1C", 4);
	if (entry->extra_data_size > 0)
		write(entry->extra_data.data(), entry->extra_data_size);
	return entry->extra_data_size + 8;
}

size_t MP4Muxer::mov_write_vpcc()
{
    const mov_track_t* track = _track.get();
    const struct mov_sample_entry_t* entry = track->stsd.current.get();
    write32BE(entry->extra_data_size + 12); /* size */
    write("vpcC", 4);
    write8BE(1); /* version */
    write24BE(0); /* flags */
    if (entry->extra_data_size > 0)
        write(entry->extra_data.data(), entry->extra_data_size);
    return entry->extra_data_size + 12;
}

size_t MP4Muxer::mov_write_dops()
{
    const mov_track_t* track = _track.get();
    const struct mov_sample_entry_t* entry = track->stsd.current.get();
    if (entry->extra_data_size < 18)
        return 0;
    
    assert(0 == memcmp(entry->extra_data.data(), "OpusHead", 8));
    write32BE(entry->extra_data_size); /* size */
    write("dOps", 4);
    write8BE(0); // The Version field shall be set to 0.
    write8BE(entry->extra_data[9]); // channel count
    write16BE((entry->extra_data[11]<<8) | entry->extra_data[10]); // PreSkip (LSB -> MSB)
    write32BE((entry->extra_data[15]<<8) | (entry->extra_data[14]<<8) | (entry->extra_data[13]<<8) | entry->extra_data[12]); // InputSampleRate (LSB -> MSB)
    write16BE((entry->extra_data[17]<<8) | entry->extra_data[16]); // OutputGain (LSB -> MSB)
    write(entry->extra_data.data() + 18, entry->extra_data_size - 18);
    return entry->extra_data_size;
}