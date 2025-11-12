#ifdef ENABLE_HLS

#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

#include "WorkPoller/WorkLoopPool.h"
#include "Common/Config.h"
#include "RecordHls.h"
#include "Common/HookManager.h"

using namespace std;

RecordHls::RecordHls(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate)
    :_template(recordTemplate)
{
    _urlParser = urlParser;
}

RecordHls::~RecordHls()
{
    auto frameSrc = _source.lock();
    if (frameSrc) {
        frameSrc->delConnection(this);
    }
}

bool RecordHls::start()
{
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();

    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    auto file_name = to_string(time(nullptr));
    auto now = time(nullptr);
    auto nowTm = TimeClock::localtime(now);
    auto abpath = File::absolutePath(_urlParser.path_, rootPath) + "/" 
                    + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon) 
                    + "/" + to_string(nowTm.tm_mday) + "/" + file_name + ".m3u8";
    
    logInfo << "get record path: " << abpath;
    // if (!_file.open(abpath, "wb+")) {
    //     return false;
    // }

    _hlsWrite = make_shared<HlsFileWriter>(_urlParser, abpath,file_name);
    if (!_hlsWrite->open()) {
        return false;
    }

    _recordInfo.uri = _urlParser.path_;
    _recordInfo.status = "on";
    _recordInfo.startTime = time(nullptr);
    _recordInfo.filePath = abpath;
    _recordInfo.fileName = _urlParser.path_ + "/" 
                    + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon) 
                    + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".m3u8";

    auto hook = HookManager::instance()->getHook("MediaHook");
    // if (hook) {
    //     hook->onRecord(_recordInfo);
    // }

    weak_ptr<RecordHls> wSelf = dynamic_pointer_cast<RecordHls>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(src);
		if (!frameSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, frameSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayFrame(frameSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<FrameMediaSource>(self->_urlParser, nullptr);
    }, this);

    return true;
}

void RecordHls::onPlayFrame(const FrameMediaSource::Ptr &frameSrc)
{
    auto trackInfos = frameSrc->getTrackInfo();
    for (auto& trackIter : trackInfos) {
        _hlsWrite->addTrackInfo(trackIter.second);
    }
    _hlsWrite->start();
    _clock.start();
    weak_ptr<RecordHls> wSelf = dynamic_pointer_cast<RecordHls>(shared_from_this());

    if (!_playFrameReader) {
		logInfo << "set _playFrameReader";
		_playFrameReader = frameSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playFrameReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = "record-hls";
			ret.port_ = 0;
			ret.protocol_ = self->_urlParser.protocol_;
			return ret;
		});
		_playFrameReader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logInfo << "setReadCB =================";
		_playFrameReader->setReadCB([wSelf](const MediaSource::FrameRingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/ || self->_stop) {
				return;
			}

            // auto buffer = StreamBuffer::create();
            // buffer->assign(pack->data(), pack->size());

            auto task = make_shared<WorkTask>();
            task->priority_ = 100;
            task->func_ = [wSelf, pack](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
                //self->tryNewSegment(pack);

                logTrace << "pack->keyFrame(): " << pack->keyFrame();
                //self->_hlsWrite->inputFrame(pack, pack->getTrackIndex(), pack->keyFrame());
                self->_hlsWrite->onFrame(pack);
                };
            self->_workLoop->addOrderTask(task);
        });
        _source = frameSrc;
	}
}

void RecordHls::stop()
{
    auto self = dynamic_pointer_cast<RecordHls>(shared_from_this());
     //_loop->async([self](){
     //    self->_hlsWrite->stop();
     //}, true);

    auto task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [self](){
        // auto self = wSelf.lock();
        // if (!self) {
        //     return ;
        // }
        self->_hlsWrite->stop();
        self->_hlsWrite->close();
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
    _stop = true;
}

void RecordHls::onError(const string& err)
{
    logInfo << "get a err: " << err;
    stop();
}

void RecordHls::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

#endif