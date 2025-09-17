#if defined(ENABLE_FFMPEG)
#include "VideoStack.h"
#include "Transcode.h"
#include "Log/Logger.h"
#include "Common/Define.h"
#include "Util/String.h"
#include <fstream>
#include <libavutil/pixfmt.h>
#include <memory>
#include <mutex>

// ITU-R BT.601
// #define  RGB_TO_Y(R, G, B) ((( 66 * (R) + 129 * (G) +  25 * (B)+128) >> 8)+16)
// #define  RGB_TO_U(R, G, B) (((-38 * (R) -  74 * (G) + 112 * (B)+128) >> 8)+128)
// #define  RGB_TO_V(R, G, B) (((112 * (R) -  94 * (G) -  18 * (B)+128) >> 8)+128)

// ITU-R BT.709
#define RGB_TO_Y(R, G, B) (((47 * (R) + 157 * (G) + 16 * (B) + 128) >> 8) + 16)
#define RGB_TO_U(R, G, B) (((-26 * (R) - 87 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB_TO_V(R, G, B) (((112 * (R) - 102 * (G) - 10 * (B) + 128) >> 8) + 128)

VideoStackManager& VideoStackManager::Instance()
{
    static VideoStackManager instance;
    return instance;
}

Param::~Param() { VideoStackManager::Instance().unrefChannel(id, width, height, pixfmt); }

Channel::Channel(const std::string& id, int width, int height, AVPixelFormat pixfmt)
    : _id(id), _width(width), _height(height), _pixfmt(pixfmt) {
    _tmp = std::make_shared<FFmpegFrame>();

    _tmp->get()->width = _width;
    _tmp->get()->height = _height;
    _tmp->get()->format = _pixfmt;

    av_frame_get_buffer(_tmp->get(), 32);

    memset(_tmp->get()->data[0], 0, _tmp->get()->linesize[0] * _height);
    memset(_tmp->get()->data[1], 0, _tmp->get()->linesize[1] * _height / 2);
    memset(_tmp->get()->data[2], 0, _tmp->get()->linesize[2] * _height / 2);

    auto frame = VideoStackManager::Instance().getBgImg();
    _sws = std::make_shared<FFmpegSws>(_pixfmt, _width, _height);

    if (frame) {
        _tmp = _sws->inputFrame(frame);
    }
}

void Channel::addParam(const std::weak_ptr<Param>& p) {
    std::lock_guard<std::recursive_mutex> lock(_mx);
    _params.push_back(p);
}

void Channel::onFrame(const FFmpegFrame::Ptr& frame) {
    std::weak_ptr<Channel> wSelf = shared_from_this();

    if (!_workLoop) {
        _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    }

    WorkTask::Ptr task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [wSelf, frame](){
        auto self = wSelf.lock();
        if (!self) { return; }
        self->_tmp = self->_sws->inputFrame(frame);

        self->forEachParam([self](const Param::Ptr& p) { self->fillBuffer(p); });
    };
    // logInfo << "add work task";
    _workLoop->addOrderTask(task);
}

void Channel::forEachParam(const std::function<void(const Param::Ptr&)>& func) {
    for (auto& wp : _params) {
        if (auto sp = wp.lock()) { func(sp); }
    }
}

void Channel::fillBuffer(const Param::Ptr& p) {
    if (auto buf = p->weak_buf.lock()) { copyData(buf, p); }
}

