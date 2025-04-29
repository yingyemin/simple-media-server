#ifndef RecordReaderMp4_H
#define RecordReaderMp4_H

#ifdef ENABLE_MP4

#include "RecordReader.h"
#include "Mp4/Mp4FileReader.h"

using namespace std;

class RecordReaderMp4 : public RecordReader
{
public:
    using Ptr = shared_ptr<RecordReaderMp4>;

    RecordReaderMp4(const string& path);
    ~RecordReaderMp4();

public:
    bool start() override;
    void stop() override;
    void close() override;
    
    void seek(uint64_t timeStamp) override;
    void pause(bool isPause) override;
    void scale(float scale) override;
    uint64_t getDuration() override;

private:
    bool initMp4();

private:
    Mp4FileReader::Ptr _mp4Reader;
};

#endif
#endif //RecordReaderMp4_H
