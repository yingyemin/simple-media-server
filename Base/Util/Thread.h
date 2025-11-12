#ifndef Thread_H
#define Thread_H

#include <string>

// using namespace std;

class Thread
{
public:
    Thread() {}
    ~Thread() {}

public:
    static int getThreadId();
    static std::string getThreadName();
    static void setThreadName(const std::string& name);

private:
};

#endif