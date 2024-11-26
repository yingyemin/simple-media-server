#ifndef HttpFile_h
#define HttpFile_h

#include <string>

#include "HttpParser.h"
#include "Util/File.h"

using namespace std;

class HttpFile
{
public:
    HttpFile(const HttpParser& parser);
    ~HttpFile();

public:
    bool isValid();
    bool isFile();
    bool isDir();

    string getIndex();
    string getFilePath();
    int getFileSize();
    void setRange(uint64_t startPos, uint64_t len);
    uint64_t getSize();

    StreamBuffer::Ptr read(int size = 1024 * 1024);

private:
    uint64_t _startPos = 0;
    uint64_t _size = 0;
    uint64_t _readSize = 0;
    string _filePath;
    HttpParser _parser;
    File _file;
};

#endif //HttpFile_h