void Channel::copyData(const FFmpegFrame::Ptr& buf, const Param::Ptr& p) {

    switch (p->pixfmt) {
        case AV_PIX_FMT_YUV420P: {
            for (int i = 0; i < p->height; i++) {
                memcpy(buf->get()->data[0] + buf->get()->linesize[0] * (i + p->posY) + p->posX,
                       _tmp->get()->data[0] + _tmp->get()->linesize[0] * i, _tmp->get()->width);
            }
            // 确保height为奇数时，也能正确的复制到最后一行uv数据  [AUTO-TRANSLATED:69895ea5]
            // Ensure that the uv data can be copied to the last line correctly when height is odd
            for (int i = 0; i < (p->height + 1) / 2; i++) {
                // U平面  [AUTO-TRANSLATED:8b73dc2d]
                // U plane
                memcpy(buf->get()->data[1] + buf->get()->linesize[1] * (i + p->posY / 2) +
                           p->posX / 2,
                       _tmp->get()->data[1] + _tmp->get()->linesize[1] * i, _tmp->get()->width / 2);

                // V平面  [AUTO-TRANSLATED:8fa72cc7]
                // V plane
                memcpy(buf->get()->data[2] + buf->get()->linesize[2] * (i + p->posY / 2) +
                           p->posX / 2,
                       _tmp->get()->data[2] + _tmp->get()->linesize[2] * i, _tmp->get()->width / 2);
            }
            break;
        }
        case AV_PIX_FMT_NV12: {
            // TODO: 待实现  [AUTO-TRANSLATED:247ec1df]
            // TODO: To be implemented
            break;
        }

        default: logWarn << "No support pixformat: " << av_get_pix_fmt_name(p->pixfmt); break;
    }
}
void StackPlayer::addChannel(const std::weak_ptr<Channel>& chn) {
    auto chnS = chn.lock();
    if (_frame && chnS) {
        chnS->onFrame(_frame);
    }
    std::lock_guard<std::recursive_mutex> lock(_mx);
    _channels.push_back(chn);
}

void StackPlayer::play()
{
    if (endWith(_url, ".jpg") || endWith(_url, ".jpeg") || endWith(_url, ".png")) {
        FfmpegFormat::Ptr fmt = make_shared<FfmpegFormat>(_url);
        if (fmt->open()) {
            auto packet = fmt->readPacket();
            if (packet) {
                FFmpegDecoder::Ptr dec = make_shared<FFmpegDecoder>(fmt);
                dec->setOnDecode([this](const FFmpegFrame::Ptr& frame){
                    logInfo << "VideoStack::onDecode, pts: " << frame->get()->pts;
                    _frame = frame;
                    onFrame(frame);
                });
                dec->inputFrame(packet, true, false);
            }
        }
        return ;
    }
    std::weak_ptr<StackPlayer> wSelf = shared_from_this();

    UrlParser urlParser;
    urlParser.protocol_ = PROTOCOL_FRAME;
    urlParser.path_ = _url;
    urlParser.vhost_ = DEFAULT_VHOST;
    urlParser.type_ = "trans-source";
    MediaSource::getOrCreateAsync(urlParser.path_, urlParser.vhost_, urlParser.protocol_, urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        if (!src) {
            logError << "get src failed";
            return ;
        }

        src->getLoop()->async([wSelf, src](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->onOriginFrameSource(src);
        }, true);
    }, 
    [wSelf, urlParser]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<FrameMediaSource>(urlParser, nullptr);
    }, this);
}

void StackPlayer::onOriginFrameSource(const MediaSource::Ptr &src)
{
    std::weak_ptr<StackPlayer> wSelf = shared_from_this();

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(src);
    _originSource = frameSrc;

    auto tracks = frameSrc->getTrackInfo();
    // auto audioTrack = std::dynamic_pointer_cast<AudioTrack>(strongPlayer->getTrack(TrackAudio, false));

    for (auto track : tracks) {
        // TODO:添加使用显卡还是cpu解码的判断逻辑  [AUTO-TRANSLATED:44bef37a]
        // TODO: Add logic to determine whether to use GPU or CPU decoding
        if (track.second->trackType_ == "video") {
            _videoDecoder = std::make_shared<FFmpegDecoder>(
            track.second, 0, std::vector<std::string>{"h264", "hevc"});

            _videoDecoder->setOnDecode([wSelf](const FFmpegFrame::Ptr& frame) mutable {
                auto self = wSelf.lock();
                if (!self) { return; }

                self->onFrame(frame);
            });
        } else if (track.second->trackType_ == "audio") {
            _audioDecoder = std::make_shared<FFmpegDecoder>(
            track.second, 0, std::vector<std::string>{"aac"});
            _audioDecoder->setOnDecode([wSelf](const FFmpegFrame::Ptr& frame) mutable {
                auto self = wSelf.lock();
                if (!self) { return; }

                self->onFrame(frame);
            });
        }
        
    }
    
    _playReader = frameSrc->getRing()->attach(frameSrc->getLoop(), true);
    _playReader->setGetInfoCB([wSelf]() {
        auto self = wSelf.lock();
        ClientInfo ret;
        if (!self) {
            return ret;
        }
        // ret.ip_ = self->_socket->getLocalIp();
        // ret.port_ = self->_socket->getLocalPort();
        ret.protocol_ = PROTOCOL_FRAME;
        // ret.bitrate_ = self->_lastBitrate;
        ret.close_ = [wSelf](){
            auto self = wSelf.lock();
            if (self) {
                // self->close();
            }
        };
        return ret;
    });
    _playReader->setDetachCB([wSelf]() {
        auto strong_self = wSelf.lock();
        if (!strong_self) {
            return;
        }
        // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
        // strong_self->close();
    });
    // logInfo << "setReadCB =================";
    _playReader->setReadCB([wSelf](const MediaSource::FrameRingDataType &pack) {
        // logInfo << "get a frame from origin source";
        auto self = wSelf.lock();
        if (!self/* || pack->empty()*/) {
            return;
        }

         if (!self->_workLoop) {
            self->_workLoop = WorkLoopPool::instance()->getLoopByCircle();
        }

        WorkTask::Ptr task = make_shared<WorkTask>();
        task->priority_ = 100;
        task->func_ = [wSelf, pack](){
            auto self = wSelf.lock();
            if (!self) { return; }
            // logInfo << "start decode ===========";
            if (pack->_trackType == VideoTrackType) {
                self->_videoDecoder->inputFrame(pack, false, true);
            } else if (pack->_trackType == AudioTrackType) {
                self->_audioDecoder->inputFrame(pack, false, true);
            }
        };
        // logInfo << "add work task";
        self->_workLoop->addOrderTask(task);
    });

    frameSrc->addOnDetach(this, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        // self->close();
    });
}

