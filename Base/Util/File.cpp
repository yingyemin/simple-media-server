#include <dirent.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include "File.h"
#include "Log/Logger.h"
#include "String.h"
#include "Path.h"

using namespace std;

FILE *File::createFile(const char *file, const char *mode)
{
    std::string path = file;
    std::string dir;
    int index = 1;
    FILE *ret = NULL;
    while (true) {
        index = path.find('/', index) + 1;
        dir = path.substr(0, index);
        if (dir.length() == 0) {
            break;
        }
        if (access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
            if (mkdir(dir.c_str(), 0777) == -1) {  //如果不存在就用mkdir函数来创建
                return NULL;
            }
        }
    }
    if (path[path.size() - 1] != '/') {
        ret = fopen(file, mode);
    }
    return ret;
}

bool File::createDir(const char *file, unsigned int mod) {
    std::string path = file;
    std::string dir;
    int index = 1;
    while (1) {
        index = path.find('/', index) + 1;
        dir = path.substr(0, index);
        if (dir.length() == 0) {
            break;
        }
        if (access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
            if (mkdir(dir.c_str(), mod) == -1) {  //如果不存在就用mkdir函数来创建
                return false;
            }
        }
    }
    return true;
}

//判断是否为目录
bool File::isDir(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) == 0) {
        //lstat返回文件的信息，文件信息存放在stat结构中
        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            return true;
        }
        
        if (S_ISLNK(statbuf.st_mode)) {
            char realFile[256] = { 0 };
            if (-1 == readlink(path, realFile, sizeof(realFile))) {
                return false;
            }
            return File::isDir(realFile);
        }
    }
    return false;
}

//判断是否为常规文件
bool File::isFile(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) == 0) {
        if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
            return true;
        }
        
        if (S_ISLNK(statbuf.st_mode)) {
            char realFile[256] = { 0 };
            if (-1 == readlink(path, realFile, sizeof(realFile))) {
                return false;
            }
            return File::isFile(realFile);
        }
    }
    return false;
}

