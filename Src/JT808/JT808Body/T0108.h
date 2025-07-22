#ifndef T0108_H
#define T0108_H

#include <string>

class T0108 {
private:
    // 升级类型：0.终端 12.道路运输证IC卡读卡器 52.北斗卫星定位模块，占用 1 字节
    int type;
    // 升级结果：0.成功 1.失败 2.取消，占用 1 字节
    int result;

public:
    // 终端
    static const int Terminal = 0;
    // 道路运输证IC卡 读卡器
    static const int CardReader = 12;
    // 北斗卫星定位模块
    static const int Beidou = 52;

    // 获取升级类型
    int getType() const;
    // 设置升级类型
    void setType(int t);
    // 获取升级结果
    int getResult() const;
    // 设置升级结果
    void setResult(int r);
};

#endif // T0108_H
