#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "FlvFileWriter.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

FlvFileWriter::FlvFileWriter(const std::string& file_path):_filepath(file_path)
{

}

FlvFileWriter::~FlvFileWriter()
{}

void FlvFileWriter::write(const char* data, int size)
{
    _file.write(data, size);
}

void FlvFileWriter::read(char* data, int size)
{
    _file.read(data, size);
}

void FlvFileWriter::seek(uint64_t offset)
{
    _file.seek(offset);
}

size_t FlvFileWriter::tell()
{
    return _file.tell();
}

bool FlvFileWriter::open()
{
    if (!_file.open(_filepath, "wb+")) {
        logWarn << "open mp4 file failed: " << _filepath;
        return false;
    }
    return true;
}
void FlvFileWriter::close()
{
    _file.close();
}

void FlvFileWriter::onWriteCustom(const StreamBuffer::Ptr& buffer)
{
    write(buffer->data(), buffer->size());
}

void FlvFileWriter::onWriteCustom(const char* buffer, int len)
{
    write(buffer, len);
}