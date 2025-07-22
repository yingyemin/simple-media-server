#ifndef T8701_H
#define T8701_H

#include <vector>

class T8701 {
private:
    // " 130.设置车辆信息" +
    //         " 131.设置记录仪初次安装日期" +
    //         " 132.设置状态童配置信息" +
    //         " 194.设置记录仪时间" +
    //         " 195.设置记录仪脉冲系数" +
    //         " 196.设置初始里程" +
    //         " 197~207.预留")
    int type; // 命令字，占用 1 字节
    std::vector<unsigned char> content; // 数据块(参考 GB/T 19056)

public:
    T8701();
    ~T8701() = default;

    // Getter 方法
    int getType() const;
    const std::vector<unsigned char>& getContent() const;

    // Setter 方法，支持链式调用
    T8701& setType(int newType);
    T8701& setContent(const std::vector<unsigned char>& newContent);
    T8701& addContentByte(unsigned char byte);
};

#endif // T8701_H
