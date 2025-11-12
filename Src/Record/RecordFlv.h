#ifdef ENABLE_FLV

#ifndef RecordFLV_H
#define RecordFLV_H


#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include "Net/Buffer.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"
#include "Util/TimeClock.h"
#include "Flv/FlvFileWriter.h"
#include "Util/Util.h"

#include "Record.h"
// using namespace std;


class RecordFlv : public Record
{
public:
    using Ptr = std::shared_ptr<RecordFlv>;

    RecordFlv(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate);
    ~RecordFlv();
public:
    bool start() override;
    void stop() override;
    void setOnClose(const std::function<void()>& cb);
    std::string getFormat() override { return "flv"; }
private:
    void onError(const std::string& err);
    void onPlayFrame(const RtmpMediaSource::Ptr& frameSrc);
private:
    bool _stop = false;
    int _recordCount = 0;
    uint64_t _recordDuration = 0;
    FlvFileWriter::Ptr _flvWriter = nullptr;
    File _file;
    TimeClock _clock;
    RecordTemplate::Ptr _template;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    RtmpMediaSource::Wptr _source;
    //MediaSource::FrameRingType::DataQueReaderT::Ptr _playFrameReader;
    RtmpMediaSource::RingType::DataQueReaderT::Ptr _playFrameReader;
    std::function<void()> _onClose;
};


#endif
#endif