//判断是否是特殊目录
bool File::isSpecialDir(const char *path) {
    return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

//生成完整的文件路径
void getFilePath(const char *path, const char *file_name, char *file_path) {
    strcpy(file_path, path);
    if (file_path[strlen(file_path) - 1] != '/') {
        strcat(file_path, "/");
    }
    strcat(file_path, file_name);
}

void File::deleteFile(const char *path)
{
    DIR *dir;
    dirent *dir_info;
    char file_path[PATH_MAX];
    if (isFile(path)) {
        remove(path);
        return;
    }
    if (isDir(path)) {
        if ((dir = opendir(path)) == NULL) {
            rmdir(path);
            return;
        }
        while ((dir_info = readdir(dir)) != NULL) {
            if (isSpecialDir(dir_info->d_name)) {
                continue;
            }
            getFilePath(path, dir_info->d_name, file_path);
            deleteFile(file_path);
        }
        rmdir(path);
        closedir(dir);
        return;
    }
    unlink(path);
}

string File::loadFile(const char *path) {
    FILE *fp = fopen(path,"rb");
    if(!fp){
        return "";
    }
    fseek(fp,0,SEEK_END);
    auto len = ftell(fp);
    fseek(fp,0,SEEK_SET);
    string str(len,'\0');
    if (-1 == fread((char *) str.data(), str.size(), 1, fp)) {
        
    }
    fclose(fp);
    return str;
}

bool File::saveFile(const string &data, const char *path) {
    FILE *fp = fopen(path,"wb");
    if(!fp){
        return false;
    }
    fwrite(data.data(),data.size(),1,fp);
    fclose(fp);
    return true;
}

string File::parentDir(const string &path) {
    auto parent_dir = path;
    if(parent_dir.back() == '/'){
        parent_dir.pop_back();
    }
    auto pos = parent_dir.rfind('/');
    if(pos != string::npos){
        parent_dir = parent_dir.substr(0,pos + 1);
    }
    return parent_dir;
}

string File::absolutePath(const string &path,const string &currentPath_in,bool canAccessParent) {
    string currentPath = currentPath_in;
    if(!currentPath.empty()){
        //当前目录不为空
        if(currentPath.front() == '.'){
            //如果当前目录是相对路径，那么先转换成绝对路径
            currentPath = absolutePath(currentPath_in, Path::exeDir(), true);
        }
    } else{
        currentPath = Path::exeDir();
    }

    if(path.empty()){
        //相对路径为空，那么返回当前目录
        return currentPath;
    }

    if(currentPath.back() != '/'){
        //确保当前目录最后字节为'/'
        currentPath.push_back('/');
    }

    auto rootPath = currentPath;
    auto dir_vec = split(path,"/");
    for(auto &dir : dir_vec){
        if(dir.empty() || dir == "."){
            //忽略空或本文件夹
            continue;
        }
        if(dir == ".."){
            //访问上级目录
            if(!canAccessParent && currentPath.size() <= rootPath.size()){
                //不能访问根目录之外的目录
                logInfo << "out of root path" << endl;
                return "";
            }
            currentPath = parentDir(currentPath);
            continue;
        }
        currentPath.append(dir);
        currentPath.append("/");
    }

    if(path.back() != '/' && currentPath.back() == '/'){
        //在路径是文件的情况下，防止转换成目录
        currentPath.pop_back();
    }
    string curDir = Path::exeDir();
    if (currentPath.size() < curDir.size() || curDir != currentPath.substr(0, curDir.size())) {
        logError << "currentPath is invalid: " << currentPath << endl;
        return "";
    }

    return currentPath;
}

void File::scanDir(const string &path_in, const function<bool(const string &path, bool isDir)> &cb, bool enterSubdirectory) {
    string path = path_in;
    if(path.back() == '/'){
        path.pop_back();
    }

    DIR *pDir;
    dirent *pDirent;
    if ((pDir = opendir(path.data())) == NULL) {
        //文件夹无效
        return;
    }
    while ((pDirent = readdir(pDir)) != NULL) {
        if (isSpecialDir(pDirent->d_name)) {
            continue;
        }
        if(pDirent->d_name[0] == '.'){
            //隐藏的文件
            continue;
        }
        string strAbsolutePath = path + "/" + pDirent->d_name;
        bool isDir = File::isDir(strAbsolutePath.data());
        if(!cb(strAbsolutePath, isDir)){
            //不再继续扫描
            break;
        }

        if(isDir && enterSubdirectory){
            //如果是文件夹并且扫描子文件夹，那么递归扫描
            scanDir(strAbsolutePath,cb,enterSubdirectory);
        }
    }
    closedir(pDir);
}

File::File(const string& filePath)
    :_filePath(filePath)
{

}

File::File()
{

}

File::~File()
{
    if (_fp) {
        fclose(_fp);
        _fp = nullptr;
    }
}

bool File::open(const string& filePath, const string& mode)
{
    if (_fp) {
        return true;
    }

    _filePath = filePath;
    _fp = fopen(_filePath.data(), mode.c_str());

    if (!_fp) {
        logWarn << "open file failed: " << _filePath;
        return false;
    }

    return true;
}

StreamBuffer::Ptr File::read(int size)
{
    if (_filePath.empty()) {
        logWarn << "file path is empty";
        return nullptr;
    }

    if (!_fp) {
        _fp = fopen(_filePath.data(), "ab+");
        if (!_fp) {
            logWarn << "open file failed: " << _filePath;
            return nullptr;
        }
    }

    auto buffer = StreamBuffer::create();
    buffer->setCapacity(size + 1);

    int readBytes = fread(buffer->data(), 1, size, _fp);
    logTrace << "read size: " << readBytes;
    if (readBytes == 0) {
        return nullptr;
    }

    buffer->substr(0, readBytes);

    return buffer;
}

int File::read(char* data, int size)
{
    if (_filePath.empty()) {
        logWarn << "file path is empty";
        return 0;
    }

    if (!_fp) {
        _fp = fopen(_filePath.data(), "ab+");
        if (!_fp) {
            logWarn << "open file failed: " << _filePath;
            return 0;
        }
    }

    int readBytes = fread(data, 1, size, _fp);
    logInfo << "read size: " << readBytes;
    if (readBytes == 0) {
        return readBytes;
    }

    return readBytes;
}

bool File::write(const Buffer::Ptr& buffer)
{
    return write(buffer->data(), buffer->size());
}

bool File::write(const char* data, int size)
{
    // TODO 写失败后缓存一下，稍后重试
    if (_filePath.empty()) {
        logWarn << "file path is empty";
        return false;
    }

    if (!_fp) {
        _fp = fopen(_filePath.data(), "ab+");
        if (!_fp) {
            logWarn << "open file failed: " << _filePath;
            return false;
        }
    }

    int writeBytes = fwrite(data, 1, size, _fp);
    if (!writeBytes) {
        return false;
    }

    return true;
}

void File::seek(uint64_t size)
{
    fseek(_fp, size, SEEK_SET);
}

uint64_t File::tell()
{
    return ftell(_fp);
}

int File::getFileSize()
{
    if (_filePath.empty()) {
        logWarn << "file path is empty";
        return 0;
    }

    if (!_fp) {
        _fp = fopen(_filePath.data(), "ab+");
        if (!_fp) {
            logWarn << "open file failed: " << _filePath;
            return 0;
        }
    }

    struct stat statbuf;
    if (stat(_filePath.data(), &statbuf) == 0) {
        return statbuf.st_size;
    }

    return 0;
}
