#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "FlvMuxer.h"
#include "Logger.h"
#include "Util/String.hpp"


using namespace std;

void FlvMuxer::onWriteFlvHeader(const RtmpMediaSource::Ptr& src)
{
    FLVHeader header;
    memset(&header, 0, sizeof(FLVHeader));
    header.flv[0] = 'F';
    header.flv[1] = 'L';
    header.flv[2] = 'V';
    header.version = FLVHeader::kFlvVersion;
    header.length = htonl(FLVHeader::kFlvHeaderLength);
    header.have_video = src->_avcHeaderSize ? 1 : 0;
    header.have_audio = src->_aacHeaderSize ? 1 : 0;
    header.previous_tag_size0 = 0;

    onWriteCustom((char*)&header, sizeof(FLVHeader));

    auto metadata = src->getMetadata();
    if (metadata.size() > 0) {
        AmfEncoder amfEncoder;
        amfEncoder.encodeString("onMetaData", 10);
        amfEncoder.encodeECMA(metadata);

        onWriteFlvTag(RTMP_NOTIFY, amfEncoder.data(), 0);
        //sendFlvTag(RTMP_NOTIFY, 0, amfEncoder.data(), amfEncoder.size());
    }
}

void FlvMuxer::onWriteFlvTag(const RtmpMessage::Ptr& pkt, uint32_t time_stamp) {
    onWriteFlvTag(pkt->type_id, pkt->payload, time_stamp);
}
void FlvMuxer::onWriteFlvTag(uint8_t type, const StreamBuffer::Ptr& buffer, uint32_t time_stamp)
{
#if 0
    char tag_header[11] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char previous_tag_size[4] = { 0x0, 0x0, 0x0, 0x0 };

    tag_header[0] = type;
    writeUint24BE(tag_header + 1, buffer->size());
    tag_header[4] = (time_stamp >> 16) & 0xff;
    tag_header[5] = (time_stamp >> 8) & 0xff;
    tag_header[6] = time_stamp & 0xff;
    tag_header[7] = (time_stamp >> 24) & 0xff;

    writeUint32BE(previous_tag_size, buffer->size() + 11);

    onWriteCustom(tag_header, 11);
    //if (payload->size() > payload_size) {
    //    payload->substr(0, payload_size);
    //}
   
    onWriteCustom(buffer->data(), buffer->size());

    onWriteCustom(previous_tag_size, 4);
#endif
#if 1
    RtmpTagHeader header;
    header.type = type;
    writeUint24BE((char*)header.data_size, (uint32_t)buffer->size());
    header.timestamp_ex = (time_stamp >> 24) & 0xff;
    writeUint24BE((char*)header.timestamp, time_stamp & 0xFFFFFF);

    //tag header
    onWriteCustom((char*)&header, sizeof(header));

    //tag data
    onWriteCustom(buffer->data(), buffer->size());

    //PreviousTagSize
    uint32_t size = htonl((uint32_t)(buffer->size() + sizeof(header)));

    onWriteCustom((char*)&size, 4);
#endif

}