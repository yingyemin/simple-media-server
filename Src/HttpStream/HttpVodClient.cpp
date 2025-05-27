#include "HttpVodClient.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

HttpVodClient::HttpVodClient(MediaClientType type, const string& appName, const string& streamName)
    :_type(type)
{
    _loop = EventLoop::getCurrentLoop();
    _localUrlParser.path_ = "/" + appName + "/" + streamName;
    _localUrlParser.protocol_ = PROTOCOL_FRAME;
    _localUrlParser.type_ = "vod";
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

HttpVodClient::~HttpVodClient()
{
    auto src = _source.lock();
    if (src) {
        src->release();
        src->delConnection(this);
    }
}

void HttpVodClient::download()
{
    if (_complete) {
        _downloader = nullptr;
        return ;
    }

    if (_isDownload) {
        return ;
    }

    weak_ptr<HttpVodClient> wSelf = shared_from_this();
    _downloader = make_shared<HttpDownload>();
    _downloader->setOnHttpResponce([wSelf](const char *data, int len){
        auto self = wSelf.lock();
        if (!self || !data || len == 0) {
            return ;
        }

        self->decode(data, len);
    });

    _downloader->setOnClose([wSelf](uint64_t filesize){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        logInfo << "self->_curPos: " << self->_curPos;
        logInfo << "filesize: " << filesize;

        if (self->_curPos >= filesize) {
            self->_complete = true;
        }
        self->_isDownload = false;
    });

    _downloader->setRange(_curPos, _readSize);
    _curPos += _readSize;

    _isDownload = true;
    _downloader->start(_localIp, _localPort, _url, _timeout);
}

bool HttpVodClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    _localIp = localIp;
    _localPort = localPort;
    _url = url;
    _timeout = timeout;

    auto source = MediaSource::getOrCreate(_localUrlParser.path_, DEFAULT_VHOST
                        , PROTOCOL_FRAME, "vod"
                        , [this](){
                            return make_shared<FrameMediaSource>(_localUrlParser, _loop);
                        });

    if (!source) {
        logWarn << "another source is exist: " << _localUrlParser.path_;
        stop();

        return false;
    }

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
    if (!frameSrc) {
        logWarn << "this source is not rtmp: " << source->getProtocol();
        stop();

        return false;
    }

    frameSrc->setOrigin();
    frameSrc->setOriginSocket(_socket);
    frameSrc->setAction(false);
    _source = frameSrc;

    download();

    weak_ptr<HttpVodClient> wSelf = shared_from_this();
    _loop->addTimerTask(40, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        self->checkFrame();

        if (self->_frameList.size() < 100) {
            self->download();
        }

        return 40;
    }, nullptr);

    return true;
}

void HttpVodClient::stop()
{
    close();
}

void HttpVodClient::pause()
{

}

void HttpVodClient::close()
{
    if (_onClose) {
        _onClose();
    }
}

void HttpVodClient::onError(const string& err)
{
    logInfo << "get a error: " << err;

    close();
}

void HttpVodClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void HttpVodClient::getProtocolAndType(string& protocol, MediaClientType& type)
{
    protocol = "ps";
    type = _type;
}

void HttpVodClient::onFrame(const FrameBuffer::Ptr& frame)
{
    _frameList.emplace_back(frame);
}

void HttpVodClient::checkFrame()
{
    // logInfo << "check frame: " << _frameList.size();
    if (_complete && _frameList.size() == 0) {
        stop();
    }

    auto source = _source.lock();
    if (!source) {
        return ;
    }

    while (_frameList.size() > 0) {
        auto frameIt = _frameList.front();
        if (!frameIt) {
            break;
        }

        if (_mapTrackInfo.size() == 2) {
            if (frameIt->getTrackType() == AudioTrackType) {
                source->onFrame(frameIt);
                _frameList.pop_front();
                continue ;
            }
        }

        if (_first) {
            _first = false;
            _basePts = frameIt->pts();
            _clock.update();
        }

        // logInfo << "frameIt->pts(): " << frameIt->pts();
        // logInfo << "_basePts: " << _basePts;
        // logInfo << "_clock.startToNow(): " << _clock.startToNow();
        if (frameIt->pts() < _clock.startToNow() + _basePts) {
            source->onFrame(frameIt);
            _frameList.pop_front();
        } else {
            break;
        }
    }
}