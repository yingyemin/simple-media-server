#ifndef Config_H
#define Config_H

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>

#include "json.hpp"
#include "Util/Variant.h"

using json = nlohmann::json;
// using namespace std;

class Config : public std::enable_shared_from_this<Config>
{
public:
    using Ptr = std::shared_ptr<Config>;

    Config() {}
    ~Config() {}

public:
    static Config::Ptr instance();

    void load(const std::string& file);

    json& getConfig();

    // 更新多级配置时，如json[key1][key2].此时参数key=key1_key2
    void update(const std::string& key1, const std::string& key2 = "", 
                                const std::string& key3 = "", const std::string& key4 = "");
    void addUpdateFunc(const std::string& key, const std::function<void(const json& config)>& func);

    Variant get(const std::string& key1, const std::string& key2 = "", const std::string& key3 = "", 
                const std::string& key4 = "", const std::string& value = "");

    Variant getAndListen(const std::function<void(const json& config)>& func, const std::string& key1, const std::string& key2 = "", 
                        const std::string& key3 = "", const std::string& key4 = "", const std::string& value = "");

    void set(const Variant& value, const std::string& key1, const std::string& key2 = "", 
                const std::string& key3 = "", const std::string& key4 = "");

    
    void setAndUpdate(const Variant& value, const std::string& key1, const std::string& key2 = "", 
                const std::string& key3 = "", const std::string& key4 = "");

    void set(json& jValue, const Variant& value);
    Variant get(json& value);

private:
    json _config;
    std::mutex _updateMtx;
    std::unordered_map<std::string, std::vector<std::function<void(const json& config)>>> _mapUpdate;
};


#endif //Config_H