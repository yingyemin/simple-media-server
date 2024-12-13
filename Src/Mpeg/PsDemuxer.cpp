#include "PsDemuxer.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Log/Logger.h"
#include "Codec/AacTrack.h"
#include "Codec/Mp3Track.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"
#include "Mpeg.h"
#include "EventPoller/EventLoop.h"

#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

//#define W_PS_FILE
//#define W_VIDEO_FILE
//#define W_AUDIO_FILE
//#define W_UNKONW_FILE

static bool parseAacFrame(const unsigned char* buf, int len)
{
    // unsigned char buf[10 * 1024] = {0};
    // int n = fread(buf, 1, 7, fp);
    // if (n < 7) {
    //     logWarn << "size < 7" << endl;
    //     return false;
    // }
    // avs->read_bytes((char*)buf, 7);
    if (buf == nullptr || len < 7) {
        logWarn << "Invalid input: buf is null or len < 7" << endl;
        return false;
    }

    int frame_size = 0;
    if((buf[0] == 0xff) && ((buf[1] & 0xff) == 0xf1)){
        frame_size |= ((buf[3] & 0x03) << 11);	// high 2bit
        frame_size |= buf[4] << 3;				// middle 8bit
        frame_size |= ((buf[5] & 0xe0) >> 5);	// low 3bit
    } else {
        logWarn << "it is not a aac frame" << endl;
        printf("%x %x %x %x %x %x %x\n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
        return false;
    }
    logInfo << "frame_size is: " << frame_size << endl;
    if (frame_size <= 0 || frame_size > len) {
        logWarn << "Invalid frame size: " << frame_size << endl;
        return false;
    }
    // n = fread(buf + 7, 1, frame_size - 7, fp);
    // if (n < frame_size - 7) {
    //     logWarn << "size < " << frame_size << endl;
    //     return false;
    // }
    // avs->read_bytes((char*)(buf + 7), frame_size - 7);

    // 解析 Profile	
    char profile_str[10] = {0};
    char frequence_str[10] = {0}; 

    unsigned char profile = buf[2] & 0xC0;
    profile = profile>>6;
    
    switch(profile){
        case 0: sprintf(profile_str,"Main");break;
        case 1: sprintf(profile_str,"LC");break;
        case 2: sprintf(profile_str,"SSR");break;
        default:sprintf(profile_str,"unknown");
                printf("%x %x %x %x %x %x %x\n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
                break;
    }

    int channel = (buf[2] & 0x01) << 2;
    channel |= (buf[3] & 0xC0) >> 6;
    
    // 解析 frequence
    unsigned char sampling_frequency_index = buf[2] & 0x3C;
    sampling_frequency_index = sampling_frequency_index >> 2;
    switch(sampling_frequency_index){
        case 0: sprintf(frequence_str,"96000Hz");break;
        case 1: sprintf(frequence_str,"88200Hz");break;
        case 2: sprintf(frequence_str,"64000Hz");break;
        case 3: sprintf(frequence_str,"48000Hz");break;
        case 4: sprintf(frequence_str,"44100Hz");break;
        case 5: sprintf(frequence_str,"32000Hz");break;
        case 6: sprintf(frequence_str,"24000Hz");break;
        case 7: sprintf(frequence_str,"22050Hz");break;
        case 8: sprintf(frequence_str,"16000Hz");break;
        case 9: sprintf(frequence_str,"12000Hz");break;
        case 10:sprintf(frequence_str,"11025Hz");break;
        case 11:sprintf(frequence_str,"8000Hz")	;break;
        default:sprintf(frequence_str,"unknown");
                printf("frequence_str: %x %x %x %x %x %x %x\n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
                break;
    }
    
    printf("%8s | %8s | %5d | %5d\n",profile_str, frequence_str, channel, frame_size);

    return true;
}

PsDemuxer::PsDemuxer()
{
    
}

uint64_t PsDemuxer::parsePsTimestamp(const uint8_t* p)
{
    if (p == nullptr) {
        return 0;
    }

	unsigned long byte;
	//total 33 bits
	unsigned long part1, part2, part3;

	//1st byte, 5、6、7 bit
	byte = *p++;
	part1 = (byte & 0x0e);

	//2 byte, all bit 
	byte = (*(p++)) << 8;
    //3 bytes 1--7 bit
	byte += *(p++);
	part2 = (byte & 0xfffe) >> 1;
	
	//4 byte, all bit
	byte = (*(p++)) << 8;
    //5 byte 1--7 bit
	byte += *(p++);
	part3 = (byte & 0xfffe) >> 1;

    //<32--part1--30> <29----part2----15> <14----part3----0>
	uint64_t timestamp  = (part1  << 29) | (part2 << 15) | part3;
	return timestamp ;
}

// TODO 需要支持将剩下的buffer保存下来，下次拼接使用
int PsDemuxer::onPsStream(char* ps_data, int ps_size, uint32_t timestamp, uint32_t ssrc)
{
    int err = 0;
    int complete_len = 0;
    int incomplete_len = ps_size;
    char *next_ps_pack = ps_data;

    if (!_remainBuffer.empty()) {
        _remainBuffer.append(ps_data, ps_size);
        incomplete_len = _remainBuffer.size();
        next_ps_pack = _remainBuffer.data();
        ps_size = incomplete_len;
    }

    if (!next_ps_pack || incomplete_len == 0) {
        return -1;
    }
    // logInfo << "incomplete_len: " << incomplete_len;
    char* end = next_ps_pack + incomplete_len;

    // SrsSimpleStream video_stream;
    // SrsSimpleStream audio_stream;
    // StringBuffer video_stream;
    uint64_t audio_pts = 0;
    uint64_t video_pts = 0;
    int pse_index = 0;
    bool hasAudio = false;

    uint8_t audio_es_id = 0;
    //static uint8_t audio_es_type = 0;
    uint8_t video_es_id = 0;
    //static uint8_t video_es_type = 0;

// #ifdef W_PS_FILE           
//         if (!ps_fw.is_open()) {
//                 std::string filename = "test_ps_" + channel_id + ".mpg";
//                 ps_fw.open(filename.c_str());
//         }
//         ps_fw.write(ps_data, ps_size, NULL);          
// #endif

	while(incomplete_len >= sizeof(PsPacketStartCode))
    {
    	if (next_ps_pack
			&& next_ps_pack[0] == (char)0x00
			&& next_ps_pack[1] == (char)0x00
			&& next_ps_pack[2] == (char)0x01
			&& next_ps_pack[3] == (char)0xBA)
		{
            //ps header 
            // logInfo << "get a ps ba";
            if (incomplete_len < sizeof(PsPacketHeader)) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
            PsPacketHeader *head = (PsPacketHeader *)next_ps_pack;
            uint8_t stuffingLength = head->stuffingLength & 0x07;

            // logInfo << "incomplete_len: " << incomplete_len;
            // logInfo << "end - next_ps_pack: " << (end - next_ps_pack);
            // logInfo << "need_len: " << (sizeof(PsPacketHeader) + stuffingLength);
            if (incomplete_len < sizeof(PsPacketHeader) + stuffingLength) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
        
            next_ps_pack = next_ps_pack + sizeof(PsPacketHeader) + stuffingLength;
            complete_len = complete_len + sizeof(PsPacketHeader) + stuffingLength;
            incomplete_len = ps_size - complete_len;
        }
        else if(next_ps_pack
			&& next_ps_pack[0] == (char)0x00
			&& next_ps_pack[1] == (char)0x00
			&& next_ps_pack[2] == (char)0x01
			&& next_ps_pack[3] == (char)0xBB)
        {
            // logInfo << "get a ps bb";
            //ps system header 
            if (incomplete_len < sizeof(PsPacketBBHeader)) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
            PsPacketBBHeader *bbhead=(PsPacketBBHeader *)(next_ps_pack);
            int bbheaderlen = htons(bbhead->length);

            if (incomplete_len < sizeof(PsPacketBBHeader) + bbheaderlen) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
            next_ps_pack = next_ps_pack + sizeof(PsPacketBBHeader) + bbheaderlen;
            complete_len = complete_len + sizeof(PsPacketBBHeader) + bbheaderlen;
            incomplete_len = ps_size - complete_len;

            // first_keyframe_flag = true;
        }
        else if(next_ps_pack
			&& next_ps_pack[0] == (char)0x00
			&& next_ps_pack[1] == (char)0x00
			&& next_ps_pack[2] == (char)0x01
			&& next_ps_pack[3] == (char)0xBC)
        {
            //program stream map 
            // logInfo << "get a ps bc";
            
            if (incomplete_len < sizeof(PsMapPacket)) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }

		    PsMapPacket* psmap_pack = (PsMapPacket*)next_ps_pack;
            int psmLength = htons(psmap_pack->length);

            if (incomplete_len < sizeof(PsPacketBBHeader) + psmLength) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
          
            next_ps_pack = next_ps_pack + psmLength + sizeof(PsMapPacket);
            complete_len = complete_len + psmLength + sizeof(PsMapPacket);
            incomplete_len = ps_size - complete_len;

            //parse ps map
            uint16_t psm_length=0, ps_info_length=0, es_map_length=0;
            char *buf = (char*)psmap_pack + sizeof(PsMapPacket);

            // SrsBuffer buf(p, (int)psmLength);

            psm_length =(int)psmLength;
            // buf.read_1bytes();
            // buf.read_1bytes();
            buf += 2;

            ps_info_length = buf[0] << 8 | buf[1];
            buf += 2;

            /* skip program_stream_info */
            // buf.skip(ps_info_length);
            buf += ps_info_length;


            // /*es_map_length = */buf.read_2bytes();
            buf += 2;
            /* Ignore es_map_length, trust psm_length */
            es_map_length = psm_length - ps_info_length - 10;
        
            // /* at least one es available? */
            while (es_map_length >= 4) {
                uint8_t type      = buf[0];
                uint8_t es_id     = buf[1];
                buf += 2;
                uint16_t es_info_length = buf[0] << 8 | buf[1];
                buf += 2;
                // std::string s_type = get_ps_map_type_str(type);

                // logInfo << "es_id: " << (int)es_id;

                /* remember mapping from stream id to stream type */
                if (es_id >= PS_AUDIO_ID && es_id <= PS_AUDIO_ID_END){
                    _hasAudio = true;
                    if (_audio_es_type != type){
                        // srs_trace("gb28181: ps map audio es_type=%s(%x), es_id=%0x, es_info_length=%d", 
                        //  s_type.c_str(), type, es_id, es_info_length);
                    }
                    
                    // logInfo << "get a ps audio map";
                    audio_es_id = es_id;
                    _audio_es_type = type;
                    if (_audio_es_type == STREAM_TYPE_AUDIO_AAC) {
                        _audioCodec = "aac";
                        // auto trackInfo = make_shared<AacTrack>();
                        // trackInfo->index_ = AudioTrackType;
                        // trackInfo->codec_ = "aac";
                        // trackInfo->trackType_ = "audio";
                        // trackInfo->samplerate_ = 90000;
                        // trackInfo->payloadType_ = 97;
                        auto trackInfo = AacTrack::createTrack(AudioTrackType, 97, 44100);
                        addTrackInfo(trackInfo);
                    } else if (_audio_es_type == STREAM_TYPE_AUDIO_G711) {
                        _audioCodec = "g711a";
                        // auto trackInfo = make_shared<G711aTrack>();
                        // trackInfo->index_ = AudioTrackType;
                        // trackInfo->codec_ = "g711a";
                        // trackInfo->trackType_ = "audio";
                        auto trackInfo = G711aTrack::createTrack(AudioTrackType, 8, 8000);
                        addTrackInfo(trackInfo);
                        _firstAac = false;
                    } else if (_audio_es_type == STREAM_TYPE_AUDIO_G711ULAW) {
                        _audioCodec = "g711u";
                        // auto trackInfo = make_shared<G711uTrack>();
                        // trackInfo->index_ = AudioTrackType;
                        // trackInfo->codec_ = "g711u";
                        // trackInfo->trackType_ = "audio";
                        auto trackInfo = G711uTrack::createTrack(AudioTrackType, 0, 8000);
                        addTrackInfo(trackInfo);
                        _firstAac = false;
                    } else if (_audio_es_type == STREAM_TYPE_AUDIO_G711ULAW) {
                        _audioCodec = "mp3";
                        auto trackInfo = Mp3Track::createTrack(AudioTrackType, 14, 44100);
                        addTrackInfo(trackInfo);
                        _firstAac = false;
                    }
                }else if (es_id >= PS_VIDEO_ID && es_id <= PS_VIDEO_ID_END){
                    _hasVideo = true;
                    if (_video_es_type != type){
                        // srs_trace("gb28181: ps map video es_type=%s(%x), es_id=%0x, es_info_length=%d", 
                        //  s_type.c_str(), type, es_id, es_info_length);
                    }
                    // logInfo << "get a ps video map";

                    video_es_id = es_id;
                    _video_es_type = type;
                    if (_video_es_type == STREAM_TYPE_VIDEO_H264) {
                        _videoCodec = "h264";
                        auto trackInfo = H264Track::createTrack(VideoTrackType, 96, 90000);
                        addTrackInfo(trackInfo);
                        _firstVps = false;
                    } else if (_video_es_type == STREAM_TYPE_VIDEO_HEVC) {
                        _videoCodec = "h265";
                        // auto trackInfo = make_shared<H265Track>();
                        // trackInfo->index_ = VideoTrackType;
                        // trackInfo->codec_ = "h265";
                        // trackInfo->trackType_ = "video";
                        // trackInfo->samplerate_ = 90000;
                        // trackInfo->payloadType_ = 96;
                        auto trackInfo = H265Track::createTrack(VideoTrackType, 96, 90000);
                        addTrackInfo(trackInfo);
                    }
                }
           
                /* skip program_stream_info */
                // buf.skip(es_info_length);
                buf += es_info_length;
                es_map_length -= 4 + es_info_length;
                if (_audio_es_type == 0) {
                    _firstAac = false;
                }
            }
    
        }
        else if(next_ps_pack
			&& next_ps_pack[0] == (char)0x00
			&& next_ps_pack[1] == (char)0x00
			&& next_ps_pack[2] == (char)0x01
			&& next_ps_pack[3] == (char)0xE0)
        {
            _hasVideo = true;
            if (!_videoFrame) {
                _videoFrame = createFrame(VideoTrackType);
            }
            //pse video stream
            if (incomplete_len < sizeof(PsePacket)) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
            // logInfo << "get a ps video frame";

            PsePacket* pse_pack = (PsePacket*)next_ps_pack;

            int packlength = htons(pse_pack->length);
            int payloadlen = packlength - 2 - 1 - pse_pack->stuffingLength;
            if (payloadlen <= 0) {
                logInfo << "payloadlen <= 0";
                return -1;
            }

            // logInfo << "incomplete_len: " << incomplete_len;
            // logInfo << "end - next_ps_pack: " << (end - next_ps_pack);
            // logInfo << "need_len: " << (sizeof(PsePacket) + pse_pack->stuffingLength + payloadlen);
            if (incomplete_len < sizeof(PsePacket) + pse_pack->stuffingLength + payloadlen) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }

            if (incomplete_len < 19) {
                return -1;
            }

            unsigned char pts_dts_flags = (pse_pack->info[1] & 0xF0) >> 6;
            //in a frame of data, pts is obtained from the first PSE packet
            if (/*pse_index == 0 && */pts_dts_flags > 0) {
				video_pts = parsePsTimestamp((unsigned char*)next_ps_pack + 9);
                // srs_info("gb28181: ps stream video ts=%u pkt_ts=%u", video_pts, timestamp);
                if (pts_dts_flags == 3) {
                    auto dts = parsePsTimestamp((unsigned char*)next_ps_pack + 14);
                    logInfo << "get a video dts: " << dts << endl;
                }
			}
            // pse_index +=1;
         
            next_ps_pack = next_ps_pack + 9 + pse_pack->stuffingLength;
            complete_len = complete_len + 9 + pse_pack->stuffingLength;

            
            // logError << "payloadlen : " << payloadlen;
            if (video_pts == 0) {
                video_pts = _lastVideoPts;
            }
            
            // logInfo << "_lastVideoPts: " << _lastVideoPts << ", video_pts: " << video_pts;
            if (_lastVideoPts != -1 && _lastVideoPts != video_pts && _videoFrame) {
                // onDecode(_videoStream);
                if (_videoCodec != "unknown" && _videoFrame->size() > 0) {
                    // static int i = 0;
                    // string name = "testpsvod" + to_string(i++) + ".h264";
                    // FILE* fp = fopen(name.c_str(), "ab+");
                    // fwrite(_videoStream.data(), 1, _videoStream.size(), fp);
                    // fclose(fp);

                    // logInfo << "decode a frame";
                    onDecode(_videoFrame, VideoTrackType, video_pts, video_pts);
                }
                _videoFrame = createFrame(VideoTrackType);
            }
            
            if (_videoFrame) {
                if (_videoFrame->_buffer.empty()) {
                    _videoFrame->_buffer.assign(next_ps_pack, payloadlen);
                } else {
                    _videoFrame->_buffer.append(next_ps_pack, payloadlen);
                }
            }
            _lastVideoPts = video_pts;

            // logInfo << "ps_size: " << ps_size;
            // logInfo << "payloadlen: " << payloadlen;
            // logInfo << "complete_len: " << complete_len;

            next_ps_pack = next_ps_pack + payloadlen;
            complete_len = complete_len + payloadlen;
            incomplete_len = ps_size - complete_len;
        }
     	else if (next_ps_pack
			&& next_ps_pack[0] == (char)0x00
			&& next_ps_pack[1] == (char)0x00
			&& next_ps_pack[2] == (char)0x01
			&& next_ps_pack[3] == (char)0xBD)
        {
            //private stream 
            if (incomplete_len < sizeof(PsePacket)) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }

			PsePacket* pse_pack = (PsePacket*)next_ps_pack;
			
            int packlength = htons(pse_pack->length);
			int payload_len = packlength - 2 - 1 - pse_pack->stuffingLength;

            logInfo << "incomplete_len: " << incomplete_len;
            logInfo << "need_len: " << (sizeof(PsePacket) + pse_pack->stuffingLength + payload_len);
            if (incomplete_len < sizeof(PsePacket) + pse_pack->stuffingLength + payload_len) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
            
			next_ps_pack = next_ps_pack + payload_len + 9 + pse_pack->stuffingLength;
            complete_len = complete_len + (payload_len + 9 + pse_pack->stuffingLength);
            incomplete_len = ps_size - complete_len;
		}
		else if (next_ps_pack
			&& next_ps_pack[0] == (char)0x00
			&& next_ps_pack[1] == (char)0x00
			&& next_ps_pack[2] == (char)0x01
			&& next_ps_pack[3] == (char)0xC0)
        {
            _hasAudio = true;
            //audio stream
            if (incomplete_len < sizeof(PsePacket)) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }
            // logInfo << "get a ps audio frame";

            FrameBuffer::Ptr audio_stream = createFrame(AudioTrackType);
            PsePacket* pse_pack = (PsePacket*)next_ps_pack;

			int packlength = htons(pse_pack->length);
			int payload_len = packlength - 2 - 1 - pse_pack->stuffingLength;

            if (payload_len <= 0) {
                return -1;
            }

            // logInfo << "incomplete_len: " << incomplete_len;
            // logInfo << "payload_len: " << payload_len;
            // logInfo << "packlength: " << packlength;
            // logInfo << "sizeof(PsePacket): " << sizeof(PsePacket);
            // logInfo << "end - next_ps_pack: " << (end - next_ps_pack);
            // logInfo << "need_len: " << (sizeof(PsePacket) + pse_pack->stuffingLength + payload_len);
            if (incomplete_len < sizeof(PsePacket) + pse_pack->stuffingLength + payload_len) {
                _remainBuffer.assign(next_ps_pack, incomplete_len);
                logInfo << "_remainBuffer size: " << _remainBuffer.size();
                return -1;
            }

            if (incomplete_len < 19) {
                return -1;
            }

		    unsigned char pts_dts_flags = (pse_pack->info[1] & 0xF0) >> 6;
			if (pts_dts_flags > 0 ) {
				audio_pts = parsePsTimestamp((unsigned char*)next_ps_pack + 9);
                // srs_info("gb28181: ps stream video ts=%u pkt_ts=%u", audio_pts, timestamp);
                if (pts_dts_flags == 3) {
                    auto dts = parsePsTimestamp((unsigned char*)next_ps_pack + 14);
                    logInfo << "get a audio dts: " << dts << endl;
                }
         	}

            next_ps_pack = next_ps_pack + 9 + pse_pack->stuffingLength;
            if (payload_len > 4) {

                //if ps map is not aac, but stream  many be aac adts , try update type, 
                //TODO: dahua audio ps map type always is 0x90(g711)

                uint8_t p1 = (uint8_t)(next_ps_pack[0]);
                uint8_t p2 = (uint8_t)(next_ps_pack[1]);
                uint8_t p3 = (uint8_t)(next_ps_pack[2]);
                uint8_t p4 = (uint8_t)(next_ps_pack[3]);

                if (_audio_es_type != STREAM_TYPE_AUDIO_AAC &&
                    (p1 & 0xFF) == 0xFF &&  (p2 & 0xF0) == 0xF0) {
                    
                    //try update aac type
                    // SrsBuffer avs(next_ps_pack, payload_len);
                    char* frame = NULL;
                    int frame_size = 0;

                    if (!parseAacFrame((unsigned char*)next_ps_pack, payload_len)) {
                        // srs_info("gb28181: client_id %s, audio data not aac adts (%#x/%u) %02x %02x %02x %02x\n", 
                        //          channel_id.c_str(), ssrc, timestamp, p1, p2, p3, p4);  
                        // srs_error_reset(err);
                        logInfo << "parse aac error, it is not a aac" << endl;
                    }else{
                        // srs_warn("gb28181: client_id %s, ps map is not aac (%s) type, but stream many be aac adts, try update type",
                        //      channel_id.c_str(), get_ps_map_type_str(audio_es_type).c_str());
                        logWarn << "audio id is: " << (int)_audio_es_type << ", but it is a aac" << endl;
                        _audio_es_type = STREAM_TYPE_AUDIO_AAC;
                        _audioCodec = "aac";
                        auto trackInfo = make_shared<AacTrack>();
                        trackInfo->index_ = AudioTrackType;
                        trackInfo->codec_ = "aac";
                        trackInfo->trackType_ = "audio";
                        trackInfo->samplerate_ = 90000;
                        trackInfo->payloadType_ = 97;
                        addTrackInfo(trackInfo);
                    }
                }
            }
         
            audio_stream->_buffer.append(next_ps_pack, payload_len);
            
// #ifdef W_AUDIO_FILE            
//             if (!audio_fw.is_open()) {
//                  std::string filename = "test_audio_" + channel_id + ".aac";
//                  audio_fw.open(filename.c_str());
//             }
//             audio_fw.write(next_ps_pack,  payload_len, NULL);          
// #endif
            
			next_ps_pack = next_ps_pack + payload_len;
            complete_len = complete_len + (payload_len + 9 + pse_pack->stuffingLength);
            incomplete_len = ps_size - complete_len;

            hasAudio = true;
            //Buffer::Ptr audio_frame = std::make_shared<BufferOffset<StringBuffer> >(std::move(audio_stream));
            // _decoder->_on_decode(0, _audio_es_type, 1, audio_pts, audio_pts, audio_stream.data(), audio_stream.size());
            if (_audioCodec != "unknown") {
                onDecode(audio_stream, AudioTrackType, audio_pts, audio_pts);
            }

            // if (hander && audio_enable && audio_stream.length() && can_send_ps_av_packet()) {
            //     if ((err = hander->on_rtp_audio(&audio_stream, audio_pts, audio_es_type)) != srs_success) {
            //         return srs_error_wrap(err, "process ps audio packet");
            //     }
            // }
		}
        else
        {
// #ifdef W_UNKONW_FILE            
//             if (!unknow_fw.is_open()) {
//                  std::string filename = "test_unknow_" + channel_id + ".mpg";
//                  unknow_fw.open(filename.c_str());
//             }
//             unknow_fw.write(next_ps_pack,  incomplete_len, NULL);          
// #endif      
            //TODO: fixme unkonw ps data parse
            // if (next_ps_pack
            // && next_ps_pack[0] == (char)0x00
			// && next_ps_pack[1] == (char)0x00
			// && next_ps_pack[2] == (char)0x00
			// && next_ps_pack[3] == (char)0x01){
            //     //dahua's PS header may lose packets. It is sent by an RTP packet of Dahua's PS header
            //     //dahua rtp send format:
            //     //ts=1000 seq=1 mark=false payload= ps header
            //     //ts=1000 seq=2 mark=false payload= video
            //     //ts=1000 seq=3 mark=true payload= video
            //     //ts=1000 seq=4 mark=true payload= audio
            //     incomplete_len = ps_size - complete_len; 
            //     complete_len = complete_len + incomplete_len;
            // }
            // _remainBuffer.clear();

            // first_keyframe_flag = false;
            // srs_trace("gb28181: client_id %s, unkonw ps data (%#x/%u) %02x %02x %02x %02x\n", 
            //     channel_id.c_str(), ssrc, timestamp,  
            //     next_ps_pack[0]&0xFF, next_ps_pack[1]&0xFF, next_ps_pack[2]&0xFF, next_ps_pack[3]&0xFF);
            // break;

            logInfo << "invalid ps packet, find next ps packet";

            next_ps_pack = next_ps_pack + 1;
            complete_len = complete_len + 1;
            incomplete_len = ps_size - complete_len;
        }
    }

    if (end - next_ps_pack > 0) {
        logInfo << "remain size: " << incomplete_len << "|" << (end - next_ps_pack);
        _remainBuffer.assign(next_ps_pack, end - next_ps_pack);
    } else if (!_remainBuffer.empty()){
        _remainBuffer.clear();
    }

    // if (complete_len != ps_size){
    //     logWarn << "decode ps packet error: ssrc ==> " << ssrc << "; timastamp ==> " << timestamp
    //           << "; complete_len ==> " << complete_len << "; ps_size ==> " << ps_size << endl;
    // } else if (video_stream.size() && (_video_es_type == STREAM_TYPE_VIDEO_H264 || _video_es_type == STREAM_TYPE_VIDEO_HEVC)) {
    //     //Buffer::Ptr video_frame = std::make_shared<BufferOffset<StringBuffer> >(std::move(video_stream));
    //     // _decoder->_on_decode(0, _video_es_type, 1, video_pts, video_pts, video_stream.data(), video_stream.size());
    // } else if (video_stream.size()) {
    //     logWarn << "data size: " << video_stream.size() << "; video type: " << (int)_video_es_type << endl;
    // } else if (!hasAudio) {
    //     logWarn << "have not decode any data" << endl;
    // }
  
    return 0;
}

