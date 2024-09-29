#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WorkPoller/WorkLoopPool.h"
#include "Common/Config.h"
#include "RecordMp4.h"

using namespace std;

RecordMp4::RecordMp4(const UrlParser& urlParser)
    :_urlParser(urlParser)
{

}

RecordMp4::~RecordMp4()
{
    auto frameSrc = _source.lock();
    if (frameSrc) {
        frameSrc->delConnection(this);
    }
}

bool RecordMp4::start()
{
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();

    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    auto abpath = File::absolutePath(_urlParser.path_, rootPath) + ".mp4";
    logInfo << "get record path: " << abpath;
    // if (!_file.open(abpath, "wb+")) {
    //     return false;
    // }

    _mp4Writer = make_shared<Mp4FileWriter>(0, abpath);
    if (!_mp4Writer->open()) {
        return false;
    }
    _mp4Writer->init();

    weak_ptr<RecordMp4> wSelf = dynamic_pointer_cast<RecordMp4>(shared_from_this());
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

void RecordMp4::onPlayFrame(const FrameMediaSource::Ptr &frameSrc)
{
    auto trackInfos = frameSrc->getTrackInfo();
    for (auto& trackIter : trackInfos) {
        if (trackIter.second->trackType_ == "video") {
            _mp4Writer->addVideoTrack(trackIter.second);
        } else if (trackIter.second->trackType_ == "audio") {
            _mp4Writer->addAudioTrack(trackIter.second);
        }
    }
    weak_ptr<RecordMp4> wSelf = dynamic_pointer_cast<RecordMp4>(shared_from_this());

    if (!_playFrameReader) {
		logInfo << "set _playFrameReader";
		_playFrameReader = frameSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playFrameReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = "record-mp4";
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

            // auto task = make_shared<WorkTask>();
            // task->priority_ = 100;
            // task->func_ = [wSelf, pack](){
            //     auto self = wSelf.lock();
            //     if (!self) {
            //         return ;
            //     }
                logInfo << "pack->keyFrame(): " << pack->keyFrame();
                self->_mp4Writer->inputFrame(pack, pack->getTrackIndex(), pack->keyFrame());
            // };
            // self->_workLoop->addOrderTask(task);
        });
        _source = frameSrc;
	}
}

void RecordMp4::stop()
{
    auto self = dynamic_pointer_cast<RecordMp4>(shared_from_this());
    _loop->async([self](){
        self->_mp4Writer->stop();
    }, true);
    
    if (_onClose) {
        _onClose();
    }
    _stop = true;
}

void RecordMp4::onError(const string& err)
{
    logInfo << "get a err: " << err;
    stop();
}

void RecordMp4::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}
