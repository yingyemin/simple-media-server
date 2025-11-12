#if defined(_WIN32)
#include "Util.h"
#include <io.h>   
#include <direct.h>
#include <windows.h>
#include <shlobj.h> // For IShellLink
#include <comdef.h> // For COM error handling
#else
#include <dirent.h>
#include <unistd.h>
#endif // WIN32

#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include "File.h"
#include "Log/Logger.h"
#include "String.hpp"
#include "Path.h"


#if !defined(_WIN32)
#define    _unlink    unlink
#define    _rmdir    rmdir
#define    _access    access
#endif

#if defined(_WIN32)

int mkdir(const char* path, int mode) {
    return _mkdir(path);
}

DIR* opendir(const char* name) {
    char namebuf[512];
    snprintf(namebuf, sizeof(namebuf), "%s\\*.*", name);

    WIN32_FIND_DATAA FindData;
    auto hFind = FindFirstFileA(namebuf, &FindData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    DIR* dir = (DIR*)malloc(sizeof(DIR));
    memset(dir, 0, sizeof(DIR));
    dir->dd_fd = 0;   // simulate return  
    dir->handle = hFind;
    return dir;
}

struct dirent* readdir(DIR* d) {
    HANDLE hFind = d->handle;
    WIN32_FIND_DATAA FileData;
    //fail or end  
    if (!FindNextFileA(hFind, &FileData)) {
        return nullptr;
    }
    struct dirent* dir = (struct dirent*)malloc(sizeof(struct dirent) + sizeof(FileData.cFileName));
    strcpy(dir->d_name, (char*)FileData.cFileName);
    dir->d_reclen = (uint16_t)strlen(dir->d_name);
    //check there is file or directory  
    if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        dir->d_type = 2;
    }
    else {
        dir->d_type = 1;
    }
    if (d->index) {
        //覆盖前释放内存  [AUTO-TRANSLATED:1cb478a1]
        //Release memory before covering
        free(d->index);
        d->index = nullptr;
    }
    d->index = dir;
    return dir;
}

int closedir(DIR* d) {
    if (!d) {
        return -1;
    }
    //关闭句柄  [AUTO-TRANSLATED:ec4f562d]
    //Close handle
    if (d->handle != INVALID_HANDLE_VALUE) {
        FindClose(d->handle);
        d->handle = INVALID_HANDLE_VALUE;
    }
    //释放内存  [AUTO-TRANSLATED:0f4046dc]
    //Release memory
    if (d->index) {
        free(d->index);
        d->index = nullptr;
    }
    free(d);
    return 0;
}
// 定义 Windows 文件类型常量
#define S_ISREG(fileAttributes) (fileAttributes & FILE_ATTRIBUTE_NORMAL || fileAttributes & FILE_ATTRIBUTE_ARCHIVE || \
                                 fileAttributes & FILE_ATTRIBUTE_HIDDEN || fileAttributes & FILE_ATTRIBUTE_SYSTEM || \
                                 fileAttributes & FILE_ATTRIBUTE_TEMPORARY || fileAttributes & FILE_ATTRIBUTE_COMPRESSED)
#define S_ISLNK(fileAttributes) (fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)

#endif // defined(_WIN32)

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
            if (mkdir(dir.c_str(), 0777) == -1) {  //如果不存在就用mkdir函数来创建
                return false;
            }
        }
    }
    return true;
}

//判断是否为目录
bool File::isDir(const char *path) {
#if defined(_WIN32)
    auto dir = opendir(path);
    if (!dir) {
        return false;
    }
    closedir(dir);
    return true;
#else
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
#endif
}
#if defined(_WIN32)
// 将 const char * 转换为 LPCOLESTR（宽字符）
std::wstring MultiByteToWideString(const char* path)
{
    if (path == nullptr)
        return std::wstring();

    int len = MultiByteToWideChar(CP_ACP, 0, path, -1, nullptr, 0);
    if (len == 0)
        return std::wstring(); // 转换失败

    std::wstring widePath(len, 0);
    MultiByteToWideChar(CP_ACP, 0, path, -1, &widePath[0], len);

    return widePath;
}

bool File::isFile(const char* path)
{
    // 使用 GetFileAttributes 获取文件属性
    DWORD attributes = GetFileAttributesA(path);

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        // 路径不存在或无法访问
        return false;
    }

    // 判断文件类型是否为普通文件
    if (S_ISREG(attributes))
    {
        return true;
    }

    // 如果是链接（快捷方式），尝试解析到目标路径
    if (S_ISLNK(attributes))
    {
        char realFile[MAX_PATH] = { 0 };
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        if (SUCCEEDED(hr))
        {
            IShellLink* pShellLink = nullptr;
            hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

            if (SUCCEEDED(hr))
            {
                IPersistFile* pPersistFile = nullptr;
                hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

                if (SUCCEEDED(hr))
                {
                    std::wstring widePath = MultiByteToWideString(path);
                    LPCOLESTR olePath = widePath.c_str();
                    // 设置代码页为 CP_UTF8 以支持 UTF-8 路径
                    hr = pPersistFile->Load(olePath, STGM_READ);

                    if (SUCCEEDED(hr))
                    {
                        WIN32_FIND_DATA findData = { 0 };
                        hr = pShellLink->Resolve(nullptr, 0); // Resolve the link

                        if (SUCCEEDED(hr))
                        {
                            // 获取目标路径
                            hr = pShellLink->GetPath(realFile, MAX_PATH, &findData, 0);

                            if (SUCCEEDED(hr))
                            {
                                // 数量化目标路径
                                char resolvedPath[MAX_PATH] = { 0 };
                                GetLongPathNameA(realFile, resolvedPath, MAX_PATH);

                                // 递归调用 isFile 函数判断目标路径是否是文件
                                return File::isFile(resolvedPath);
                            }
                        }
                    }

                    pPersistFile->Release();
                }

                pShellLink->Release();
            }

            CoUninitialize();
        }

        // 如果解析失败或快捷方式无效，返回 false
        return false;
    }

    // 不是普通文件或快捷方式
    return false;
}
#else
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
#endif
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
    close();
}

bool File::open(const string& filePath, const string& mode)
{
    if (_fp) {
        return true;
    }

    createDir(filePath.c_str(), 0755);

    _filePath = filePath;
    _fp = fopen(_filePath.data(), mode.c_str());

    if (!_fp) {
        logWarn << "open file failed: " << _filePath;
        return false;
    }

    return true;
}

void File::close()
{
    if (_fp) {
        fclose(_fp);
        _fp = nullptr;
    }
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
    logDebug << "cur pos: " << tell();
    logDebug << "read size: " << readBytes;
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
    // logTrace << "read size: " << readBytes;
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
bool File::isEnd()
{
    return feof(_fp);
}


void File::rewind()
{
    fseek(_fp, 0, SEEK_SET);
}

bool File::seek_end()
{
    if (fseek(_fp, 0, SEEK_END) != 0) {
        return false;
    }
    return true;
}
bool File::seek_from_end(int64_t size)
{
    if (fseek(_fp, size, SEEK_END) != 0)
    {
        return false;
    }
    return true;
}
bool File::seek_from_cur(int64_t size)
{
    if (fseek(_fp, size, SEEK_CUR) != 0)
    {
        return false;
    }
    return true;
}