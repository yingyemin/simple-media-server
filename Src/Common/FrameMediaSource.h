    /*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef SRC_FRAME_FRAMEMEDIASOURCE_H_
#define SRC_FRAME_FRAMEMEDIASOURCE_H_

#include <mutex>
#include <string>
#include <memory>
#include <functional>
#include "MediaSource.h"
#include "Track.h"
#include "StampAdjust.h"
#include "Util/TimeClock.h"

#define RTP_GOP_SIZE 512


class FrameMediaSource : public MediaSource {
public:
    using Ptr = std::shared_ptr<FrameMediaSource>;
    using Wptr = weak_ptr<FrameMediaSource>;
    
    FrameMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr);
    virtual ~FrameMediaSource();


public:
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr& sink) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;

    void inputFrame(const FrameBuffer::Ptr& frame);
    FrameRingType::Ptr getRing() {return _ring;}
    float getFps() {return _fps;}
    uint64_t getLastFrameTime() {return TimeClock::now() - _lastFrameTime;}
    int getLastGopTime() {return _gopTime;}
    uint64_t getLastKeyframeTime() {return TimeClock::now() - _lastKeyframeTime;}

private:
    bool _sendConfig = false;
    int _ring_size = 25;
    int _gopTime = 0;
    float _fps = 0;
    uint64_t _frameCount = 0;
    uint64_t _lastFrameTime = 0;
    uint64_t _lastKeyframeTime = 0;
    TimeClock _frameClock;
    shared_ptr<StampAdjust> _videoStampAdjust;
    shared_ptr<StampAdjust> _audioStampAdjust;
    FrameRingType::Ptr _ring;
    FrameBuffer::Ptr _frame;
};


#endif /* SRC_FRAME_FRAMEMEDIASOURCE_H_ */
