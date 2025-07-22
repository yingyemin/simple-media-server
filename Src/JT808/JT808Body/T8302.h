#ifndef T8302_H
#define T8302_H

#include <string>
#include <vector>

class T8302 {
public:
    // 嵌套类 Option
    class Option {
    private:
        int id;          // 答案 ID，占用 1 字节
        int length;      // 答案内容长度，占用 2 字节
        std::string content; // 答案内容，长度可变

    public:
        Option();
        ~Option();

        // Getter 方法
        int getId() const;
        const std::string& getContent() const;

        // Setter 方法，支持链式调用
        Option& setId(int newId);
        Option& setContent(const std::string& newContent);
    };

private:
    int sign;          // 标志，占用 1 字节
    std::string content; // 问题，长度可变，每字符 1 字节
    std::vector<Option> options; // 候选答案列表

public:
    T8302();
    ~T8302();

    // Getter 方法
    int getSign() const;
    const std::string& getContent() const;
    const std::vector<Option>& getOptions() const;

    // Setter 方法，支持链式调用
    T8302& setSign(int newSign);
    T8302& setContent(const std::string& newContent);
    T8302& setOptions(const std::vector<Option>& newOptions);

    // 添加选项方法
    T8302& addOption(int id, const std::string& content);
};

#endif // T8302_H
