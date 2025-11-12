#ifndef Frame_H
#define Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

#include "Net/Buffer.h"

// using namespace std;

class FrameBuffer : public std::enable_shared_from_this<FrameBuffer>
{
public:
    using Ptr = std::shared_ptr<FrameBuffer>;
    using funcCreateFrame = std::function<FrameBuffer::Ptr(int startSize, int index, bool addStart)>;

    FrameBuffer();

    static const char* findNextNalu(const char* p, size_t bytes, size_t& leading);
    static int startSize(const char* data, int len);

    char *data() const { return (char *)_buffer->data(); }
    size_t size() const { return _buffer->size(); }
    uint64_t dts() const { return _dts; }
    uint64_t pts() const { return _pts; }
    size_t startSize() const { return _startSize; }
    std::string codec() const { return _codec; }
    int getTrackType() {return _trackType;}
    int getTrackIndex() {return _index;}
    StringBuffer::Ptr buffer() { return _buffer; }

    virtual bool keyFrame() const { return _isKeyframe; }
    virtual bool metaFrame() const { return _isKeyframe; }
    virtual bool startFrame() const { return false; }
    virtual uint8_t getNalType() { return 0;}
    virtual void split(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) {cb(shared_from_this());}
    virtual bool isBFrame() {return false;}
    virtual bool isNewNalu() {return true;}

    virtual bool isNonPicNalu() {return false;}

    static int getVideoStartSize(const uint8_t* data, int len);
    
    static FrameBuffer::Ptr createFrame(const std::string& codecName, int startSize, int index, bool addStart);
    
    static void registerFrame(const std::string& codecName, const funcCreateFrame& func);

public:
    bool _isKeyframe = false;
    int _profile = 0;
    int _index;
    int _trackType = 0;
    uint64_t _dts = 0;
    uint64_t _pts = 0;
    size_t _startSize = 0;
    std::string _codec;
    StringBuffer::Ptr _buffer;

    static std::unordered_map<std::string, funcCreateFrame> _mapCreateFrame;
};


#endif //Frame_H