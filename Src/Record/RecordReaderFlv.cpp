#ifdef ENABLE_RTMP

#include "RecordReaderFlv.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.hpp"
#include "Common/Define.h"
#include "Rtmp/Amf.h"
#include "Rtmp/Rtmp.h"

#define FLV_HEADER_SIZE		9 // DataOffset included
#define FLV_TAG_HEADER_SIZE	11 // StreamID included

// FLV Tag Type
#define FLV_TYPE_AUDIO		8
#define FLV_TYPE_VIDEO		9
#define FLV_TYPE_SCRIPT		18

using namespace std;

RecordReaderFlv::RecordReaderFlv(const string& path)
    :RecordReader(path)
{
    // 去掉第一层目录
    auto tmpPath = path.substr(path.find_first_of("/", 1) + 1);
    // 去掉第二层目录
    tmpPath = tmpPath.substr(tmpPath.find_first_of("/", 1) + 1);
    // 找到最后一层目录的位置
    int pos = tmpPath.find_last_of("/");
    // 获取点播文件的路径
    _filePath = tmpPath.substr(0, pos);
    // 获取循环次数
    _loopCount = stoi(tmpPath.substr(pos + 1));
}

RecordReaderFlv::~RecordReaderFlv()
{
    
}

bool RecordReaderFlv::start()
{
    RecordReader::start();
    firstMediaTagLen = 0;
    weak_ptr<RecordReaderFlv> wSelf = dynamic_pointer_cast<RecordReaderFlv>(shared_from_this());
    static string rootPath = Config::instance()->getAndListen([](const json& config) {
        rootPath = Config::instance()->get("Record", "rootPath");
        }, "Record", "rootPath");

    string ext = _filePath.substr(_filePath.rfind('.') + 1);
    auto abpath = File::absolutePath(_filePath, rootPath);

    if (!_file.open(abpath, "rb+")) {
        return false;
    }

    _demuxer.setOnFlvHeader([wSelf](const char* data, int len) {
        auto self = wSelf.lock();
        if (!self) {
            return;
        }

        self->_hasVideo = data[4] & 0x1;
        self->_hasAudio = data[4] & 0x4;
        });

    _demuxer.setOnFlvMetadata([wSelf](const char* data, int len) {
        auto self = wSelf.lock();
        if (!self) {
            return;
        }

        // 去掉flv tag头
        data += 11;
        len -= 11;

        AmfDecoder decoder;
        int bytes_used = 0;
        bytes_used += decoder.decode(data + bytes_used, len - bytes_used, 1);
        if (decoder.getString() != "onMetaData") {
            logWarn << "flv metadata header error: " << decoder.getString();
            return;
        }

        decoder.reset();
        bytes_used += decoder.decode(data + bytes_used, len - bytes_used, 1);
        
      /*  auto src = self->_source.lock();
        if (src) {
            src->setMetadata(decoder.getObjects());
        }*/
    });

    _demuxer.setOnFlvMedia([wSelf](const char* data, int len) {
        auto self = wSelf.lock();
        if (!self) {
            return;
        }

        auto type = data[0];

        // 去掉flv tag头
        // data += 11;
        // len -= 11;

        if (type == 8) {
            self->handleAudio(data, len);
        }
        else if (type == 9) {
            self->handleVideo(data, len);
        }
        else {
            logWarn << "invalid tag type: " << (int)(type);
        }
    });

    getDurationFromFile();
    _loop->addTimerTask(40, [wSelf]() {
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        if (self->_isPause) {
            return 40;
        }
        
        auto now = self->_clock.startToNow();
        int sleepTime = 40;

        logDebug << "start to read flv";
        lock_guard<mutex> lck(self->_mtxFrameList);

        while (!self->_frameList.empty()) {
            if (self->_isPause) {
                return 40;
            }
            auto frame = self->_frameList.front();
            logTrace << "frame->dts(): " << frame->dts() << ", self->_baseDts: " << self->_baseDts
                << ", now: " << now;
            uint64_t dtsDiff = 0;
            if (frame->dts() < self->_baseDts) {
                self->_frameList.pop_front();
                continue;
            }
            dtsDiff = (frame->dts() - self->_baseDts) / self->_scale;
            if (dtsDiff <= now || frame->dts() > self->_lastFrameTime + 500) {
                if (self->_onFrame) {
                    self->_frameList.pop_front();
                    self->_lastFrameTime = frame->dts();
                    frame->_dts /= self->_scale;
                    frame->_pts /= self->_scale;
                    self->_onFrame(frame);
                }
            }
            else {
                break;
            }
        }

        logDebug << "self->_frameList size: " << self->_frameList.size();
        if (self->_frameList.size() < 25 && !self->_isReading) {
            // for (int i = 0; i < 25; ++i) {
            auto task = make_shared<WorkTask>();
            task->priority_ = 100;
            task->func_ = [wSelf]() {
                auto self = wSelf.lock();
                if (!self) {
                    return;
                }
                auto buffer = self->_file.read();
                if (!buffer) {
                    self->close();
                    return;
                }
                logInfo << "start on flv stream ....";
                self->_demuxer.input(buffer->data(), buffer->size());
                self->_isReading = false;
                };
            self->_workLoop->addOrderTask(task);
            self->_isReading = true;
            // }
        }
        return sleepTime > 0 ? sleepTime : 1;

    }, nullptr);
    return true;
}

