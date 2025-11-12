#ifndef GB28181_Utils_H
#define GB28181_Utils_H

#include <string>
#include <cstdint>

/// @brief 时间格式化
/// @param time 
/// @return 格式化后的字符串
std::string LocalTime(time_t time);

/// @brief 生成一个随机字符串
/// @param n 字符串长度
/// @return 生成的随机字符串
std::string GenerateRandomString(int n);

/// @brief 将ssrc转换为16进制字符串
/// @param ssrc 媒体流标识ID		
/// @return 16进制表示的SSRC字符串
std::string SSRC_Hex(const std::string& ssrc);

/// @brief 将字符串转换为utf8编码
/// @param input 
/// @return UTF8编码的字符串
std::string ToUtf8String(const std::string &input);

/// @brief 将字符串转换为mbcs编码
/// @param input 
/// @return mbcs编码的字符串
std::string ToMbcsString(const std::string &input);

/// @brief 获取当前模块的目录路径
/// @return 目录
std::string GetCurrentModuleDirectory();

/// @brief 将ISO8601格式的时间字符串转换为time_t格式
/// @param gz 
/// @return time_t格式的时间
int64_t ISO8601ToTimeT(const std::string& gz);

/// @brief 
/// @param timestamp 
/// @return 
std::string TimeToISO8601(const time_t timestamp);

#endif
