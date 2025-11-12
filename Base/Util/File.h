#ifndef SRC_UTIL_FILE_H_
#define SRC_UTIL_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <functional>
#include <limits.h>


#include "Net/Buffer.h"
#if defined(_WIN32)
#include "Util/Util.h"
#ifndef PATH_MAX
#define PATH_MAX 1024

#endif // !PATH_MAX

struct dirent {
    long d_ino;              /* inode number*/
    off_t d_off;             /* offset to this dirent*/
    unsigned short d_reclen; /* length of this d_name*/
    unsigned char d_type;    /* the type of d_name*/
    char d_name[1];          /* file name (null-terminated)*/
};
typedef struct _dirdesc {
    int     dd_fd;      /** file descriptor associated with directory */
    long    dd_loc;     /** offset in current buffer */
    long    dd_size;    /** amount of data returned by getdirentries */
    char* dd_buf;    /** data buffer */
    int     dd_len;     /** size of data buffer */
    long    dd_seek;    /** magic cookie returned by getdirentries */
    HANDLE handle;
    struct dirent* index;
} DIR;
# define __dirfd(dp)    ((dp)->dd_fd)

int mkdir(const char* path, int mode);
DIR* opendir(const char*);
int closedir(DIR*);
struct dirent* readdir(DIR*);

#endif // defined(_WIN32)

// using namespace std;

class File
{
public:
    File(const std::string& filePath);
    File();
    ~File();
public:
    //创建路径
    static bool createDir(const char *file, unsigned int mod);
    //新建文件，目录文件夹自动生成
    static FILE *createFile(const char *file, const char *mode);
    //判断是否为目录
    static bool isDir(const char *path) ;
    //判断是否为常规文件
    static bool isFile(const char *path) ;
    //判断是否是特殊目录（. or ..）
    static bool isSpecialDir(const char *path);
    //删除目录或文件
    static void deleteFile(const char *path) ;

    /**
     * 加载文件内容至string
     * @param path 加载的文件路径
     * @return 文件内容
     */
    static std::string loadFile(const char *path);

    /**
     * 保存内容至文件
     * @param data 文件内容
     * @param path 保存的文件路径
     * @return 是否保存成功
     */
    static bool saveFile(const std::string &data,const char *path);

    /**
     * 获取父文件夹
     * @param path 路径
     * @return 文件夹
     */
    static std::string parentDir(const std::string &path);

    /**
     * 替换"../"，获取绝对路径
     * @param path 相对路径，里面可能包含 "../"
     * @param currentPath 当前目录
     * @param canAccessParent 能否访问父目录之外的目录
     * @return 替换"../"之后的路径
     */
    static std::string absolutePath(const std::string &path, const std::string &currentPath,bool canAccessParent = false);

    /**
     * 遍历文件夹下的所有文件
     * @param path 文件夹路径
     * @param cb 回调对象 ，path为绝对路径，isDir为该路径是否为文件夹，返回true代表继续扫描，否则中断
     * @param enterSubdirectory 是否进入子目录扫描
     */
    static void scanDir(const std::string &path,const std::function<bool(const std::string &path,bool isDir)> &cb, bool enterSubdirectory = false);

public:
    bool open(const std::string& filePath, const std::string& mode);
    void close();
    int getFileSize();
    StreamBuffer::Ptr read(int size = 1024 * 1024);
    int read(char* data, int size = 1024 * 1024);
    bool write(const Buffer::Ptr& buffer);
    bool write(const char* data, int size);
    void seek(uint64_t size);
    bool seek_end();
    uint64_t tell();
    bool isEnd();
    void rewind();
    bool seek_from_cur(int64_t size);
    bool seek_from_end(int64_t size);
private:
    FILE* _fp = nullptr;
    std::string _filePath;
};

#endif /* SRC_UTIL_FILE_H_ */
