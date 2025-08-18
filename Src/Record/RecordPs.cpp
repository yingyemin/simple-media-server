#ifdef ENABLE_MPEG

#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

#include "WorkPoller/WorkLoopPool.h"
#include "Common/Config.h"
#include "RecordPs.h"

using namespace std;

RecordPs::RecordPs(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate)
{
    _template = recordTemplate;
    _urlParser = urlParser;
}

RecordPs::~RecordPs()
{
    auto psSrc = _source.lock();
    if (psSrc) {
        psSrc->delConnection(this);
    }
}

bool RecordPs::start()
{
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();

    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    auto now = time(nullptr);
    auto nowTm = TimeClock::localtime(now);
    auto abpath = File::absolutePath(_urlParser.path_, rootPath) + "/" 
                    + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon) 
                    + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".ps";
    logInfo << "get record path: " << abpath;

    _file = make_shared<File>();
    if (!_file->open(abpath, "wb+")) {
        return false;
    }
    
    _recordInfo.uri = _urlParser.path_;
    _recordInfo.status = "on";
    _recordInfo.startTime = time(nullptr);
    _recordInfo.filePath = abpath;
    _recordInfo.fileName = _urlParser.path_ + "/" 
                    + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon) 
                    + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".ps";

    weak_ptr<RecordPs> wSelf = dynamic_pointer_cast<RecordPs>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto psSrc = dynamic_pointer_cast<PsMediaSource>(src);
		if (!psSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, psSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayPs(psSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<PsMediaSource>(self->_urlParser, nullptr, true);
    }, this);

    return true;
}

void RecordPs::onPlayPs(const PsMediaSource::Ptr &psSrc)
{
    weak_ptr<RecordPs> wSelf = dynamic_pointer_cast<RecordPs>(shared_from_this());

    if (!_playPsReader) {
		logInfo << "set _playPsReader";
		_playPsReader = psSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playPsReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = "record-ps";
			ret.port_ = 0;
			ret.protocol_ = self->_urlParser.protocol_;
			return ret;
		});
		_playPsReader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logInfo << "setReadCB =================";
		_playPsReader->setReadCB([wSelf](const PsMediaSource::RingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/) {
				return;
			}
			// logInfo << "send rtmp msg";
			auto pktList = *(pack.get());
			for (auto& pkt : pktList) {
                auto buffer = StreamBuffer::create();
                buffer->assign(pkt->data(), pkt->size());

                auto task = make_shared<WorkTask>();
                task->priority_ = 100;
                task->func_ = [wSelf, buffer, pkt](){
                    auto self = wSelf.lock();
                    if (!self) {
                        return ;
                    }
                    
                    self->tryNewSegment(pkt);

                    self->_file->write(buffer);
                };
                self->_workLoop->addOrderTask(task);
			}
		});
        _source = psSrc;
	}
}

void RecordPs::tryNewSegment(const FrameBuffer::Ptr& frame)
{
    logDebug << "_template->duration: " << _template->duration;
    logDebug << "_recordDuration: " << _recordDuration;
    logDebug << "_clock.startToNow(): " << _clock.startToNow();
    logDebug << "_recordDuration: " << _recordDuration;
    logDebug << "_template->segment_duration: " << _template->segment_duration;
    if (_template->duration > 0 && _recordDuration + _clock.startToNow() > _template->duration) {
        stop();
        return ;
    }

    if ((frame->keyFrame() || frame->metaFrame()) && _clock.startToNow() > _template->segment_duration) {
        _recordDuration += _clock.startToNow();
        if ((_template->segment_count > 0 && ++_recordCount >= _template->segment_count)) {
            stop();
            return ;
        }

        _clock.update();

        _file->close();
        // _recordInfo.status = "off";
        _recordInfo.endTime = time(nullptr);
        _recordInfo.duration = _recordInfo.endTime - _recordInfo.startTime;
        struct stat statbuf;
        if (stat(_recordInfo.filePath.data(), &statbuf) == 0) {
            _recordInfo.fileSize = statbuf.st_size;
        }
        

        auto hook = HookManager::instance()->getHook("MediaHook");
        if (hook) {
            hook->onRecord(_recordInfo);
        }

        static string rootPath = Config::instance()->getAndListen([](const json &config){
            rootPath = Config::instance()->get("Record", "rootPath");
        }, "Record", "rootPath");

        auto now = time(nullptr);
        auto nowTm = TimeClock::localtime(now);
        auto abpath = File::absolutePath(_urlParser.path_, rootPath) + "/" 
                    + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon) 
                    + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".ps";
        logInfo << "get record path: " << abpath;
        // if (!_file.open(abpath, "wb+")) {
        //     return false;
        // }

        _file = make_shared<File>();
        if (!_file->open(abpath, "wb+")) {
            stop();
            return ;
        }

        OnRecordInfo info;
        _recordInfo = info;
        _recordInfo.uri = _urlParser.path_;
        _recordInfo.status = "on";
        _recordInfo.startTime = time(nullptr);
        _recordInfo.filePath = abpath;
        _recordInfo.fileName = _urlParser.path_ + "/" 
                    + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon) 
                    + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".ps";

        // if (hook) {
        //     hook->onRecord(_recordInfo);
        // }
    }
}

void RecordPs::stop()
{
    auto self = dynamic_pointer_cast<RecordPs>(shared_from_this());

    auto task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [self](){
        self->_file->close();
        self->_recordInfo.endTime = time(nullptr);
        self->_recordInfo.duration = self->_recordInfo.endTime - self->_recordInfo.startTime;
        struct stat statbuf;
        if (stat(self->_recordInfo.filePath.data(), &statbuf) == 0) {
            self->_recordInfo.fileSize = statbuf.st_size;
        }
        

        auto hook = HookManager::instance()->getHook("MediaHook");
        if (hook) {
            hook->onRecord(self->_recordInfo);
        }
    };
    _workLoop->addOrderTask(task);

    if (_onClose) {
        _onClose();
    }
}

void RecordPs::onError(const string& err)
{
    logInfo << "get a err: " << err;
    stop();
}

void RecordPs::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

#endif