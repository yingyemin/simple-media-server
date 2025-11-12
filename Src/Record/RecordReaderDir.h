#ifndef RecordReaderDir_H
#define RecordReaderDir_H

#include "RecordReader.h"

// using namespace std;

class RecordReaderDir : public RecordReader
{
public:
    using Ptr = std::shared_ptr<RecordReaderDir>;

    RecordReaderDir(const std::string& path);
    ~RecordReaderDir();

public:
    bool start() override;
    void stop() override;
    void close() override;
    
    void seek(uint64_t timeStamp) override;
    void pause(bool isPause) override;
    void scale(float scale) override;
    uint64_t getDuration() override;

private:
    bool _isReading = false;
    int _state = 0; // 1 : get first stamp; 2: get last stamp
    int _dirIndex = 0;
    int _dirLoopCount = 0;
    uint64_t _firstDts = 0;
    uint64_t _duration = 0;
    RecordReader::Ptr _reader;
    std::vector<RecordReader::Ptr> _vecDemuxer;
};

#endif //RecordReaderDir_H
