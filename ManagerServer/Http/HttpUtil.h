#ifndef HttpUtil_h
#define HttpUtil_h

#include <string>

// using namespace std;

class HttpUtil
{
public:
    static std::string getStatusDesc(int status);
    static std::string getMimeType(const std::string& filename);

private:
};

#endif //HttpUtil_h