FrameBuffer::Ptr PsDemuxer::createFrame(int index)
{
    FrameBuffer::Ptr frame;
    if (index == VideoTrackType) {
        if (_videoCodec == "h264") {
            frame = make_shared<H264Frame>();
        } else if (_videoCodec == "h265") {
            frame = make_shared<H265Frame>();
        } else {
            logWarn << "Unsupported video codec: " << _videoCodec;
            return frame;
        }

        frame->_trackType = VideoTrackType;
        _videoFrame = frame;
    } else {
        frame = make_shared<FrameBuffer>();
        frame->_trackType = AudioTrackType;
    }

    return frame;
}

void PsDemuxer::onDecode(const FrameBuffer::Ptr& frame, int index, uint64_t pts, uint64_t dts)
{
    if (!frame) {
        return ;
    }
    // if (_firstAac && _audioCodec == "aac" && frame->getTrackType() == AudioTrackType)
    // {
    //     if (_mapTrackInfo.find(AudioTrackType) != _mapTrackInfo.end()) {
    //         auto aacTrack = dynamic_pointer_cast<AacTrack>(_mapTrackInfo[AudioTrackType]);
    //         aacTrack->setAacInfoByAdts(frame->data(), 7);
    //         _firstAac = false;
    //         onReady(AudioTrackType);
    //     }
    // } else if (!_audioCodec.empty() && frame->getTrackType() == AudioTrackType) {
    //     onReady(AudioTrackType);
    // }

    // FrameBuffer::Ptr frame = createFrame(index);
    // frame->_buffer.assign(data.data(), data.size());
    frame->_pts = pts > 0 ? pts / 90 : 0; // pts * 1000 / 90000,计算为毫秒
    frame->_dts = dts > 0 ? dts / 90 : 0;
    frame->_index = index;
    frame->_trackType = index;
    frame->_codec = index == VideoTrackType ? _videoCodec : _audioCodec;

    if (index == AudioTrackType && _audioCodec == "aac") {
        frame->_startSize = 7;
    } else if (index == VideoTrackType && (_videoCodec == "h264" || _videoCodec == "h265")) {
        frame->_startSize = 0;
        if (readUint32BE(frame->data()) == 1) {
            frame->_startSize = 4;
        } else if (readUint24BE(frame->data()) == 1) {
            frame->_startSize = 3;
        }
        // logInfo << "frame->_startSize: " << frame->_startSize;
    }
    if (index == VideoTrackType) {
        if (_videoCodec == "h265") {
            auto h265frame = dynamic_pointer_cast<H265Frame>(frame);
            h265frame->split([this, h265frame](const FrameBuffer::Ptr &subFrame){
                // if (_firstVps || _firstSps || _firstPps) {
                //     auto h265Track = dynamic_pointer_cast<H265Track>(_mapTrackInfo[VideoTrackType]);
                //     auto nalType = subFrame->getNalType();
                //     switch (nalType)
                //     {
                //     case H265_VPS:
                //         h265Track->setVps(subFrame);
                //         _firstVps = false;
                //         break;
                //     case H265_SPS:
                //         h265Track->setSps(subFrame);
                //         _firstSps = false;
                //         break;
                //     case H265_PPS:
                //         h265Track->setPps(subFrame);
                //         _firstPps = false;
                //         break;
                //     default:
                //         break;
                //     }
                // }else {
                //     logInfo << "gb28181 _onReady";
                //     onReady(VideoTrackType);
                // }
                if (_onFrame) {
                    _onFrame(subFrame);
                }
            });
        } else if (_videoCodec == "h264") {
            auto h264frame = dynamic_pointer_cast<H264Frame>(frame);
            h264frame->split([this, h264frame](const FrameBuffer::Ptr &subFrame){
                // if (_firstSps || _firstPps) {
                //     auto h264Track = dynamic_pointer_cast<H264Track>(_mapTrackInfo[VideoTrackType]);
                //     auto nalType = subFrame->getNalType();
                //     logInfo << "get a h264 nal type: " << (int)nalType;
                //     switch (nalType)
                //     {
                //     case H264_SPS:
                //         h264Track->setSps(subFrame);
                //         _firstSps = false;
                //         break;
                //     case H264_PPS:
                //         h264Track->setPps(subFrame);
                //         _firstPps = false;
                //         break;
                //     default:
                //         break;
                //     }
                // } else {
                //     // logInfo << "gb28181 _onReady";
                //     onReady(VideoTrackType);
                // }
                if (_onFrame) {
                    _onFrame(subFrame);
                }
            });
        }
        return ;
    }

    if (_onFrame) {
        _onFrame(frame);
    }
}

