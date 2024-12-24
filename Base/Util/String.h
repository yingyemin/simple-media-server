#ifndef String_h
#define String_h

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

using namespace std;

string& toLower(string& str);
string& toUpper(string &str);
string trim(string &str, const string& chars);
string trimFront(string &str, const string& chars);
string trimBack(string &str, const string& chars);
vector<string> split(const string &s, const string& delim);
unordered_map<string, string> split(const string &str, const string &vecDelim, const string &mapDelim, const string &trimStr = " ", bool tolower = false);
string replace(string str, string old_str, string new_str);
bool endWith(const string& str, const string& suffix);
bool endWithIgnoreCase(const string& str, const string& suffix);
bool startWith(const string& str, const string& suffix);
bool startWithIgnoreCase(const string& str, const string& suffix);
string randomStr(int sz, bool printable = true);
string randomString(int sz);
string findSubStr(const string& buf, const string& start, const string& end, int bufSize = 0);

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