#ifndef RECORD_READER_FLV_H
#define RECORD_READER_FLV_H

#ifdef ENABLE_RTMP

#include "RecordReader.h"
#include "Flv/FlvDemuxer.h"
#include "Rtmp/RtmpMessage.h"
#include "Rtmp/RtmpDecodeTrack.h"
#include "Rtmp/Amf.h"

#include <string>
#include <memory>
#include <functional>

// using namespace std;

struct FlvTagHeader
{
    uint8_t filter; // 0-No pre-processing required
    uint8_t type; // 8-audio, 9-video, 18-script data
    uint32_t size; // data size
    uint32_t timestamp;
    uint32_t streamId;
};

class RecordReaderFlv : public RecordReader
{
public:
    RecordReaderFlv(const string& path);
    ~RecordReaderFlv();

public:

public:
    // override MediaClient
    bool start() override;
    void stop() override;
    void close() override;
    void seek(uint64_t timeStamp) override;
    void pause(bool isPause) override;
    void scale(float scale) override;
private:
    int FlvTagHeaderRead(struct FlvTagHeader* tag, const uint8_t* buf, size_t len);
    //int FlvReaderRead(int* tagtype, uint32_t* timestamp, size_t* taglen);
    void getDurationFromFile();
public:
    void handleAudio(const char* data, int len);
    void handleVideo(const char* data, int len);


    virtual uint64_t getDuration();
private:
    bool _hasVideo = false;
    bool _hasAudio = false;
    bool _validVideoTrack = true;
    bool _validAudioTrack = true;
    
    uint64_t startTimeStamp = 0;
    uint64_t durationTime = 0;
    uint64_t firstMediaTagLen = 0;
    FlvDemuxer _demuxer;
    
    int _avcHeaderSize = 0;
    StreamBuffer::Ptr _avcHeader;
    int _aacHeaderSize = 0;
    StreamBuffer::Ptr _aacHeader;
    
 
    bool _isReading = false;
    RtmpDecodeTrack::Ptr _rtmpVideoDecodeTrack;
    RtmpDecodeTrack::Ptr _rtmpAudioDecodeTrack;

};

#endif
#endif 