#ifndef Mp4FileWriter_H
#define Mp4FileWriter_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Mp4Muxer.h"
#include "Util/File.h"

using namespace std;

class Mp4FileWriter : public MP4Muxer
{
public:
    using Ptr = shared_ptr<Mp4FileWriter>;

    Mp4FileWriter(int fastFlag, const string& filepath);
    ~Mp4FileWriter();

public:
    void write(const char* data, int size);
    void read(char* data, int size);
    void seek(uint64_t offset);
    size_t tell();

    bool open();

private:
    string _filepath;
    File _file;
};


#endif //Mp4FileWriter_H
