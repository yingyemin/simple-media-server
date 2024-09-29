#ifndef Frame_H
#define Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"

using namespace std;

class FrameBuffer : public enable_shared_from_this<FrameBuffer>
{
public:
    using Ptr = std::shared_ptr<FrameBuffer>;

    FrameBuffer();

    static const char* findNextNalu(const char* p, size_t bytes, size_t& leading);
    static int startSize(const char* data, int len);

    char *data() const { return (char *)_buffer.data(); }
    size_t size() const { return _buffer.size(); }
    uint64_t dts() const { return _dts; }
    uint64_t pts() const { return _pts; }
    size_t startSize() const { return _startSize; }
    string codec() const { return _codec; }
    int getTrackType() {return _trackType;}
    int getTrackIndex() {return _index;}

    virtual bool keyFrame() const { return false; }
    virtual bool metaFrame() const { return false; }
    virtual bool startFrame() const { return false; }
    virtual uint8_t getNalType() { return 0;}
    virtual void split(const function<void(const FrameBuffer::Ptr& frame)>& cb) {cb(shared_from_this());}
    virtual bool isBFrame() {return false;}

public:
    int _profile = 0;
    int _index;
    int _trackType = 0;
    uint64_t _dts = 0;
    uint64_t _pts = 0;
    size_t _startSize = 0;
    string _codec;
    StringBuffer _buffer;
};


#endif //Frame_H
