#ifndef MP4Box_H
#define MP4Box_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

// using namespace std;

#define N_BRAND	8

#define MOV_TAG(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define MOV_NULL    MOV_TAG(0, 0, 0, 0)
#define MOV_MOOV    MOV_TAG('m', 'o', 'o', 'v')
#define MOV_ROOT    MOV_TAG('r', 'o', 'o', 't')
#define MOV_TRAK    MOV_TAG('t', 'r', 'a', 'k')
#define MOV_MDIA    MOV_TAG('m', 'd', 'i', 'a')
#define MOV_EDTS    MOV_TAG('e', 'd', 't', 's')
#define MOV_MINF    MOV_TAG('m', 'i', 'n', 'f')
#define MOV_GMHD    MOV_TAG('g', 'm', 'h', 'd') // Apple QuickTime gmhd(text media)
#define MOV_DINF    MOV_TAG('d', 'i', 'n', 'f')
#define MOV_STBL    MOV_TAG('s', 't', 'b', 'l')
#define MOV_MVEX    MOV_TAG('m', 'v', 'e', 'x')
#define MOV_MOOF    MOV_TAG('m', 'o', 'o', 'f')
#define MOV_TRAF    MOV_TAG('t', 'r', 'a', 'f')
#define MOV_MFRA    MOV_TAG('m', 'f', 'r', 'a')

#define MOV_VIDEO	MOV_TAG('v', 'i', 'd', 'e') // ISO/IEC 14496-12:2015(E) 12.1 Video media (p169)
#define MOV_AUDIO	MOV_TAG('s', 'o', 'u', 'n') // ISO/IEC 14496-12:2015(E) 12.2 Audio media (p173)
#define MOV_META	MOV_TAG('m', 'e', 't', 'a') // ISO/IEC 14496-12:2015(E) 12.3 Metadata media (p181)
#define MOV_HINT	MOV_TAG('h', 'i', 'n', 't') // ISO/IEC 14496-12:2015(E) 12.4 Hint media (p183)
#define MOV_TEXT	MOV_TAG('t', 'e', 'x', 't') // ISO/IEC 14496-12:2015(E) 12.5 Text media (p184)
#define MOV_SUBT	MOV_TAG('s', 'u', 'b', 't') // ISO/IEC 14496-12:2015(E) 12.6 Subtitle media (p185)
#define MOV_FONT	MOV_TAG('f', 'd', 's', 'm') // ISO/IEC 14496-12:2015(E) 12.7 Font media (p186)
#define MOV_CLCP	MOV_TAG('c', 'l', 'c', 'p') // ClosedCaptionHandler
#define MOV_ALIS	MOV_TAG('a', 'l', 'i', 's') // Apple QuickTime Macintosh alias

// https://developer.apple.com/library/content/documentation/General/Reference/HLSAuthoringSpec/Requirements.html#//apple_ref/doc/uid/TP40016596-CH2-SW1
// Video encoding requirements 1.10: Use 'avc1', 'hvc1', or 'dvh1' rather than 'avc3', 'hev1', or 'dvhe'
#define MOV_H264    MOV_TAG('a', 'v', 'c', '1') // H.264 ISO/IEC 14496-15:2010(E) 5.3.4 AVC Video Stream Definition (18)
#define MOV_HEVC    MOV_TAG('h', 'v', 'c', '1') // H.265
#define MOV_MP4V    MOV_TAG('m', 'p', '4', 'v') // MPEG-4 Video
#define MOV_MP4A    MOV_TAG('m', 'p', '4', 'a') // AAC
#define MOV_MP4S    MOV_TAG('m', 'p', '4', 's') // ISO/IEC 14496-14:2003(E) 5.6 Sample Description Boxes (p14)
#define MOV_OPUS    MOV_TAG('O', 'p', 'u', 's') // http://www.opus-codec.org/docs/opus_in_isobmff.html
#define MOV_VP8     MOV_TAG('v', 'p', '0', '8')
#define MOV_VP9     MOV_TAG('v', 'p', '0', '9') // https://www.webmproject.org/vp9/mp4/
#define MOV_VP10    MOV_TAG('v', 'p', '1', '0')
#define MOV_AV1     MOV_TAG('a', 'v', '0', '1') // https://aomediacodec.github.io/av1-isobmff
#define MOV_VC1     MOV_TAG('v', 'c', '-', '1')
#define MOV_DIRAC   MOV_TAG('d', 'r', 'a', 'c')
#define MOV_AC3     MOV_TAG('a', 'c', '-', '3')
#define MOV_DTS     MOV_TAG('d', 't', 's', 'c') // DTS-HD

