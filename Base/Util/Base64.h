#ifndef Base64_h
#define Base64_h

#include <string>

// using namespace std;

class Base64
{
public:
    static std::string encode(const std::string& str);
    static std::string decode(const std::string& str);
    static int indexOfCode(const char c);

};


#endif