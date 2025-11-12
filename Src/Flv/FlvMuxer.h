#ifndef FLVMuxer_H
#define FLVMuxer_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Util/File.h"
#include "Util/Util.h"
#include "Rtmp/RtmpMediaSource.h"

// using namespace std;
#pragma pack(push, 1)
class FLVHeader {
public:
    static constexpr uint8_t kFlvVersion = 1;
    static constexpr uint8_t kFlvHeaderLength = 9;
    //FLV
    char flv[3];
    //File version (for example, 0x01 for FLV version 1)
    uint8_t version;
#if __BYTE_ORDER == __BIG_ENDIAN
    // 保留,置0  [AUTO-TRANSLATED:46985374]
    // Preserve, set to 0
    uint8_t : 5;
    // 是否有音频  [AUTO-TRANSLATED:9467870a]
    // Whether there is audio
    uint8_t have_audio : 1;
    // 保留,置0  [AUTO-TRANSLATED:46985374]
    // Preserve, set to 0
    uint8_t : 1;
    // 是否有视频  [AUTO-TRANSLATED:42d0ed81]
    // Whether there is video
    uint8_t have_video : 1;
#else
    // 是否有视频  [AUTO-TRANSLATED:42d0ed81]
    // Whether there is video
    uint8_t have_video : 1;
    // 保留,置0  [AUTO-TRANSLATED:46985374]
    // Preserve, set to 0
    uint8_t : 1;
    // 是否有音频  [AUTO-TRANSLATED:9467870a]
    // Whether there is audio
    uint8_t have_audio : 1;
    // 保留,置0  [AUTO-TRANSLATED:46985374]
    // Preserve, set to 0
    uint8_t : 5;
#endif
    // The length of this header in bytes,固定为9  [AUTO-TRANSLATED:126988fc]
    // The length of this header in bytes, fixed to 9
    uint32_t length;
    // 固定为0  [AUTO-TRANSLATED:d266c0a7]
    // Fixed to 0
    uint32_t previous_tag_size0;
};

class RtmpTagHeader {
public:
    uint8_t type = 0;
    uint8_t data_size[3] = { 0 };
    uint8_t timestamp[3] = { 0 };
    uint8_t timestamp_ex = 0;
    uint8_t streamid[3] = { 0 }; /* Always 0. */
};

struct RtmpVideoHeaderEnhanced {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t enhanced : 1;
    uint8_t frame_type : 3;
    uint8_t pkt_type : 4;
    uint32_t fourcc;
#else
    uint8_t pkt_type : 4;
    uint8_t frame_type : 3;
    uint8_t enhanced : 1;
    uint32_t fourcc;
#endif
};
#pragma pack(pop)

class FlvMuxer {
public:
    using Ptr = std::shared_ptr<FlvMuxer>;
    FlvMuxer() = default;
    virtual ~FlvMuxer() = default;

public:
    void onWriteFlvHeader(const RtmpMediaSource::Ptr& src);
    //void onWriteRtmp(const RtmpMessage::Ptr& pkt, bool flush);
    void onWriteFlvTag(const RtmpMessage::Ptr& pkt, uint32_t time_stamp);
    void onWriteFlvTag(uint8_t type, const StreamBuffer::Ptr& buffer, uint32_t time_stamp);
public:
    virtual void onWriteCustom(const char* buffer, int len) {}
};


#endif //FlvFileWriter_H
