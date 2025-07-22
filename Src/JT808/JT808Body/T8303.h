#ifndef T8303_H
#define T8303_H

#include <vector>
#include <string>

class T8303 {
public:
    // 嵌套类 Info
    class Info {
    private:
        int type;       // 信息类型，占用 1 字节
        std::string name; // 信息名称，长度可变，按 2 字节为单位计算长度

    public:
        Info();
        ~Info();

        // Getter 方法
        int getType() const;
        const std::string& getName() const;

        // Setter 方法，支持链式调用
        Info& setType(int newType);
        Info& setName(const std::string& newName);
    };

private:
    int type;           // 设置类型，占用 1 字节
    std::vector<Info> infos; // 信息项，总单元 1 字节表示列表长度

public:
    T8303();
    ~T8303();

    // Getter 方法
    int getType() const;
    const std::vector<Info>& getInfos() const;

    // Setter 方法，支持链式调用
    T8303& setType(int newType);
    T8303& setInfos(const std::vector<Info>& newInfos);

    // 添加信息项方法
    T8303& addInfo(int type, const std::string& name);
};

#endif // T8303_H
