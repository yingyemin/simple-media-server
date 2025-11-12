#ifndef _STRING_H_
#define _STRING_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

//using namespace std;

std::string& toLower(std::string& str);
std::string& toUpper(std::string& str);
std::string trim(std::string& str, const std::string& chars);
std::string trimFront(std::string& str, const std::string& chars);
std::string trimBack(std::string& str, const std::string& chars);
std::vector<std::string> split(const std::string& s, const std::string& delim);
std::unordered_map<std::string, std::string> split(const std::string& str, const std::string& vecDelim, const std::string& mapDelim, const std::string& trimStr = " ", bool tolower = false);
std::string replace(std::string str, std::string old_str, std::string new_str);
bool endWith(const std::string& str, const std::string& suffix);
bool endWithIgnoreCase(const std::string& str, const std::string& suffix);
bool startWith(const std::string& str, const std::string& suffix);
bool startWithIgnoreCase(const std::string& str, const std::string& suffix);
std::string randomStr(int sz, bool printable = true);
std::string randomString(int sz);
std::string findSubStr(const std::string& buf, const std::string& start, const std::string& end, int bufSize = 0);

uint64_t readUint48BE(const char* buf);
uint64_t readUint48LE(const char* buf);
uint32_t readUint32BE(const char* buf);
uint32_t readUint32LE(const char* buf);
uint32_t readUint24BE(const char* buf);
uint32_t readUint24LE(const char* buf);
uint16_t readUint16BE(const char* buf);
uint16_t readUint16LE(const char* buf);

void writeUint32BE(char* p, uint32_t value);
void writeUint32LE(char* p, uint32_t value);
void writeUint24BE(char* p, uint32_t value);
void writeUint24LE(char* p, uint32_t value);
void writeUint16BE(char* p, uint16_t value);
void writeUint16LE(char* p, uint16_t value);

#endif