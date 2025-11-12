#ifdef ENABLE_MPEG

#ifndef RecordPs_H
#define RecordPs_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Util/File.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"
#include "Common/UrlParser.h"
#include "Mpeg/PsMediaSource.h"
#include "Record.h"

// using namespace std;

class RecordPs : public Record
{
public:
    using Ptr = std::shared_ptr<RecordPs>;

    RecordPs(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate);
    ~RecordPs();

public:
    bool start() override;
    void stop() override;
    void setOnClose(const std::function<void()>& cb) override;
    std::string getFormat() override {return "ps";}

private:
    void onError(const std::string& err);
    void onPlayPs(const PsMediaSource::Ptr &psSrc);
    void tryNewSegment(const FrameBuffer::Ptr& frame);

private:
    int _recordCount = 0;
    uint64_t _recordDuration = 0;
    TimeClock _clock;
    std::shared_ptr<File> _file;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    PsMediaSource::Wptr _source;
    PsMediaSource::RingType::DataQueReaderT::Ptr _playPsReader;
    std::function<void()> _onClose;
};

#endif
#endif //RecordPs_H
