#include "Path.h"

#include <unistd.h>
#include <limits.h>

using namespace std;

string Path::exeDir()
{
    auto path = exePath();
    return path.substr(0, path.rfind('/') + 1);
}

string Path::exePath()
{
    char buffer[PATH_MAX * 2 + 1] = { 0 };
    int n = -1;
    
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));

    string filePath = "./";
    if (n > 0) {
        filePath = buffer;
    }

    return filePath;
}

string Path::exeName()
{
    auto path = exePath();
    return path.substr(path.rfind('/') + 1);
}

string Path::getFileName(const std::string& path)
{
    // 找到最后一个斜杠的位置
    size_t last_slash_idx = path.find_last_of("/\\");
    if (last_slash_idx == std::string::npos) {
        last_slash_idx = 0; // 如果没有找到斜杠，则从头开始查找点号
    } else {
        last_slash_idx++; // 移动到斜杠之后的位置
    }
    // 找到最后一个点号的位置
    size_t last_dot_idx = path.find_last_of(".");
    if (last_dot_idx == std::string::npos || last_dot_idx < last_slash_idx) {
        // 如果没有点号或者点号出现在斜杠之后，则整个字符串即为文件名（无扩展名）
        return path.substr(last_slash_idx);
    } else {
        // 否则，返回最后一个点号之前的字符串作为文件名（无扩展名）
        return path.substr(last_slash_idx, last_dot_idx - last_slash_idx);
    }
}