#ifndef RecordReaderRecord_H
#define RecordReaderRecord_H

#include "RecordReader.h"

using namespace std;

class RecordReaderRecord : public RecordReader
{
public:
    using Ptr = shared_ptr<RecordReaderRecord>;

    RecordReaderRecord(const string& path);
    ~RecordReaderRecord();

public:
    bool start() override;
    void stop() override;
    void close() override;
    
    void seek(uint64_t timeStamp) override;
    void pause(bool isPause) override;
    void scale(float scale) override;
    uint64_t getDuration() override;

private:
    bool _firstAddTrack = true;
    bool _isReading = false;
    int _state = 0; // 1 : get first stamp; 2: get last stamp
    int _dirIndex = 0;
    int _dirLoopCount = 0;
    uint64_t _firstDts = 0;
    uint64_t _duration = 0;
    uint64_t _startTime = 0;
    uint64_t _endTime = 0;
    RecordReader::Ptr _reader;
    vector<RecordReader::Ptr> _vecDemuxer;
    vector<uint64_t> _vecFileStartTime;
};

#endif //RecordReaderRecord_H
