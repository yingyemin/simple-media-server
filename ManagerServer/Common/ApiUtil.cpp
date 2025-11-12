#include "ApiUtil.h"

using namespace std;

void checkArgs(const nlohmann::json &allArgs, const std::vector<std::string>& requiredParams)
{
    string lossParams = "";
    for (const auto& param : requiredParams) {
        if (allArgs.find(param) == allArgs.end() || allArgs[param].is_null() || allArgs[param].empty() || (allArgs[param].is_string() && allArgs.value(param, "") == "")) {
            lossParams += param + " ";
        }
    }
    if (!lossParams.empty()) {
        string errorMsg = "缺少必要参数:" + lossParams;
        throw ApiException(302, errorMsg);
        return ;
    }
}

int toInt(const nlohmann::json& j)
{
    if (j.is_string()) {
        return stoi(j.get<string>());
    }
    return j;
}

float toFloat(const nlohmann::json& j)
{
    if (j.is_string()) {
        return stof(j.get<string>());
    }
    return j;
}

int getInt(const nlohmann::json& j, const string& key, int defaultValue)
{
    if (j.find(key) == j.end()) {
        return defaultValue;
    }

    auto v = j[key];
    if (v.is_string()) {
        return stoi(v.get<string>());
    }
    return v;
}

float getFloat(const nlohmann::json& j, const string& key, float defaultValue)
{
    if (j.find(key) == j.end()) {
        return defaultValue;
    }

    auto v = j[key];
    if (v.is_string()) {
        return stof(v.get<string>());
    }
    return v;
}