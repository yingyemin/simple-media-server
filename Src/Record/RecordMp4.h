#ifndef RecordMp4_H
#define RecordMp4_H

#ifdef ENABLE_MP4

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Mp4/Mp4FileWriter.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"
#include "Util/TimeClock.h"
#include "Common/FrameMediaSource.h"
#include "Record.h"

// using namespace std;

class RecordMp4 : public Record
{
public:
    using Ptr = std::shared_ptr<RecordMp4>;

    RecordMp4(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate);
    ~RecordMp4();

public:
    bool start() override;
    void stop() override;
    void setOnClose(const std::function<void()>& cb) override;
    std::string getFormat() override {return "mp4";}

private:
    void onError(const std::string& err);
    void onPlayFrame(const FrameMediaSource::Ptr &frameSrc);
    void tryNewSegment(const FrameBuffer::Ptr& frame);

private:
    bool _stop = false;
    int _recordCount = 0;
    uint64_t _recordDuration = 0;
    File _file;
    TimeClock _clock;
    Mp4FileWriter::Ptr _mp4Writer;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    FrameMediaSource::Wptr _source;
    MediaSource::FrameRingType::DataQueReaderT::Ptr _playFrameReader;
    std::function<void()> _onClose;
};

#endif
#endif //RecordPs_H
