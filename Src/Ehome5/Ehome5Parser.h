#ifndef Ehome5Parser_H
#define Ehome5Parser_H

#include "Buffer.h"
#include <unordered_map>

using namespace std;

class Ehome5Parser {
public:
    void parse(const char *data, size_t len);

    void setOnEhomePacket(const function<void(const char* data, int len)>& cb);
    void onEhomePacket(const char* data, int len);

private:
    bool _first = true;
    int _offset = 4;
    StringBuffer _remainData;
    function<void(const char* data, int len)> _onEhomePacket;
};



#endif //Ehome5Parser_H