void RecordReaderFlv::stop()
{
    close();
}
void RecordReaderFlv::close()
{
    startTimeStamp = 0;
    RecordReader::close();
}

void RecordReaderFlv::handleVideo(const char* data, int len)
{

    if (!_validVideoTrack) {
        return ;
    }
    weak_ptr<RecordReaderFlv> wSelf = dynamic_pointer_cast<RecordReaderFlv>(shared_from_this());
	uint8_t type = RTMP_VIDEO;
	uint8_t *payload = (u_char*)data + 11;
	uint32_t length = len - 11;
	uint8_t frame_type = (payload[0] >> 4) & 0x0f;
	uint8_t codec_id = payload[0] & 0x0f;

    uint32_t timestamp = readUint24BE(data + 4); //扩展字段也读了

    logTrace << "timestamp: " << timestamp;
    timestamp |= ((data[8]) << 24);

    if (!_rtmpVideoDecodeTrack) {
        _rtmpVideoDecodeTrack = make_shared<RtmpDecodeTrack>(VideoTrackType);
        if (_rtmpVideoDecodeTrack->createTrackInfo(VideoTrackType, codec_id) != 0) {
            _validVideoTrack = false;
            return;
        }
        _rtmpVideoDecodeTrack->setOnFrame([wSelf](const FrameBuffer::Ptr& frame) {
            auto self = wSelf.lock();
            if (!self) {
                return;
            }
            lock_guard<mutex> lck(self->_mtxFrameList);
            logTrace << "###input frame flv";
            self->_frameList.push_back(frame);
        });
        if (_onTrackInfo) {
            _onTrackInfo(_rtmpVideoDecodeTrack->getTrackInfo());
        }
        _rtmpVideoDecodeTrack->startDecode();
    }

   

    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(length + 1);
    memcpy(msg->payload->data(), payload, length);
    msg->abs_timestamp = timestamp;
    msg->length = length;
    msg->type_id = RTMP_VIDEO;
    msg->csid = RTMP_CHUNK_VIDEO_ID;

    if (!_avcHeader && frame_type == 1/* && codec_id == RTMP_CODEC_ID_H264*/) {
            // logInfo << "payload[1] : " << (int)payload[1];
        if (payload[1] == 0) {
            // sps pps??
            _avcHeaderSize = length;
            _avcHeader = make_shared<StreamBuffer>(length + 1);
            memcpy(_avcHeader->data(), msg->payload->data(), length);
            // session->SetAvcSequenceHeader(avc_sequence_header_, avc_sequence_header_size_);
            
            type = RTMP_AVC_SEQUENCE_HEADER;
            _rtmpVideoDecodeTrack->setConfigFrame(msg);
        }
    }

    // FILE* fp = fopen("testrtmp.rtmp", "ab+");
    // fwrite(msg->payload.get(), msg->length, 1, fp);
    // fclose(fp);
    msg->trackIndex_ = VideoTrackType;
    _rtmpVideoDecodeTrack->onRtmpPacket(msg);

    logTrace << "_rtmpVideoDecodeTrack decodeRtmp";
    _rtmpVideoDecodeTrack->decodeRtmp(msg);

}

