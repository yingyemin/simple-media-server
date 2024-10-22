#include "String.h"
#include "Log/Logger.h"

#include <cstring>
#include <string>
#include <algorithm>
#include <regex>
#include <random>

static constexpr char CCH[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

std::string &toLower(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return str;
}

// string转大写
std::string &toUpper(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return str;
}

string trimWithChar(string &str, const string& chars){
    regex pattern("^[" + chars + "]");
    str = regex_replace(str, pattern, "");

    regex patternback("[" + chars + "]$");
    return regex_replace(str, patternback, "");
}

string trim(string &str, const string& chars)
{
    if (str.empty()) {
        return "";
    }

    str = trimFront(str, chars);
    str = trimBack(str, chars);
    return str;
}

string trimFront(string &str, const string& chars)
{
    if (str.empty()) {
        return "";
    }
    // regex pattern("^[" + chars + "]");
    // return regex_replace(str, pattern, "");

    while (str.find(chars) == 0) {
        str = str.substr(chars.length());
    }

    return str;
}
string trimBack(string &str, const string& chars)
{
    if (str.empty()) {
        return "";
    }
    // regex pattern("[" + chars + "]$");
    // return regex_replace(str, pattern, "");

    while (str.find(chars) == (str.length() - chars.length())) {
        str = str.substr(0, str.length() - chars.length());
    }

    return str;
}

vector<string> split(const string &s, const string& delim)
{
    vector<string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != string::npos) {
        if (index - last > 0) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + delim.size();
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0) {
        ret.push_back(s.substr(last));
    }
    return ret;
}

unordered_map<string, string> split(const string &str, const string &vecDelim, const string &mapDelim, 
                    const string &trimStr, bool tolower)
{
    unordered_map<string, string> ret;
    auto arg_vec = split(str, vecDelim);
    for (auto &key_val : arg_vec) {
        if (key_val.empty()) {
            // 忽略
            continue;
        }
        auto pos = key_val.find(mapDelim);
        if (pos != string::npos) {
            auto key = key_val.substr(0, pos);
            key = trimWithChar(key, trimStr);//trim(string(key_val, 0, pos));
            
            if (tolower) {
                key = toLower(key);
            }
            auto val = key_val.substr(pos + mapDelim.size());
            val = trimWithChar(val, trimStr);
            ret[key] = val;
        }
    }
    return ret;
}

static bool compareIgnoreCase(unsigned char a, unsigned char b) {
	return std::tolower(a) == std::tolower(b);
}

bool endWith(const string& str, const string& suffix)
{
    if (str.size() < suffix.size()) {
		return false;
	}
 
	std::string tstr = str.substr(str.size() - suffix.size());
 
	if (tstr == suffix) {
		return true;
	} else {
		return false;
	}
}

bool endWithIgnoreCase(const string& str, const string& suffix)
{
    if (str.size() < suffix.size()) {
		return false;
	}
 
	std::string tstr = str.substr(str.size() - suffix.size());
 
	if (tstr.length() == suffix.length()) {
		return std::equal(suffix.begin(), suffix.end(), tstr.begin(), compareIgnoreCase);
	} else {
		return false;
	}
}

string randomStr(int sz, bool printable) 
{
    string ret;
    ret.resize(sz);
    std::mt19937 rand(std::random_device{}());
    for (int i = 0; i < sz; ++i) {
        if (printable) {
            uint32_t x = rand() % (sizeof(CCH) - 1);
            ret[i] = CCH[x];
        } else {
            ret[i] = rand() % 0xFF;
        }
    }
    return ret;
}

string randomString(int length) 
{
    static string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string result;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, charset.size() - 1);
 
    for (int i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
 
    return result;
}

string findSubStr(const string& buf, const string& start, const string& end, int bufSize)
{
    if(bufSize <=0 ){
        bufSize = buf.size();
    }
    int msg_start = 0, msg_end = bufSize;
    int len = 0;
    if (start != "") {
        len = start.size();
        msg_start = buf.find(start);
    }
    if (msg_start == string::npos) {
        return "";
    }
    logInfo << "msg_start: " << msg_start;
    logInfo << "len: " << len;
    auto bufstart = buf.substr(msg_start + len);
    if (end != "") {
        msg_end = bufstart.find(end);
        if (msg_end == string::npos) {
            msg_end = bufSize;
        }
    }
    return bufstart.substr(0, msg_end);
}

uint32_t readUint48BE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint64_t value = ((uint64_t)p[0] << 40) | ((uint64_t)p[1] << 32) | (p[2] << 24) | (p[3] << 16) | (p[4] << 8) | p[5];
	return value;

}

uint32_t readUint48LE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint64_t value = ((uint64_t)p[5] << 40) | ((uint64_t)p[4] << 32) | (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	return value;

}

uint32_t readUint32BE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	return value;

}

uint32_t readUint32LE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	return value;

}

uint32_t readUint24BE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
	return value;
}

uint32_t readUint24LE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint16_t readUint16BE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint16_t value = (p[0] << 8) | p[1];
	return value; 
}

uint16_t readUint16LE(const char* buf)
{
    uint8_t* p = (uint8_t*)buf;
	uint16_t value = (p[1] << 8) | p[0];
	return value; 

}

void writeUint32BE(char* p, uint32_t value)
{
	p[0] = value >> 24;
	p[1] = value >> 16;
	p[2] = value >> 8;
	p[3] = value & 0xff;
}

void writeUint32LE(char* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
	p[3] = value >> 24;
}

void writeUint24BE(char* p, uint32_t value)
{
	p[0] = value >> 16;
	p[1] = value >> 8;
	p[2] = value & 0xff;
}

void writeUint24LE(char* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
}

void writeUint16BE(char* p, uint16_t value)
{
	p[0] = value >> 8;
	p[1] = value & 0xff;
}

void writeUint16LE(char* p, uint16_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
}

bool startWith(const string& str, const string& suffix)
{
    if (str.size() < suffix.size()) {
		return false;
	}
 
	std::string tstr = str.substr(0 , suffix.size());
 
	if (tstr == suffix) {
		return true;
	} else {
		return false;
	}
}

bool startWithIgnoreCase(const string& str, const string& suffix)
{
    if (str.size() < suffix.size()) {
		return false;
	}
 
	std::string tstr = str.substr(0, suffix.size());
 
	if (tstr.length() == suffix.length()) {
		return std::equal(suffix.begin(), suffix.end(), tstr.begin(), compareIgnoreCase);
	} else {
		return false;
	}
}