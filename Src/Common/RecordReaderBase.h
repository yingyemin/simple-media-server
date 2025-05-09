#ifndef RecordReaderBase_H
#define RecordReaderBase_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <list>

#include "Net/Buffer.h"
#include "Track.h"
#include "Frame.h"
#include "Util/File.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"

using namespace std;

class RecordReaderBase
{
public:
    using Ptr = shared_ptr<RecordReaderBase>;

    RecordReaderBase() {}
    ~RecordReaderBase() {}

public:
    virtual bool start() {return true;}
    virtual void stop() {}
    virtual void close() {}
    virtual void setOnTrackInfo(const function<void(const TrackInfo::Ptr& trackInfo)>& cb) {}
    virtual void setOnReady(const function<void()>& cb) {}
    virtual void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {}
    virtual void setOnClose(const function<void()>& cb) {}
    
    virtual void seek(uint64_t timeStamp) {};
    virtual void pause(bool isPause) {};
    virtual void scale(float scale) {};
    virtual uint64_t getDuration() {return 0;};

    static void registerCreateFunc(const function<RecordReaderBase::Ptr(const string& path)>& func);
    static RecordReaderBase::Ptr createRecordReader(const string& path);

private:
    static function<RecordReaderBase::Ptr(const string& path)> _createRecordReader;
};

#endif //RecordReaderBase_H
