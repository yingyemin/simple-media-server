#include <fstream>

#include "Config.h"
#include "Log/Logger.h"
#include "Util/String.h"

using namespace std;

Config::Ptr Config::instance()
{ 
    static Config::Ptr config(new Config()); 
    return config; 
}

void Config::load(const string& filename)
{
    ifstream configFile(filename);
    if (!configFile.is_open()) {
        throw runtime_error("Config file " + filename + " not found");
    }

    string line;
    string configStr;
    while (getline(configFile, line)) {
        line = trim(line, " ");
        if (!line.empty()) {
            if (line[0] == '#') {
                continue ;
            } else {
                configStr += line;
            }
        }
    }
    
    configFile.close();
    _config = json::parse(configStr);
}

json& Config::getConfig()
{
    return _config;
}

void Config::addUpdateFunc(const string& key, const function<void(const json& config)>& func)
{
    lock_guard<mutex> lock(_updateMtx);
    _mapUpdate[key].emplace_back(func);
}

void Config::update(const string& key1, const string& key2, 
                                const string& key3, const string& key4)
{
    string key = key1;
    if (!key2.empty()) {
        key += "." + key2;
    }
    if (!key3.empty()) {
        key += "." + key3;
    }
    if (!key4.empty()) {
        key += "." + key4;
    }
    if (_mapUpdate.find(key) == _mapUpdate.end()) {
        return ;
    }

    lock_guard<mutex> lock(_updateMtx);
    auto &funcs = _mapUpdate[key];
    for (auto& func : funcs) {
        func(_config);
    }
}

Variant Config::get(json& value)
{
    if (value.is_string()) {
        string str = value;
        return str;
    } else {
        logInfo << "get a value: " << value;
        float i = value;
        return i;
    }
}

Variant Config::get(const string& key1, const string& key2, const string& key3, const string& key4)
{
    if (!key4.empty()) {
        if (!_config.contains(key1) || !_config[key1].contains(key2) || 
            !_config[key1][key2].contains(key3) || !_config[key1][key2][key3].contains(key4))
        {
            return "";
        }
        return get(_config[key1][key2][key3][key4]);
    } else if (!key3.empty()) {
        if (!_config.contains(key1) || !_config[key1].contains(key2) || 
            !_config[key1][key2].contains(key3))
        {
            return "";
        }
        return get(_config[key1][key2][key3]);
    } else if (!key2.empty()) {
        if (!_config.contains(key1) || !_config[key1].contains(key2))
        {
            return "";
        }
        return get(_config[key1][key2]);
    } else {
        if (!_config.contains(key1))
        {
            return "";
        }
        return get(_config[key1]);
    }
}

Variant Config::getAndListen(const function<void(const json& config)>& func, const string& key1, 
                        const string& key2, const string& key3, const string& key4)
{
    string key = key1;
    if (!key2.empty()) {
        key += "." + key2;
    }
    if (!key3.empty()) {
        key += "." + key3;
    }
    if (!key4.empty()) {
        key += "." + key4;
    }
    addUpdateFunc(key, func);
    // logInfo << "key: " << key;

    return get(key1, key2, key3, key4);
}

void Config::set(json& jValue, const Variant& value)
{
    if (jValue.is_string()) {
        jValue = value.as<string>();
    } else if (jValue.is_number_float()) {
        jValue = value.as<float>();
    } else {
        jValue = value.as<int>();
    }
}

void Config::set(const Variant& value, const string& key1, const string& key2, 
                const string& key3, const string& key4)
{
    int value1 = value.as<int>();
    if (!key4.empty()) {
        auto &jValue = _config[key1][key2][key3][key4];
        set(jValue, value);
    } else if (!key3.empty()) {
        auto &jValue = _config[key1][key2][key3];
        set(jValue, value);
    } else if (!key2.empty()) {
        auto &jValue = _config[key1][key2];
        set(jValue, value);
    } else {
        auto &jValue = _config[key1];
        set(jValue, value);
    }
}

void Config::setAndUpdate(const Variant& value, const string& key1, const string& key2, 
                const string& key3, const string& key4)
{
    set(value, key1, key2, key3, key4);
    update(key1, key2, key3, key4);
}