// ISO/IEC 14496-1:2010(E) 7.2.6.6 DecoderConfigDescriptor
// Table 6 - streamType Values (p51)
enum
{
	MP4_STREAM_ODS		= 0x01, /* ObjectDescriptorStream */
	MP4_STREAM_CRS		= 0x02, /* ClockReferenceStream */
	MP4_STREAM_SDS		= 0x03, /* SceneDescriptionStream */
	MP4_STREAM_VISUAL	= 0x04, /* VisualStream */
	MP4_STREAM_AUDIO	= 0x05, /* AudioStream */
	MP4_STREAM_MP7		= 0x06, /* MPEG7Stream */
	MP4_STREAM_IPMP		= 0x07, /* IPMPStream */
	MP4_STREAM_OCIS		= 0x08, /* ObjectContentInfoStream */
	MP4_STREAM_MPEGJ	= 0x09, /* MPEGJStream */
	MP4_STREAM_IS		= 0x0A, /* Interaction Stream */
	MP4_STREAM_IPMPTOOL = 0x0B, /* IPMPToolStream */
};

enum
{
	MOV_BRAND_ISOM = MOV_TAG('i', 's', 'o', 'm'),
	MOV_BRAND_AVC1 = MOV_TAG('a', 'v', 'c', '1'),
	MOV_BRAND_ISO2 = MOV_TAG('i', 's', 'o', '2'),
	MOV_BRAND_MP71 = MOV_TAG('m', 'p', '7', '1'),
	MOV_BRAND_ISO3 = MOV_TAG('i', 's', 'o', '3'),
	MOV_BRAND_ISO4 = MOV_TAG('i', 's', 'o', '4'),
	MOV_BRAND_ISO5 = MOV_TAG('i', 's', 'o', '5'),
	MOV_BRAND_ISO6 = MOV_TAG('i', 's', 'o', '6'),
	MOV_BRAND_MP41 = MOV_TAG('m', 'p', '4', '1'), // ISO/IEC 14496-1:2001 MP4 File Format v1
	MOV_BRAND_MP42 = MOV_TAG('m', 'p', '4', '2'), // ISO/IEC 14496-14:2003 MP4 File Format v2
	MOV_BRAND_MOV  = MOV_TAG('q', 't', ' ', ' '), // Apple Quick-Time File Format
	MOV_BRAND_DASH = MOV_TAG('d', 'a', 's', 'h'), // MPEG-DASH
	MOV_BRAND_MSDH = MOV_TAG('m', 's', 'd', 'h'), // MPEG-DASH
	MOV_BRAND_MSIX = MOV_TAG('m', 's', 'i', 'x'), // MPEG-DASH
};

enum
{
	MOV_TKHD_FLAG_TRACK_ENABLE = 0x01,
	MOV_TKHD_FLAG_TRACK_IN_MOVIE = 0x02,
	MOV_TKHD_FLAG_TRACK_IN_PREVIEW = 0x04,
};

enum {
	ISO_ObjectDescrTag = 0x01,
	ISO_InitialObjectDescrTag = 0x02,
	ISO_ESDescrTag = 0x03,
	ISO_DecoderConfigDescrTag = 0x04,
	ISO_DecSpecificInfoTag = 0x05,
	ISO_SLConfigDescrTag = 0x06,
	ISO_ContentIdentDescrTag = 0x07,
	ISO_SupplContentIdentDescrTag = 0x08,
	ISO_IPI_DescrPointerTag = 0x09,
	ISO_IPMP_DescrPointerTag = 0x0A,
	ISO_IPMP_DescrTag = 0x0B,
	ISO_QoS_DescrTag = 0x0C,
	ISO_RegistrationDescrTag = 0x0D,
	ISO_ES_ID_IncTag = 0x0E,
	ISO_ES_ID_RefTag = 0x0F,
	ISO_MP4_IOD_Tag = 0x10,
	ISO_MP4_OD_Tag = 0x11,
};

// ISO/IEC 14496-1:2010(E)
// 7.2.2.3 BaseCommand
// Table-2 List of Class Tags for Commands (p33)
enum {
	ISO_ObjectDescrUpdateTag = 0x01,
	ISO_ObjectDescrRemoveTag = 0x02,
	ISO_ES_DescrUpdateTag = 0x03,
	ISO_ES_DescrRemoveTag = 0x04,
	ISO_IPMP_DescrUpdateTag = 0x05,
	ISO_IPMP_DescrRemoveTag = 0x06,
	ISO_ES_DescrRemoveRefTag = 0x07,
	ISO_ObjectDescrExecuteTag = 0x08,
	ISO_User_Private = 0xC0,
};