void StackPlayer::onFrame(const FFmpegFrame::Ptr& frame) {
    std::lock_guard<std::recursive_mutex> lock(_mx);
    for (auto& weak_chn : _channels) {
        if (auto chn = weak_chn.lock()) { chn->onFrame(frame); }
    }
}

void StackPlayer::onDisconnect() {
    std::lock_guard<std::recursive_mutex> lock(_mx);
    for (auto& weak_chn : _channels) {
        if (auto chn = weak_chn.lock()) {
            auto frame = VideoStackManager::Instance().getBgImg();
            chn->onFrame(frame);
        }
    }
}

void StackPlayer::rePlay(const std::string& url) {
    _failedCount++;
    auto delay = _failedCount * 3 * 1000; 
    if (delay > 60 * 1000) {
        delay = 60 * 1000;
    } else if (delay < 2 * 1000) {
        delay = 2 * 1000;
    }
    // std::weak_ptr<StackPlayer> weakSelf = shared_from_this();
    // _timer = std::make_shared<toolkit::Timer>(delay / 1000.0f, [weakSelf, url]() {
    //     auto self = weakSelf.lock();
    //     if (!self) {}
    //     logWarn << "replay [" << self->_failedCount << "]:" << url;
    //     self->_player->play(url);
    //     return false;
    // }, nullptr);
}

VideoStack::VideoStack(const std::string& id, int width, int height, AVPixelFormat pixfmt,
                       float fps, int bitRate)
    : _id(id), _width(width), _height(height), _pixfmt(pixfmt), _fps(fps), _bitRate(bitRate) {

    _buffer = std::make_shared<FFmpegFrame>();

    _buffer->get()->width = _width;
    _buffer->get()->height = _height;
    _buffer->get()->format = _pixfmt;

    av_frame_get_buffer(_buffer->get(), 32);

    UrlParser parser;
    parser.path_ = id;
    parser.type_ = DEFAULT_TYPE;
    parser.protocol_ = PROTOCOL_FRAME;
    parser.vhost_ = DEFAULT_VHOST;
    // _frameSrc = std::make_shared<FrameMediaSource>(parser, EventLoop::getCurrentLoop());
    auto frameSrc = MediaSource::getOrCreate(id, DEFAULT_VHOST, PROTOCOL_FRAME, DEFAULT_TYPE, [parser](){
        return make_shared<FrameMediaSource>(parser, EventLoop::getCurrentLoop());
    });

    _frameSrc = dynamic_pointer_cast<FrameMediaSource>(frameSrc);

    _videoTrackInfo = TrackInfo::createTrackInfo("h264");
    // _videoTrackInfo->codec_ = "h264";
    // _videoTrackInfo->trackType_ = "video";
    // _videoTrackInfo->index_ = VideoTrackType;
    _videoTrackInfo->_width = _width;
    _videoTrackInfo->_height = _height;
    // _videoTrackInfo->payloadType_ = 96;

    _frameSrc->addTrack(_videoTrackInfo);
    _frameSrc->setOrigin();

    // dev->initAudio();         //TODO:音频  [AUTO-TRANSLATED:adc5658b]
    // dev->initAudio();         //TODO: Audio

    _isExit = false;
}

