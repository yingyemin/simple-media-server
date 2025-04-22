#ifndef RecordReader_H
#define RecordReader_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <list>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"
#include "Util/File.h"
#include "Common/RecordReaderBase.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"
#include "Util/TimeClock.h"

using namespace std;

class RecordReader : public RecordReaderBase, public enable_shared_from_this<RecordReader>
{
public:
    using Ptr = shared_ptr<RecordReader>;

    RecordReader(const string& path);
    ~RecordReader();

public:
    static void init();

public:
    virtual bool start();
    virtual void stop();
    virtual void close();
    virtual void seek(uint64_t timeStamp) = 0;
    virtual void pause(bool isPause) = 0;
    virtual void scale(float scale) = 0;
    void setOnTrackInfo(const function<void(const TrackInfo::Ptr& trackInfo)>& cb);
    void setOnReady(const function<void()>& cb);
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb);
    void setOnClose(const function<void()>& cb);

protected:
    uint64_t _lastFrameTime = 0;
    string _filePath;
    File _file;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    TimeClock _clock;
    list<FrameBuffer::Ptr> _frameList;
    function<void(const TrackInfo::Ptr& trackInfo)> _onTrackInfo;
    function<void()> _onReady;
    function<void()> _onClose;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RecordReader_H
