#ifndef Path_H
#define Path_H

#include <string>

using namespace std;

class Path
{
public:
    Path() {}
    ~Path() {}

public:
    static string exePath();
    static string exeDir();
    static string exeName();

    static string getFileName(const std::string& path);

private:
};

#endif