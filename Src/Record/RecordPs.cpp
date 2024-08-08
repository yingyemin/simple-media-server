#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WorkPoller/WorkLoopPool.h"
#include "Common/Config.h"
#include "RecordPs.h"

using namespace std;

RecordPs::RecordPs(const UrlParser& urlParser)
    :_urlParser(urlParser)
{

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

    auto abpath = File::absolutePath(_urlParser.path_, rootPath) + ".ps";
    logInfo << "get record path: " << abpath;
    if (!_file.open(abpath)) {
        return false;
    }

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
                task->func_ = [wSelf, buffer](){
                    auto self = wSelf.lock();
                    if (!self) {
                        return ;
                    }
                    self->_file.write(buffer);
                };
                self->_workLoop->addOrderTask(task);
			}
		});
        _source = psSrc;
	}
}

void RecordPs::stop()
{
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
