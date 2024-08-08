#ifndef Thread_H
#define Thread_H

#include <string>

using namespace std;

class Thread
{
public:
    Thread() {}
    ~Thread() {}

public:
    static int getThreadId();
    static string getThreadName();
    static void setThreadName(const string& name);

private:
};

#endif