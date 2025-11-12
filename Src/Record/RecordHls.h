#ifndef RecordHls_H
#define RecordHls_H

#ifdef ENABLE_HLS

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Hls/HlsFileWrite.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"
#include "Util/TimeClock.h"
#include "Common/FrameMediaSource.h"
#include "Record.h"

// using namespace std;

class RecordHls : public Record
{
public:
    using Ptr = std::shared_ptr<RecordHls>;

    RecordHls(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate);
    ~RecordHls();

public:
    bool start() override;
    void stop() override;
    void setOnClose(const std::function<void()>& cb);
    std::string getFormat() override { return "hls"; }
private:
    void onError(const std::string& err);
    void onPlayFrame(const FrameMediaSource::Ptr &frameSrc);

private:
    bool _stop = false;
    int _recordCount = 0;
    uint64_t _recordDuration = 0;
    File _file;
    TimeClock _clock;
    RecordTemplate::Ptr _template;
    HlsFileWriter::Ptr _hlsWrite;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    FrameMediaSource::Wptr _source;
    MediaSource::FrameRingType::DataQueReaderT::Ptr _playFrameReader;
    std::function<void()> _onClose;
};

#endif
#endif //RecordPs_H
