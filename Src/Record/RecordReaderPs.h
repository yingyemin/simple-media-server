#ifndef RecordReaderPs_H
#define RecordReaderPs_H

#include "RecordReader.h"
#include "Mpeg/PsDemuxer.h"

using namespace std;

class RecordReaderPs : public RecordReader
{
public:
    using Ptr = shared_ptr<RecordReaderPs>;

    RecordReaderPs(const string& path);
    ~RecordReaderPs();

public:
    bool start() override;
    void stop() override;
    void close() override;
    
    void seek(uint64_t timeStamp) override;
    void pause(bool isPause) override;
    void scale(float scale) override;
    uint64_t getDuration() override;

    void getDurationFromFile();

private:
    bool _isReading = false;
    int _state = 0; // 1 : get first stamp; 2: get last stamp
    uint64_t _firstDts = 0;
    uint64_t _duration = 0;
    PsDemuxer::Ptr _demuxer;
};

#endif //RecordReaderPs_H
