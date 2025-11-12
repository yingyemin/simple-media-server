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
    using RecordMenuMap = unordered_map<string/*path*/, 
            unordered_map<string/*year*/, 
                unordered_map<string/*month*/, 
                    unordered_map<string/*day*/, vector<string/*file*/>>>>>;

    RecordReader(const string& path);
    ~RecordReader();

public:
    static void init();

public:
    virtual bool start();
    virtual void stop();
    virtual void close();
    virtual void seek(uint64_t timeStamp) {};
    virtual void pause(bool isPause) {};
    virtual void scale(float scale) {};
    virtual void release() {};
    virtual uint64_t getDuration() {return 0;}
    void setOnTrackInfo(const function<void(const TrackInfo::Ptr& trackInfo)>& cb);
    void setOnReady(const function<void()>& cb);
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb);
    void setOnClose(const function<void()>& cb);

    static void loadRecordMenu();
    static RecordMenuMap getRecordList(const string& path = "", const string& year = "", const string& month = "", const string& day = "");
    static void addRecordInfo(const string& path, const string& year, const string& month, const string& day, const string& fileName);
    static void delRecordInfo(const string& path, const string& year, const string& month, const string& day, const string& fileName);

protected:
    bool _isPause = false;
    int _loopCount = 1;
    int _curLoopCount = 0;
    float _scale = 1;
    uint64_t _lastFrameTime = 0;
    uint64_t _baseDts = 0;
    uint64_t _playDuration = 0;
    string _filePath;
    string _recordType;
    string _vodId;
    File _file;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    TimeClock _clock;
    mutex _mtxFrameList;
    list<FrameBuffer::Ptr> _frameList;
    function<void(const TrackInfo::Ptr& trackInfo)> _onTrackInfo;
    function<void()> _onReady;
    function<void()> _onClose;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;

    static mutex _mtxRecordMenu;
    static RecordMenuMap _recordMenu;
};

#endif //RecordReader_H
