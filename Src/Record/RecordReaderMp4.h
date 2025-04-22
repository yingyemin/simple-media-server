#ifndef RecordReaderMp4_H
#define RecordReaderMp4_H

#include "RecordReader.h"

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
    
    void seek(uint64_t timeStamp) override {};
    void pause(bool isPause) override {};
    void scale(float scale) override {};
};

#endif //RecordReaderMp4_H
