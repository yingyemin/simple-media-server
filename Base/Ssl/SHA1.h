#ifndef SHA1_H_
#define SHA1_H_

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

class SHA1
{
public:
    SHA1() {}
    ~SHA1() {}
    static std::string encode(const std::string& key);
};

#endif //HMACSHA1_H_