VideoStack::~VideoStack() {
    _isExit = true;
    if (_thread.joinable()) { _thread.join(); }
}

void VideoStack::setParam(const Params& params) {
    if (_params) {
        for (auto& p : (*_params)) {
            if (!p) continue;
            p->weak_buf.reset();
        }
    }

    initBgColor();
    for (auto& p : (*params)) {
        if (!p) continue;
        p->weak_buf = _buffer;
        if (auto chn = p->weak_chn.lock()) {
            chn->addParam(p);
            chn->fillBuffer(p);
        }
    }
    _params = params;
}

void VideoStack::setBgImg(const std::string& bgImg) {
    _bgImg = bgImg;
}

void VideoStack::start() {
    _thread = std::thread([&]() {
        uint64_t pts = 0;
        int frameInterval = 1000 / _fps;
        auto lastEncTP = std::chrono::steady_clock::now();
        while (!_isExit) {
            if (std::chrono::steady_clock::now() - lastEncTP >
                std::chrono::milliseconds(frameInterval)) {
                lastEncTP = std::chrono::steady_clock::now();

                if (!_encoder) {
                    EncodeOption opt;
                    opt.width = _width;
                    opt.height = _height;
                    opt.gop = _gop;
                    _encoder = std::make_shared<FFmpegEncoder>(_videoTrackInfo, opt);
                    _encoder->setOnEncode([this](const FrameBuffer::Ptr& frame){
                        logInfo << "VideoStack::onEncode";
                        frame->split([this](const FrameBuffer::Ptr& subFrame) {
                            logInfo << "nal type: " << (int)subFrame->getNalType() << ", pts: " << subFrame->pts();
                            _frameSrc->onFrame(subFrame);
                        });
                    });
                }

                _encoder->inputFrame(_buffer, false);

                // _dev->inputYUV((char**)_buffer->get()->data, _buffer->get()->linesize, pts);
                // pts += frameInterval;
            }
        }
    });
}

void VideoStack::initBgColor() {
    // 填充底色  [AUTO-TRANSLATED:ee9bbd46]
    // Fill the background color
    if (!_bgImg.empty()) {
        FfmpegFormat::Ptr fmt = make_shared<FfmpegFormat>(_bgImg);
        if (fmt->open()) {
            auto packet = fmt->readPacket();
            if (packet) {
                FFmpegDecoder::Ptr dec = make_shared<FFmpegDecoder>(fmt);
                dec->setOnDecode([this](const FFmpegFrame::Ptr& frame){
                    logInfo << "VideoStack::onDecode, pts: " << frame->get()->pts;
                    _buffer = frame;
                });
                dec->inputFrame(packet, true, false);

                return ;
            }
        }
    }
    
    auto R = 20;
    auto G = 20;
    auto B = 20;

    double Y = RGB_TO_Y(R, G, B);
    double U = RGB_TO_U(R, G, B);
    double V = RGB_TO_V(R, G, B);

    memset(_buffer->get()->data[0], Y, _buffer->get()->linesize[0] * _height);
    memset(_buffer->get()->data[1], U, _buffer->get()->linesize[1] * _height / 2);
    memset(_buffer->get()->data[2], V, _buffer->get()->linesize[2] * _height / 2);
}

Channel::Ptr VideoStackManager::getChannel(const std::string& id, int width, int height,
                                           AVPixelFormat pixfmt) {

    std::lock_guard<std::recursive_mutex> lock(_mx);
    auto key = id + std::to_string(width) + std::to_string(height) + std::to_string(pixfmt);
    auto it = _channelMap.find(key);
    if (it != _channelMap.end()) { return it->second->acquire(); }

    return createChannel(id, width, height, pixfmt);
}