#define AV_TRACK_TIMEBASE 1000

#define MOV_TREX_FLAG_IS_LEADING_MASK					0x0C000000
#define MOV_TREX_FLAG_SAMPLE_DEPENDS_ON_MASK			0x03000000
#define MOV_TREX_FLAG_SAMPLE_IS_DEPENDED_ON_MASK		0x00C00000
#define MOV_TREX_FLAG_SAMPLE_HAS_REDUNDANCY_MASK		0x00300000
#define MOV_TREX_FLAG_SAMPLE_PADDING_VALUE_MASK			0x000E0000
#define MOV_TREX_FLAG_SAMPLE_IS_NO_SYNC_SAMPLE			0x00010000
#define MOV_TREX_FLAG_SAMPLE_DEGRADATION_PRIORITY_MASK	0x0000FFFF

// 8.6.4 Independent and Disposable Samples Box (p55)
#define MOV_TREX_FLAG_SAMPLE_DEPENDS_ON_I_PICTURE       0x02000000
#define MOV_TREX_FLAG_SAMPLE_DEPENDS_ON_NOT_I_PICTURE   0x01000000

#define MOV_TFHD_FLAG_BASE_DATA_OFFSET					0x00000001
#define MOV_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX			0x00000002
#define MOV_TFHD_FLAG_DEFAULT_DURATION					0x00000008
#define MOV_TFHD_FLAG_DEFAULT_SIZE						0x00000010
#define MOV_TFHD_FLAG_DEFAULT_FLAGS						0x00000020
#define MOV_TFHD_FLAG_DURATION_IS_EMPTY					0x00010000
#define MOV_TFHD_FLAG_DEFAULT_BASE_IS_MOOF				0x00020000

#define MOV_TRUN_FLAG_DATA_OFFSET_PRESENT						0x0001
#define MOV_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT				0x0004
#define MOV_TRUN_FLAG_SAMPLE_DURATION_PRESENT					0x0100
#define MOV_TRUN_FLAG_SAMPLE_SIZE_PRESENT						0x0200
#define MOV_TRUN_FLAG_SAMPLE_FLAGS_PRESENT						0x0400
#define MOV_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT	0x0800

#define MOV_TRACK_FLAG_CTTS_V1							0x0001 //ctts version 1

#define MOV_OBJECT_TEXT		0x08 // Text Stream
#define MOV_OBJECT_MP4V		0x20 // Visual ISO/IEC 14496-2 (c)
#define MOV_OBJECT_H264		0x21 // Visual ITU-T Recommendation H.264 | ISO/IEC 14496-10 
#define MOV_OBJECT_HEVC		0x23 // Visual ISO/IEC 23008-2 | ITU-T Recommendation H.265
#define MOV_OBJECT_AAC		0x40 // Audio ISO/IEC 14496-3
#define MOV_OBJECT_MP2V		0x60 // Visual ISO/IEC 13818-2 Simple Profile
#define MOV_OBJECT_AAC_MAIN	0x66 // MPEG-2 AAC Main
#define MOV_OBJECT_AAC_LOW	0x67 // MPEG-2 AAC Low
#define MOV_OBJECT_AAC_SSR	0x68 // MPEG-2 AAC SSR
#define MOV_OBJECT_MP3		0x69 // Audio ISO/IEC 13818-3
#define MOV_OBJECT_MP1V		0x6A // Visual ISO/IEC 11172-2
#define MOV_OBJECT_MP1A		0x6B // Audio ISO/IEC 11172-3
#define MOV_OBJECT_JPEG		0x6C // Visual ISO/IEC 10918-1 (JPEG)
#define MOV_OBJECT_PNG		0x6D // Portable Network Graphics (f)
#define MOV_OBJECT_JPEG2000	0x6E // Visual ISO/IEC 15444-1 (JPEG 2000)
#define MOV_OBJECT_VC1      0xA3 // SMPTE VC-1 Video
#define MOV_OBJECT_DIRAC    0xA4 // Dirac Video Coder
#define MOV_OBJECT_AC3      0xA5 // AC-3
#define MOV_OBJECT_EAC3     0xA5 // Enhanced AC-3
#define MOV_OBJECT_G719		0xA8 // ITU G.719 Audio
#define MOV_OBJECT_DTS      0xA9 // Core Substream
#define MOV_OBJECT_OPUS		0xAD // Opus audio
#define MOV_OBJECT_VP9      0xB1 // VP9 Video
#define MOV_OBJECT_FLAC     0xC1 // nonstandard from FFMPEG
#define MOV_OBJECT_VP8      0xC2 // nonstandard
#define MOV_OBJECT_H266		0xFC // ITU-T Recommendation H.266
#define MOV_OBJECT_G711a	0xFD // ITU G.711 alaw
#define MOV_OBJECT_G711u	0xFE // ITU G.711 ulaw
#define MOV_OBJECT_AV1		0xFF // AV1: https://aomediacodec.github.io/av1-isobmff

