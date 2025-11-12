#ifndef Ehome2Parser_H
#define Ehome2Parser_H

#include "Buffer.h"
#include <unordered_map>

// using namespace std;

class Ehome2Parser {
public:
    void parse(const char *data, size_t len);

    void setOnRtpPacket(const std::function<void(const char* data, int len)>& cb);
    void onRtpPacket(const char* data, int len);

private:
    bool _first = true;
    int _offset = 2;
    StringBuffer _remainData;
    std::function<void(const char* data, int len)> _onRtpPacket;
};



#endif //Ehome2Parser_H
