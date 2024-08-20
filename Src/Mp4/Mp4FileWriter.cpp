#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Mp4FileWriter.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

Mp4FileWriter::Mp4FileWriter(int fastFlag, const string& filepath)
    :MP4Muxer(fastFlag)
    ,_filepath(filepath)
{

}

Mp4FileWriter::~Mp4FileWriter()
{}

void Mp4FileWriter::write(const char* data, int size)
{
    _file.write(data, size);
}

void Mp4FileWriter::read(char* data, int size)
{
    _file.read(data, size);
}

void Mp4FileWriter::seek(uint64_t offset)
{
    _file.seek(offset);
}

size_t Mp4FileWriter::tell()
{
    return _file.tell();
}

bool Mp4FileWriter::open()
{
    if (_file.open(_filepath, "wb+")) {
        logWarn << "open mp4 file failed: " << _filepath;
        return false;
    }

    return true;
}