/// MOV flags
#define MOV_FLAG_FASTSTART	0x00000001
#define MOV_FLAG_SEGMENT	0x00000002 // fmp4_writer only

/// MOV av stream flag
#define MOV_AV_FLAG_KEYFREAME 0x0001

struct mov_ftyp_t
{
	uint32_t major_brand;
	uint32_t minor_version;

	uint32_t compatible_brands[N_BRAND];
	int brands_count;
};

// ISO/IEC 14496-12:2012(E) 4.2 Object Structure (16)
struct mov_box_t
{
	uint64_t size; // 0-size: box extends to end of file, 1-size: large size
	uint32_t type;

	// if 'uuid' == type
	//uint8_t usertype[16];

	// FullBox
	//uint32_t version : 8;
	//uint32_t flags : 24;
};

class MP4Demuxer;
struct mov_parse_t
{
	uint32_t type;
	uint32_t parent;
	std::function<int(const mov_box_t* box, MP4Demuxer* self)> parse;
};

// A.4 Temporal structure of the media (p148)
// The movie, and each track, has a timescale. 
// This defines a time axis which has a number of ticks per second
struct mov_mvhd_t
{
	// FullBox
	uint32_t version : 8;
	uint32_t flags : 24;

	uint32_t timescale; // time-scale for the entire presentation, the number of time units that pass in one second
	uint64_t duration; // default UINT64_MAX(by timescale)
	uint64_t creation_time; // seconds sine midnight, Jan. 1, 1904, UTC
	uint64_t modification_time; // seconds sine midnight, Jan. 1, 1904, UTC

	uint32_t rate;
	uint16_t volume; // fixed point 8.8 number, 1.0 (0x0100) is full volume
	//uint16_t reserved;
	//uint32_t reserved2[2];
	int32_t matrix[9]; // u,v,w
	//int32_t pre_defined[6];
	uint32_t next_track_ID;
};

struct mov_tkhd_t
{
	// FullBox
	uint32_t version : 8;
	uint32_t flags : 24; // MOV_TKHD_FLAG_XXX

	uint32_t track_ID; // cannot be zero
	uint64_t creation_time; // seconds sine midnight, Jan. 1, 1904, UTC
	uint64_t modification_time; // seconds sine midnight, Jan. 1, 1904, UTC
	uint64_t duration; // default UINT64_MAX(by Movie Header Box timescale)
	//uint32_t reserved;

	//uint32_t reserved2[2];
	int16_t layer;
	int16_t alternate_group;
	int16_t volume; // fixed point 8.8 number, 1.0 (0x0100) is full volume
	//uint16_t reserved;	
	int32_t matrix[9]; // u,v,w
	uint32_t width; // fixed-point 16.16 values
	uint32_t height; // fixed-point 16.16 values
};

struct mov_mdhd_t
{
	// FullBox
	uint32_t version : 8;
	uint32_t flags : 24;

	uint32_t timescale; // second
	uint64_t duration; // default UINT64_MAX(by timescale)
	uint64_t creation_time; // seconds sine midnight, Jan. 1, 1904, UTC
	uint64_t modification_time; // seconds sine midnight, Jan. 1, 1904, UTC

	uint32_t pad : 1;
	uint32_t language : 15;
	uint32_t pre_defined : 16;
};

struct mov_sample_entry_t
{
    uint16_t data_reference_index; // ref [dref] Data Reference Boxes
    uint8_t object_type_indication; // H.264/AAC MOV_OBJECT_XXX (DecoderConfigDescriptor)
    uint8_t stream_type; // MP4_STREAM_XXX
	std::string extra_data; // H.264 sps/pps
	int extra_data_size;

    union
    {
        struct mov_bitrate_t
        {
            uint32_t bufferSizeDB;
            uint32_t maxBitrate;
            uint32_t avgBitrate;
        } bitrate;