void RecordReaderFlv::handleAudio(const char* data, int len)
{
    if (!_validAudioTrack) {
        return ;
    }
    weak_ptr<RecordReaderFlv> wSelf = dynamic_pointer_cast<RecordReaderFlv>(shared_from_this());
	uint8_t type = RTMP_AUDIO;
	uint8_t *payload = (u_char*)data + 11;
	uint32_t length = len - 11;
	uint8_t sound_format = (payload[0] >> 4) & 0x0f;
	//uint8_t sound_size = (payload[0] >> 1) & 0x01;
	//uint8_t sound_rate = (payload[0] >> 2) & 0x03;
	uint8_t codec_id = payload[0] & 0x0f;

    uint32_t timestamp = readUint32BE(data + 4); //扩展字段也读了

    if (!_rtmpAudioDecodeTrack) {
        _rtmpAudioDecodeTrack = make_shared<RtmpDecodeTrack>(AudioTrackType);
        if (_rtmpAudioDecodeTrack->createTrackInfo(AudioTrackType, sound_format) != 0) {
            _validAudioTrack = false;
            return ;
        }
        _rtmpAudioDecodeTrack->setOnFrame([wSelf](const FrameBuffer::Ptr& frame) {
            auto self = wSelf.lock();
            if (!self) {
                return;
            }
            lock_guard<mutex> lck(self->_mtxFrameList);
            self->_frameList.push_back(frame);
        });
        
        if (_onTrackInfo) {
            _onTrackInfo(_rtmpAudioDecodeTrack->getTrackInfo());
        }
        _rtmpAudioDecodeTrack->startDecode();
    }

    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(length + 1);
    
    memcpy(msg->payload->data(), payload, length);

    msg->abs_timestamp = timestamp;
    msg->length = length;
    msg->type_id = RTMP_AUDIO;
    msg->csid = RTMP_CHUNK_AUDIO_ID;

    // auto msg = make_shared<RtmpMessage>(std::move(msg));
    if (!_aacHeader && sound_format == RTMP_CODEC_ID_AAC && payload[1] == 0) {
        _aacHeaderSize = msg->length;
        _aacHeader = make_shared<StreamBuffer>(length + 1);
        memcpy(_aacHeader->data(), msg->payload->data(), msg->length);
        // session->SetAacSequenceHeader(aac_sequence_header_, aac_sequence_header_size_);
        type = RTMP_AAC_SEQUENCE_HEADER;
        
        _rtmpAudioDecodeTrack->setConfigFrame(msg);
    }

    msg->trackIndex_ = AudioTrackType;
    _rtmpAudioDecodeTrack->onRtmpPacket(msg);
    _rtmpAudioDecodeTrack->decodeRtmp(msg);

}

void RecordReaderFlv::getDurationFromFile()
{
    uint32_t timestamp;
    int64_t sz;
    uint8_t data[FLV_HEADER_SIZE];
    uint8_t header[FLV_TAG_HEADER_SIZE];
    struct FlvTagHeader tag;

    if (FLV_HEADER_SIZE != _file.read((char*)data, FLV_HEADER_SIZE))
        return;

    if (4 != _file.read((char*)&sz, 4))
        return;
    // 增加头长度
    firstMediaTagLen += 13;
    int r = _file.read((char*)&header, FLV_TAG_HEADER_SIZE);
    if (r != FLV_TAG_HEADER_SIZE)
        return;
    firstMediaTagLen += FLV_TAG_HEADER_SIZE;

    // 读取script
    FlvTagHeaderRead(&tag, header, FLV_TAG_HEADER_SIZE);
    if (tag.type == FLV_TYPE_SCRIPT)
    {
        
        _file.seek_from_cur(tag.size);
        _file.seek_from_cur(4);

        firstMediaTagLen += tag.size;
        firstMediaTagLen += 4;
    }

    // 读取第一个tag 的 timestamp
    r = _file.read((char*)&header, FLV_TAG_HEADER_SIZE);
    if (r != FLV_TAG_HEADER_SIZE)
        return;
    FlvTagHeaderRead(&tag, header, FLV_TAG_HEADER_SIZE);

    startTimeStamp = tag.timestamp;


    _file.seek_from_end(-4);
    _file.read((char*)&header, 4);
    
    sz = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    _file.seek_from_end(-sz-4);

    r = _file.read((char*)&header, FLV_TAG_HEADER_SIZE);
    if (r != FLV_TAG_HEADER_SIZE)
        return;

    // 读取最后一个tag 的timestamp
    FlvTagHeaderRead(&tag, header, FLV_TAG_HEADER_SIZE);
    timestamp = tag.timestamp;

    durationTime = timestamp - startTimeStamp; //

    _file.rewind();
}

