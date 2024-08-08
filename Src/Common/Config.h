#ifndef Config_H
#define Config_H

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "json.hpp"
#include "Util/Variant.h"

using json = nlohmann::json;
using namespace std;

class Config : public enable_shared_from_this<Config>
{
public:
    using Ptr = shared_ptr<Config>;

    Config() {}
    ~Config() {}

public:
    static Config::Ptr instance();

    void load(const string& file);

    json& getConfig();

    // 更新多级配置时，如json[key1][key2].此时参数key=key1_key2
    void update(const string& key1, const string& key2 = "", 
                                const string& key3 = "", const string& key4 = "");
    void addUpdateFunc(const string& key, const function<void(const json& config)>& func);

    Variant get(const string& key1, const string& key2 = "", 
                                const string& key3 = "", const string& key4 = "");

    Variant getAndListen(const function<void(const json& config)>& func, const string& key1, const string& key2 = "", 
                                const string& key3 = "", const string& key4 = "");

    void set(const Variant& value, const string& key1, const string& key2 = "", 
                const string& key3 = "", const string& key4 = "");

    
    void setAndUpdate(const Variant& value, const string& key1, const string& key2 = "", 
                const string& key3 = "", const string& key4 = "");

    void set(json& jValue, const Variant& value);
    Variant get(json& value);

private:
    json _config;
    mutex _updateMtx;
    unordered_map<string, vector<function<void(const json& config)>>> _mapUpdate;
};


#endif //Config_H
