#ifndef Path_H
#define Path_H

#include <string>

// using namespace std;

class Path
{
public:
    Path() {}
    ~Path() {}

public:
    static std::string exePath();
    static std::string exeDir();
    static std::string exeName();

    static std::string getFileName(const std::string& path);

private:
};

#endif