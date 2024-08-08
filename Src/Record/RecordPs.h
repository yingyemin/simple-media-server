#ifndef RecordPs_H
#define RecordPs_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Util/File.h"
#include "EventPoller/EventLoop.h"
#include "WorkPoller/WorkLoop.h"
#include "Common/UrlParser.h"
#include "Mpeg/PsMediaSource.h"
#include "Record.h"

using namespace std;

class RecordPs : public Record
{
public:
    using Ptr = shared_ptr<RecordPs>;

    RecordPs(const UrlParser& urlParser);
    ~RecordPs();

public:
    bool start() override;
    void stop() override;
    void setOnClose(const function<void()>& cb);

private:
    void onError(const string& err);
    void onPlayPs(const PsMediaSource::Ptr &psSrc);

private:
    File _file;
    UrlParser _urlParser;
    EventLoop::Ptr _loop;
    WorkLoop::Ptr _workLoop;
    PsMediaSource::Wptr _source;
    PsMediaSource::RingType::DataQueReaderT::Ptr _playPsReader;
    function<void()> _onClose;
};


#endif //RecordPs_H
