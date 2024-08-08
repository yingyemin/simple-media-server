#ifndef Base64_h
#define Base64_h

#include <string>

using namespace std;

class Base64
{
public:
    static string encode(const string& str);
    static string decode(const string& str);
    static int indexOfCode(const char c);

};


#endif