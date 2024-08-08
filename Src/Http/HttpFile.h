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

    StreamBuffer::Ptr read(int size = 1024 * 1024);

private:
    string _filePath;
    HttpParser _parser;
    File _file;
};

#endif //HttpFile_h