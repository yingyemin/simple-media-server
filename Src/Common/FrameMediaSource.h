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

#define RTP_GOP_SIZE 512


class FrameMediaSource : public MediaSource {
public:
    using Ptr = std::shared_ptr<FrameMediaSource>;
    // using RingDataType = shared_ptr<list<FrameBuffer::Ptr> >;
    // using RingDataType = FrameBuffer::Ptr;
    // using RingType = DataQue<RingDataType>;
    
    FrameMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr);
    virtual ~FrameMediaSource();


public:
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr& sink) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;

private:
    int _ring_size = 25;
    FrameRingType::Ptr _ring;
};


#endif /* SRC_FRAME_FRAMEMEDIASOURCE_H_ */
