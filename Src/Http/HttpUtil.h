#ifndef HttpUtil_h
#define HttpUtil_h

#include <string>

using namespace std;

class HttpUtil
{
public:
    static string getStatusDesc(int status);
    static string getMimeType(const std::string& filename);

private:
};

#endif //HttpApi_h