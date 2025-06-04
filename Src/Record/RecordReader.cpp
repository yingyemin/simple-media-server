#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReader.h"
#include "Logger.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "RecordReaderMp4.h"
#include "RecordReaderPs.h"
#include "RecordReaderDir.h"

using namespace std;

RecordReader::RecordReader(const string& path)
{

}

RecordReader::~RecordReader()
{
    close();
}

void RecordReader::init()
{
    RecordReaderBase::registerCreateFunc([](const string& path) -> RecordReaderBase::Ptr {
        if (startWith(path, "/dir")) {
            return make_shared<RecordReaderDir>(path);
        }
        
        string ext= path.substr(path.rfind('.') + 1);
        ext = ext.substr(0, ext.find_first_of("/"));
#ifdef ENABLE_MPEG
        if (!strcasecmp(ext.data(), "ps") || !strcasecmp(ext.data(), "mpeg")) {
            return make_shared<RecordReaderPs>(path);
        }
#endif
#ifdef ENABLE_MP4
        if (!strcasecmp(ext.data(), "mp4")) {
            return make_shared<RecordReaderMp4>(path);
        }
#endif
        return nullptr;
    });
}

bool RecordReader::start()
{
    weak_ptr<RecordReader> wSelf = shared_from_this();
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();
    _clock.start();

    return true;
}

void RecordReader::stop()
{
    close();
}

void RecordReader::close()
{
    if (_onClose) {
        _onClose();
    }
    _onClose = nullptr;
    _onTrackInfo = nullptr;
    _onReady = nullptr;
    _onFrame = nullptr;
}

void RecordReader::setOnTrackInfo(const function<void(const TrackInfo::Ptr& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void RecordReader::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void RecordReader::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void RecordReader::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    _onFrame = cb;
}
