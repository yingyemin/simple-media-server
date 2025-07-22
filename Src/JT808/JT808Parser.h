#ifndef JT808Parser_H
#define JT808Parser_H

#include "JT808Packet.h"

class JT808Parser {
public:
    JT808Parser();
    ~JT808Parser();

public:
    void parse(const char* buf, size_t size);

    void setOnJT808Packet(const function<void(const JT808Packet::Ptr& buffer)>& cb);
    void onJT808Packet(const JT808Packet::Ptr& buffer);

private:
    bool parseHeader(const unsigned char* buffer, size_t bufferSize, JT808Header& header);

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    StringBuffer _remainData;
    JT808Packet::Ptr _pkt;
    function<void(const JT808Packet::Ptr& buffer)> _onJT808Packet;
};

#endif //JT808Parser_H
