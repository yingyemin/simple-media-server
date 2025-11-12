#ifndef FLVFileWriter_H
#define FLVFileWriter_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "FlvMuxer.h"
#include "Util/File.h"

// using namespace std;

class FlvFileWriter : public FlvMuxer
{
public:
    using Ptr = std::shared_ptr<FlvFileWriter>;

    FlvFileWriter(const std::string& file_path="");
    ~FlvFileWriter();

public:
    virtual void onWriteCustom(const char* buffer, int len);
    void onWriteCustom(const StreamBuffer::Ptr& buffer);
    void write(const char* data, int size);
    void read(char* data, int size);
    void seek(uint64_t offset);
    size_t tell();

    bool open();
    void close();
private:
    std::string _filepath;
    File _file;
};


#endif //FlvFileWriter_H
