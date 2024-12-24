#ifndef AV1Frame_H
#define AV1Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

using namespace std;

enum AV1OBUType
{
    Reserved = 0,
    OBU_SEQUENCE_HEADER, // like SPS
    OBU_TEMPORAL_DELIMITER,
    OBU_FRAME_HEADER, // like PPS
    OBU_TILE_GROUP,
    OBU_METADATA,
    OBU_FRAME,
    OBU_REDUNDANT_FRAME_HEADER,
    OBU_TILE_LIST,
    OBU_PADDING = 15
};

enum AV1NalType
{
    AV1_KEYFRAME = 0,
    AV1_INTERFRAME = 1,
    AV1_INTRA_ONLY_FRAME = 2,
    AV1_SWITCHFRAME = 3
};

uint8_t* leb128(const uint8_t* data, int bytes, uint64_t* v);

class AV1Frame : public FrameBuffer
{
public:
    using Ptr = shared_ptr<AV1Frame>;

    AV1Frame();
    
    AV1Frame(const AV1Frame::Ptr& frame);

    bool isNewNalu() override;

    bool keyFrame() const override {return true;}

    bool metaFrame() const override {return getObuType() == OBU_SEQUENCE_HEADER;}

    bool startFrame() const override {return getObuType() == OBU_SEQUENCE_HEADER;}

    uint8_t getNalType() override {return AV1_KEYFRAME;}
    uint8_t getObuType() const;

    bool isNonPicNalu() override {return getObuType() != OBU_FRAME;}

    void split(const function<void(const FrameBuffer::Ptr& frame)>& cb) override;

    static uint8_t getNalType(uint8_t nalByte) {return AV1_KEYFRAME;}
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    static void registerFrame();
};


#endif //AV1Frame_H