        //struct mov_uri_t
        //{
        //	char uri[256];
        //} uri;

        // visual
        struct mov_visual_sample_t
        {
            uint16_t width;
            uint16_t height;
            uint32_t horizresolution; // 0x00480000 - 72dpi
            uint32_t vertresolution; // 0x00480000 - 72dpi
            uint16_t frame_count; // default 1
            uint16_t depth; // 0x0018

			struct mov_pixel_aspect_ratio_t
			{
				uint32_t h_spacing;
				uint32_t v_spacing;
			} pasp;
        } visual;

        struct mov_audio_sample_t
        {
            uint16_t channelcount; // default 2
            uint16_t samplesize; // default 16
            uint32_t samplerate; // { default samplerate of media } << 16
        } audio;
    } u;
};

struct mov_stsd_t
{
	std::shared_ptr<mov_sample_entry_t> current; // current entry, read only
    std::vector<std::shared_ptr<mov_sample_entry_t>> entries;
    uint32_t entry_count;
};

struct mov_stts_t
{
	uint32_t sample_count;
	uint32_t sample_delta; // in the time-scale of the media
};

struct mov_stsc_t
{
	uint32_t first_chunk;
	uint32_t samples_per_chunk;
	uint32_t sample_description_index;
};

struct mov_elst_t
{
	uint64_t segment_duration; // by Movie Header Box timescale
	int64_t media_time;
	int16_t media_rate_integer;
	int16_t media_rate_fraction;
};

struct mov_trex_t
{
//	uint32_t track_ID;
	uint32_t default_sample_description_index;
	uint32_t default_sample_duration;
	uint32_t default_sample_size;
	uint32_t default_sample_flags; 
};

struct mov_tfhd_t
{
	uint32_t flags;
//	uint32_t track_ID;
	uint64_t base_data_offset;
	uint32_t sample_description_index;
	uint32_t default_sample_duration;
	uint32_t default_sample_size;
	uint32_t default_sample_flags;
};

struct mov_stbl_t
{
	std::vector<std::shared_ptr<mov_stsc_t>> stsc;
	size_t stsc_count = 0;

	std::vector<uint64_t> stco;
	uint32_t stco_count = 0;

	std::vector<std::shared_ptr<mov_stts_t>> stts;
	size_t stts_count = 0;

	std::vector<std::shared_ptr<mov_stts_t>> ctts;
	size_t ctts_count = 0;

	std::vector<uint32_t> stss;
	size_t stss_count = 0;
};

struct mov_sample_t
{
	int flags; // MOV_AV_FLAG_KEYFREAME
	int64_t pts; // track mdhd timescale
	int64_t dts;

	void* data;
	uint64_t offset; // is a 32 or 64 bit integer that gives the offset of the start of a chunk into its containing media file.
	uint32_t bytes;

	uint32_t sample_description_index;
	uint32_t samples_per_chunk; // write only
	uint32_t first_chunk; // write only
};

struct mov_fragment_t
{
	uint64_t time;
	uint64_t offset; // moof offset
};

class mov_track_t
{
public:
	mov_track_t() {}
	~mov_track_t();

	uint32_t tag; // MOV_H264/MOV_MP4A
	uint32_t handler_type; // MOV_VIDEO/MOV_AUDIO
	const char* handler_descr; // VideoHandler/SoundHandler/SubtitleHandler

	struct mov_tkhd_t tkhd;
	struct mov_mdhd_t mdhd;
	struct mov_stbl_t stbl;

	// 8.8 Movie Fragments
	struct mov_trex_t trex;
	struct mov_tfhd_t tfhd;
	std::vector<std::shared_ptr<mov_fragment_t>> frags;
	uint32_t frag_count, frag_capacity;

	struct mov_stsd_t stsd;

	std::vector<std::shared_ptr<mov_elst_t>> elst;
	size_t elst_count = 0;
	
	std::vector<std::shared_ptr<mov_sample_t>> samples;
	uint32_t sample_count = 0;
	size_t sample_offset; // sample_capacity

    int64_t tfdt_dts; // tfdt baseMediaDecodeTime
    int64_t start_dts; // write fmp4 only
    uint64_t offset; // write only
    int64_t last_dts; // write fmp4 only
    int64_t turn_last_duration; // write fmp4 only

	unsigned int flags;
};

struct mov_object_tag {
	uint8_t id;
	uint32_t tag;
};

uint32_t mov_object_to_tag(uint8_t object);
uint8_t mov_tag_to_object(uint32_t tag);

#endif //MP4Box_H

