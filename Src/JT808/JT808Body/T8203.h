#ifndef T8203_H
#define T8203_H

class T8203 {
private:
    int responseSerialNo; // 消息流水号，占用 2 字节
    // " [0]确认紧急报警" +
    //         " [1~2]保留" +
    //         " [3]确认危险预警" +
    //         " [4~19]保留" +
    //         " [20]确认进出区域报警" +
    //         " [21]确认进出路线报警" +
    //         " [22]确认路段行驶时间不足/过长报警" +
    //         " [23~26]保留" +
    //         " [27]确认车辆非法点火报警" +
    //         " [28]确认车辆非法位移报警" +
    //         " [29~31]保留")
    int type; // 报警类型，占用 4 字节

public:
    T8203();
    ~T8203();

    // Getter 方法
    int getResponseSerialNo() const;
    int getType() const;

    // Setter 方法，支持链式调用
    T8203& setResponseSerialNo(int newResponseSerialNo);
    T8203& setType(int newType);
};

#endif // T8203_H