void VideoStackManager::unrefChannel(const std::string& id, int width, int height,
                                     AVPixelFormat pixfmt) {

    std::lock_guard<std::recursive_mutex> lock(_mx);
    auto key = id + std::to_string(width) + std::to_string(height) + std::to_string(pixfmt);
    auto chn_it = _channelMap.find(key);
    if (chn_it != _channelMap.end() && chn_it->second->dispose()) {
        _channelMap.erase(chn_it);

        auto player_it = _playerMap.find(id);
        if (player_it != _playerMap.end() && player_it->second->dispose()) {
            _playerMap.erase(player_it);
        }
    }
}

int VideoStackManager::startVideoStack(const nlohmann::json& json) {

    std::string id;
    int width, height;
    auto params = parseParams(json, id, width, height);

    if (!params) {
        logError << "Videostack parse params failed!";
        return -1;
    }

    auto stack = std::make_shared<VideoStack>(id, width, height);

    for (auto& p : (*params)) {
        if (!p) continue;
        p->weak_chn = getChannel(p->id, p->width, p->height, p->pixfmt);
    }

    stack->setParam(params);
    stack->start();

    std::lock_guard<std::recursive_mutex> lock(_mx);
    _stackMap[id] = stack;
    return 0;
}

int VideoStackManager::startCustomVideoStack(const std::string& id, int width, int height, const std::string& bgImg, const Params& params) {
    if (!params) {
        logError << "Videostack parse params failed!";
        return -1;
    }

    auto stack = std::make_shared<VideoStack>(id, width, height);

    for (auto& p : (*params)) {
        if (!p) continue;
        p->weak_chn = getChannel(p->id, p->width, p->height, p->pixfmt);
    }

    stack->setBgImg(bgImg);
    stack->setParam(params);
    stack->start();

    std::lock_guard<std::recursive_mutex> lock(_mx);
    _stackMap[id] = stack;
    return 0;
}

int VideoStackManager::resetVideoStack(const nlohmann::json& json) {
    std::string id;
    int width, height;
    auto params = parseParams(json, id, width, height);

    if (!params) {
        logError << "Videostack parse params failed!";
        return -1;
    }

    VideoStack::Ptr stack;
    {
        std::lock_guard<std::recursive_mutex> lock(_mx);
        auto it = _stackMap.find(id);
        if (it == _stackMap.end()) { return -2; }
        stack = it->second;
    }

    for (auto& p : (*params)) {
        if (!p) continue;
        p->weak_chn = getChannel(p->id, p->width, p->height, p->pixfmt);
    }

    stack->setParam(params);
    return 0;
}

int VideoStackManager::stopVideoStack(const std::string& id) {
    std::lock_guard<std::recursive_mutex> lock(_mx);
    auto it = _stackMap.find(id);
    if (it != _stackMap.end()) {
        _stackMap.erase(it);
        logInfo << "VideoStack stop: " << id;
        return 0;
    }
    return -1;
}

FFmpegFrame::Ptr VideoStackManager::getBgImg() { return _bgImg; }

template<typename T> T getJsonValue(const nlohmann::json& param, const std::string& key) {
    if (param.find(key) == param.end()) {
        throw runtime_error("VideoStack parseParams missing required field: " + key);
    }
    return param[key].get<T>();
}

