#ifndef RecordReaderPs_H
#define RecordReaderPs_H

#include "RecordReader.h"

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
    
    void seek(uint64_t timeStamp) override {};
    void pause(bool isPause) override {};
    void scale(float scale) override {};
};

#endif //RecordReaderPs_H
