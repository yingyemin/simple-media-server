#ifndef ApiUtil_h
#define ApiUtil_h

#include <exception>
#include <string>

#include "Common/json.hpp"

using namespace std;

void checkArgs(const nlohmann::json &allArgs, const std::vector<std::string>& requiredParams);

class ApiException : public exception
{
public:
    ApiException(int errorCode, const string& errorMsg)
    :_errorCode(errorCode)
    ,_errorMsg(errorMsg)
    {}

    const char* what() const noexcept override {
        return _errorMsg.c_str();
    }

private:
    int _errorCode = 200;
    string _errorMsg;
};

int toInt(const nlohmann::json& j);

float toFloat(const nlohmann::json& j);

#endif //ApiUtil_h