uint64_t RecordReaderFlv::getDuration()
{
    return durationTime;
}

int RecordReaderFlv::FlvTagHeaderRead(struct FlvTagHeader* tag, const uint8_t* buf, size_t len)
{
    if (len < FLV_TAG_HEADER_SIZE)
    {
        assert(0);
        return -1;
    }

    // TagType
    tag->type = buf[0] & 0x1F;
    tag->filter = (buf[0] >> 5) & 0x01;
    assert(FLV_TYPE_VIDEO == tag->type || FLV_TYPE_AUDIO == tag->type || FLV_TYPE_SCRIPT == tag->type);

    // DataSize
    tag->size = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];

    // TimestampExtended | Timestamp
    tag->timestamp = ((uint32_t)buf[4] << 16) | ((uint32_t)buf[5] << 8) | buf[6] | ((uint32_t)buf[7] << 24);

    // StreamID Always 0
    tag->streamId = ((uint32_t)buf[8] << 16) | ((uint32_t)buf[9] << 8) | buf[10];
    //assert(0 == tag->streamId);

    return FLV_TAG_HEADER_SIZE;
}

void RecordReaderFlv::seek(uint64_t timeStamp)
{
    weak_ptr<RecordReaderFlv> wSelf = dynamic_pointer_cast<RecordReaderFlv>(shared_from_this());
    auto task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [wSelf, timeStamp]() {
        auto self = wSelf.lock();
        if (!self) {
            return;
        }
  
        self->_file.rewind();
        self->_file.seek(self->firstMediaTagLen);

        self->_clock.update();
        self->_baseDts = timeStamp;
        self->_demuxer.resetToSeek();

        uint8_t header[FLV_TAG_HEADER_SIZE];
        int r = 0;
        struct FlvTagHeader tag;
        while (!self->_file.isEnd()) {
            r = self->_file.read((char*)&header, FLV_TAG_HEADER_SIZE);
            if (r != FLV_TAG_HEADER_SIZE)
                break;
            self->FlvTagHeaderRead(&tag, header, FLV_TAG_HEADER_SIZE);
            if (timeStamp <= tag.timestamp)
            {
                lock_guard<mutex> lck(self->_mtxFrameList);
                self->_frameList.clear();

                self->_file.seek_from_cur(-FLV_TAG_HEADER_SIZE);

                self->_isReading = false;
                break;
            }
            self->_file.read(tag.size+4);
           /* logInfo << "start seek ps stream";
            if (self->_demuxer->seek(buffer->data(), buffer->size(), timeStamp, 0) == 0) {
                break;
            }*/
        }
    };
    _workLoop->addOrderTask(task);
}

void RecordReaderFlv::pause(bool isPause)
{
    _isPause = isPause;
}

void RecordReaderFlv::scale(float scale)
{
    _scale = scale;
    _clock.update();
    _baseDts = _lastFrameTime;
}
//int RecordReaderFlv::FlvReaderRead(int* tagtype, uint32_t* timestamp, size_t* taglen)
//{
//    int r;
//    uint32_t sz;
//    uint8_t header[FLV_TAG_HEADER_SIZE];
//    struct FlvTagHeader tag;
//
//    while (!_file.isEnd())
//    {
//        r = _file.read((char*)&header, FLV_TAG_HEADER_SIZE);
//        if (r != FLV_TAG_HEADER_SIZE)
//            break;
//
//        FlvTagHeaderRead(&tag, header, FLV_TAG_HEADER_SIZE);
//        if (0 == startTimeStamp)
//            startTimeStamp = tag.timestamp;
//
//        // FLV stream
//        _file.seek(tag.size);
//
//        // PreviousTagSizeN
//        r = _file.read((char*)&header, 4);
//        if (r != 4)
//            break;
//        *taglen = tag.size;
//        *tagtype = tag.type;
//        *timestamp = tag.timestamp;
//
//        sz = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
//        //flv_tag_size_read(header, 4, &sz);
//        //assert(0 == tag.streamId); // StreamID Always 0
//        //assert(sz == tag.size + FLV_TAG_HEADER_SIZE);
//        if (sz != tag.size + FLV_TAG_HEADER_SIZE)
//            break;
//        //return (sz == tag.size + FLV_TAG_HEADER_SIZE) ? 1 : -1;
//    }
//    _file.rewind();
//    return 0;
//}

#endif