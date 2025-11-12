#include "Utils.h"
#include "Util/String.hpp"
#include "Util/TimeClock.h"
#include "Log/Logger.h"
#include "Util/Path.h"
#include <random>
#include <regex>

#if defined _WIN32

std::string Utf8ToMultiByte(const std::string& utf8Str) {
	// 首先将 UTF-8 转换为宽字符字符串
	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	if (wideCharLength == 0) {
		return "";
	}
	std::wstring wideStr(wideCharLength, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideCharLength);

	// 然后将宽字符字符串转换为多字节字符串
	int multiByteLength = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (multiByteLength == 0) {
		return "";
	}
	std::string multiByteStr(multiByteLength, 0);
	WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &multiByteStr[0], multiByteLength, nullptr, nullptr);

	return multiByteStr;
}

std::string MultiByteToUtf8(const std::string& multiByteStr) {
	// 先将多字节字符串转换为宽字符字符串
	int wideCharLength = MultiByteToWideChar(CP_ACP, 0, multiByteStr.c_str(), -1, nullptr, 0);
	if (wideCharLength == 0) {
		return "";
	}
	std::wstring wideStr(wideCharLength, 0);
	MultiByteToWideChar(CP_ACP, 0, multiByteStr.c_str(), -1, &wideStr[0], wideCharLength);

	// 再将宽字符字符串转换为 UTF-8 字符串
	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (utf8Length == 0) {
		return "";
	}
	std::string utf8Str(utf8Length, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Length, nullptr, nullptr);

	return utf8Str;
}

#else
#include <iconv.h>
const size_t BUFFER_SIZE = 1024;
// 使用iconv进行编码转换
bool convert_encoding(const char* from_charset, const char* to_charset, std::string& input, std::string& output) {
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t)-1) {
        logError << "iconv_open failed: " << strerror(errno) << std::endl;
        return false;
    }

    char* in_buf = const_cast<char*>(input.c_str());
    size_t in_bytes_left = input.size();
    size_t out_bytes_left = BUFFER_SIZE;
    char out_buf[BUFFER_SIZE];

    char* pin = in_buf;
    char* pout = out_buf;

    while (in_bytes_left > 0) {
        size_t result = iconv(cd, &pin, &in_bytes_left, &pout, &out_bytes_left);
        if (result == (size_t)-1) {
            if (errno == EILSEQ) {
                // 忽略无法转换的字符
                ++pin;
                --in_bytes_left;
                continue;
            } else {
                logError << "iconv failed: " << strerror(errno) << std::endl;
                iconv_close(cd);
                return false;
            }
        }
        output.append(out_buf, BUFFER_SIZE - out_bytes_left);
        out_bytes_left = BUFFER_SIZE;
        pout = out_buf;
    }

    iconv_close(cd);
    return true;
}

#endif

std::string LocalTime(time_t time)
{
	// return fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(time));
	return TimeClock::getFmtTime("%Y-%m-%d %H:%M:%S", time);
}

std::string GenerateRandomString(int n)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 1000);

	const std::string chars("0123456789"
							"abcdefghijklmnopqrstuvwxyz"
							"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	int i = 0;
	int len = (int)chars.size();
	std::string text;
	while (i < n)
	{
		int idx = dis(gen) % len;
		text.push_back(chars[idx]);
		++i;
	}

	return text;
}

std::string SSRC_Hex(const std::string& ssrc)
{
	// return fmt::format("{:08X}", std::stol(ssrc));
	return ssrc;
}

std::string ToUtf8String(const std::string& input)
{
#ifdef _WIN32
	return MultiByteToUtf8(input);
#else
	return input;
#endif
}

std::string ToMbcsString(const std::string& input)
{
#ifdef _WIN32
	return Utf8ToMultiByte(input);
#else
	std::string gb2312_content;
	std::string utf8_content = input;
	auto ret = convert_encoding("utf8","gbk",utf8_content,gb2312_content);
	if(ret)
		return gb2312_content;
	return input;
#endif
}

std::string GetCurrentModuleDirectory()
{
	return Path::exeDir();
}


//格式化后的时间字符串转化为std::time_t格式，所有的输出的时间戳都需要转化为GMT时间
int64_t ISO8601ToTimeT(const std::string& str)
{
	std::regex pattern(R"(^\d{4}-\d{2}-\d{2}[Tt]\d{2}:\d{2}:\d{2}[Zz]?$)");
	if (std::regex_match(str, pattern)) {
		std::tm t;
		auto year = str.substr(0, 4);
		t.tm_year = std::stoi(year) - 1900;
		auto month = str.substr(5, 2);
		t.tm_mon = std::stoi(month) - 1;
		auto day = str.substr(8, 2);
		t.tm_mday = std::stoi(day);
		auto hour = str.substr(11, 2);
		t.tm_hour = std::stoi(hour);
		auto minute = str.substr(14, 2);
		t.tm_min = std::stoi(minute);
		auto second = str.substr(17, 2);
		t.tm_sec = std::stoi(second);

		// GMT，不以z结尾为本地时间,本地时间转换为标准时间
		if (str.back() != 'z' && str.back() != 'Z')
		{
			auto tt = std::mktime(&t);
			auto tm = std::localtime(&tt);
			return std::mktime(tm);
		}
		else// 即是标准时间
		{
			return std::mktime(&t);
		}
	}
	else {
		logError << "时间格式校验错误: " << str;
		return 0;
	}
}


std::string TimeToISO8601(const time_t timestamp) {
	// return fmt::format("{:%Y-%m-%dT%H:%M:%S}", fmt::localtime(timestamp));
	return TimeClock::getFmtTime("%Y-%m-%d %H:%M:%S", timestamp);
}
