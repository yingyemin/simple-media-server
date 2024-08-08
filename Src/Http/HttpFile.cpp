#include "HttpUtil.h"
#include "Logger.h"
#include "HttpFile.h"
#include "Util/Path.h"
#include "Util/String.h"
#include "Common/UrlParser.h"
#include "Common/Config.h"

#include <sys/stat.h>
#include <unordered_map>
#include <dirent.h>
#include <iomanip>

using namespace std;

HttpFile::HttpFile(const HttpParser& parser)
    :_parser(parser)
{

}

HttpFile::~HttpFile()
{

}

bool HttpFile::isValid()
{
    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Http", "Server", "Server1", "rootPath");
    }, "Http", "Server", "Server1", "rootPath");

    UrlParser urlParser;
    urlParser.parse(_parser._url);
    _filePath = File::absolutePath(urlParser.path_, rootPath);
    if (_filePath.empty()) {
        return false;
    }

    return true;
}

bool HttpFile::isDir()
{
    if (_filePath.empty()) {
        if (!isValid()) {
            return false;
        }
    }

    return File::isDir(_filePath.data());
}

bool HttpFile::isFile()
{
    if (_filePath.empty()) {
        if (!isValid()) {
            return false;
        }
    }

    return File::isFile(_filePath.data());
}

string HttpFile::getIndex()
{
    static int enableViewDir = Config::instance()->getAndListen([](const json &config){
        enableViewDir = Config::instance()->get("Http", "Server", "Server1", "enableViewDir");
    }, "Http", "Server", "Server1", "enableViewDir");

    if (!enableViewDir) {
        return "can't view dir";
    }

    string strPathPrefix(_filePath);
    string last_dir_name;
    if (strPathPrefix.back() == '/') {
        strPathPrefix.pop_back();
    } else {
        last_dir_name = split(strPathPrefix, "/").back();
    }

    if (!File::isDir(strPathPrefix.data())) {
        return "it is not a dir";
    }

    stringstream ss;
    ss <<   "<html>\r\n"
            "<head>\r\n"
            "<title>文件索引</title>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "<h1>文件索引:";

    ss << _parser._url;
    ss << "</h1>\r\n";
    if (_parser._url != "/") {
        ss << "<li><a href=\"";
        ss << "/";
        ss << "\">";
        ss << "根目录";
        ss << "</a></li>\r\n";

        ss << "<li><a href=\"";
        if(!last_dir_name.empty()){
            ss << "./";
        }else{
            ss << "../";
        }
        ss << "\">";
        ss << "上级目录";
        ss << "</a></li>\r\n";
    }

    DIR *pDir;
    dirent *pDirent;
    if ((pDir = opendir(strPathPrefix.data())) == NULL) {
        return "open dir failed";
    }
    set<string> setFile;
    while ((pDirent = readdir(pDir)) != NULL) {
        if (File::isSpecialDir(pDirent->d_name)) {
            continue;
        }
        if (pDirent->d_name[0] == '.') {
            continue;
        }
        setFile.emplace(pDirent->d_name);
    }
    int i = 0;
    for (auto &strFile :setFile) {
        string strAbsolutePath = strPathPrefix + "/" + strFile;
        bool isDir = File::isDir(strAbsolutePath.data());
        ss << "<li><span>" << i++ << "</span>\t";
        ss << "<a href=\"";
        if (!last_dir_name.empty()) {
            ss << last_dir_name << "/" << strFile;
        } else {
            ss << strFile;
        }

        if (isDir) {
            ss << "/";
        }
        ss << "\">";
        ss << strFile;
        if (isDir) {
            ss << "/</a></li>\r\n";
            continue;
        }
        //是文件
        struct stat fileData;
        if (0 == stat(strAbsolutePath.data(), &fileData)) {
            auto &fileSize = fileData.st_size;
            if (fileSize < 1024) {
                ss << " (" << fileData.st_size << "B)" << endl;
            } else if (fileSize < 1024 * 1024) {
                ss << fixed << setprecision(2) << " (" << fileData.st_size / 1024.0 << "KB)";
            } else if (fileSize < 1024 * 1024 * 1024) {
                ss << fixed << setprecision(2) << " (" << fileData.st_size / 1024 / 1024.0 << "MB)";
            } else {
                ss << fixed << setprecision(2) << " (" << fileData.st_size / 1024 / 1024 / 1024.0 << "GB)";
            }
        }
        ss << "</a></li>\r\n";
    }
    closedir(pDir);
    ss << "<ul>\r\n";
    ss << "</ul>\r\n</body></html>";
    return ss.str();
}

StreamBuffer::Ptr HttpFile::read(int size)
{
    if (!_file.open(_filePath)) {
        return nullptr;
    }

    return _file.read(size);
}

string HttpFile::getFilePath()
{
    return _filePath;
}

int HttpFile::getFileSize()
{
    if (!_file.open(_filePath)) {
        return 0;
    }

    return _file.getFileSize();
}