Params VideoStackManager::parseParams(const nlohmann::json& param, std::string& id, int& width,
                                      int& height) {

    id = param.value("id", "");
    width = stoi(param.value("width", "0"));
    height = stoi(param.value("height", "0"));
    int rows = stoi(param.value("row", "0"));// 行数
    int cols = stoi(param.value("col", "0"));// 列数

    float gapv = stof(param.value("gapv", "0.0f"));// 垂直间距
    float gaph = stof(param.value("gaph", "0.0f"));// 水平间距

    // 单个间距  [AUTO-TRANSLATED:e1b9b5b6]
    // Single spacing
    int gaphPix = static_cast<int>(round(width * gaph));
    int gapvPix = static_cast<int>(round(height * gapv));

    // 根据间距计算格子宽高  [AUTO-TRANSLATED:b9972498]
    // Calculate the width and height of the grid according to the spacing
    int gridWidth = cols > 1 ? (width - gaphPix * (cols - 1)) / cols : width;
    int gridHeight = rows > 1 ? (height - gapvPix * (rows - 1)) / rows : height;

    auto params = std::make_shared<std::vector<Param::Ptr>>(rows * cols);

    nlohmann::json urlJson;
    if (param.find("url") != param.end()) {
        if (param["url"].is_string()) {
            urlJson = nlohmann::json::parse(param.value("url", ""));
        } else if (param["url"].is_array()) {
            urlJson = param["url"];
        }
    }

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            std::string url = urlJson[row][col];

            auto param = std::make_shared<Param>();
            param->posX = gridWidth * col + col * gaphPix;
            param->posY = gridHeight * row + row * gapvPix;
            param->width = gridWidth;
            param->height = gridHeight;
            param->id = url;

            (*params)[row * cols + col] = param;
        }
    }

    // 判断是否需要合并格子 （焦点屏）  [AUTO-TRANSLATED:bfa14430]
    // Determine whether to merge grids (focus screen)
    if (param.find("span") != param.end() && param["span"].is_array() && param["span"].size() > 0) {
        for (const auto& subArray : param["span"]) {
            if (!subArray.is_array() || subArray.size() != 2) {
                throw runtime_error("Incorrect 'span' sub-array format in JSON");
            }
            std::array<int, 4> mergePos;
            unsigned int index = 0;

            for (const auto& innerArray : subArray) {
                if (!innerArray.is_array() || innerArray.size() != 2) {
                    throw runtime_error("Incorrect 'span' inner-array format in JSON");
                }
                for (const auto& number : innerArray) {
                    if (index < mergePos.size()) { mergePos[index++] =  number.get<int>(); }
                }
            }

            for (int i = mergePos[0]; i <= mergePos[2]; i++) {
                for (int j = mergePos[1]; j <= mergePos[3]; j++) {
                    if (i == mergePos[0] && j == mergePos[1]) {
                        (*params)[i * cols + j]->width =
                            (mergePos[3] - mergePos[1] + 1) * gridWidth +
                            (mergePos[3] - mergePos[1]) * gapvPix;
                        (*params)[i * cols + j]->height =
                            (mergePos[2] - mergePos[0] + 1) * gridHeight +
                            (mergePos[2] - mergePos[0]) * gaphPix;
                    } else {
                        (*params)[i * cols + j] = nullptr;
                    }
                }
            }
        }
    }
    return params;
}

bool VideoStackManager::loadBgImg(const std::string& path) {
    _bgImg = std::make_shared<FFmpegFrame>();

    _bgImg->get()->width = 1280;
    _bgImg->get()->height = 720;
    _bgImg->get()->format = AV_PIX_FMT_YUV420P;

    av_frame_get_buffer(_bgImg->get(), 32);

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) { return false; }

    file.read((char*)_bgImg->get()->data[0],
              _bgImg->get()->linesize[0] * _bgImg->get()->height);// Y
    file.read((char*)_bgImg->get()->data[1],
              _bgImg->get()->linesize[1] * _bgImg->get()->height / 2);// U
    file.read((char*)_bgImg->get()->data[2],
              _bgImg->get()->linesize[2] * _bgImg->get()->height / 2);// V
    return true;
}

void VideoStackManager::clear() { _stackMap.clear(); }

Channel::Ptr VideoStackManager::createChannel(const std::string& id, int width, int height,
                                              AVPixelFormat pixfmt) {

    std::lock_guard<std::recursive_mutex> lock(_mx);
    StackPlayer::Ptr player;
    auto it = _playerMap.find(id);
    if (it != _playerMap.end()) {
        player = it->second->acquire();
    } else {
        player = createPlayer(id);
    }

    auto refChn = std::make_shared<RefWrapper<Channel::Ptr>>(
        std::make_shared<Channel>(id, width, height, pixfmt));
    auto chn = refChn->acquire();
    player->addChannel(chn);

    _channelMap[id + std::to_string(width) + std::to_string(height) + std::to_string(pixfmt)] =
        refChn;
    return chn;
}

StackPlayer::Ptr VideoStackManager::createPlayer(const std::string& id) {
    std::lock_guard<std::recursive_mutex> lock(_mx);
    auto refPlayer =
        std::make_shared<RefWrapper<StackPlayer::Ptr>>(std::make_shared<StackPlayer>(id));
    _playerMap[id] = refPlayer;

    auto player = refPlayer->acquire();
    if (!id.empty()) { player->play(); }

    return player;
}
#endif