void PsDemuxer::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void PsDemuxer::addTrackInfo(const shared_ptr<TrackInfo>& trackInfo)
{
    if (!trackInfo) {
        return ;
    }

    auto it = _mapTrackInfo.find(trackInfo->index_);
    if (it == _mapTrackInfo.end()) {
        _mapTrackInfo[trackInfo->index_] = trackInfo;
        
        if (_onTrackInfo) {
            _onTrackInfo(trackInfo);
        }

        // if ((_hasVideo ? _videoCodec != "" : true) && 
        //     (_hasAudio ? _audioCodec != "" : true) && 
        //     _onReady && _audioCodec != "aac") {
        //     _onReady();
        // }
    } else if (it->second->codec_ != trackInfo->codec_) {
        _mapTrackInfo[trackInfo->index_] = trackInfo;
        // _resetTrackInfo(trackInfo);
    }
}

void PsDemuxer::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void PsDemuxer::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void PsDemuxer::onReady(int trackType)
{
    // if (_hasReady) {
    //     return ;
    // }
    // if (trackType == VideoTrackType) {
    //     _hasVideoReady = true;
    //     if ((_hasAudioReady || !_hasAudio) && _onReady) {
    //         _onReady();
    //         _hasReady = true;
    //     }
    // } else if (trackType == AudioTrackType) {
    //     _hasAudioReady = true;
    //     if ((_hasVideoReady || !_hasVideo) && _onReady) {
    //         _onReady();
    //         _hasReady = true;
    //     }
    // }
}

void PsDemuxer::clear()
{
    _remainBuffer.clear();
    _videoStream.clear();
}
