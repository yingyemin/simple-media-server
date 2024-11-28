#ifndef TranscodeTask_H
#define TranscodeTask_H

#ifdef ENABLE_FFMPEG

#include "Common/Frame.h"
#include "Common/FrameMediaSource.h"

#include "TranscodeVideo.h"
#include "TranscodeAudio.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

class TranscodeTask : public std::enable_shared_from_this<TranscodeTask>
{
public:
    using Ptr = std::shared_ptr<TranscodeTask>;

    TranscodeTask();
    ~TranscodeTask();

public:
    TranscodeTask::Ptr instance();
    static std::string addTask(const std::string& uri, const std::string& videoCodec, const std::string& audioCodec);

    std::string init(const std::string& uri, const std::string& videoCodec, const std::string& audioCodec);
    static void delTask(const std::string& taskId);

    void onOriginFrameSource(const MediaSource::Ptr &src);
    void close();

private:
    std::string _taskId;
    FrameMediaSource::Wptr _source;
    MediaSource::FrameRingType::DataQueReaderT::Ptr _playReader;
    std::shared_ptr<TranscodeVideo> _transcodeVideo;
    std::shared_ptr<AudioDecoder> _transcodeAudio;

    static std::mutex _mtx;
    static std::unordered_map<string, TranscodeTask::Ptr> _mapTask;
};

#endif

#endif //TranscodeTask_H