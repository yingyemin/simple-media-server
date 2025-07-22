#ifndef T9302_H
#define T9302_H

#include <cstdint>

/**
 * @class T9302
 * @brief 该类代表云台控制消息，用于封装云台调整焦距、光圈、雨刷、红外补光和变倍等控制相关信息
 */
class T9302 {
public:
    /**
     * @brief 构造函数，初始化成员变量
     */
    T9302();

    /**
     * @brief 获取逻辑通道号
     * @return 逻辑通道号，无符号 8 位整数
     */
    uint8_t getChannelNo() const;

    /**
     * @brief 获取控制参数
     * @return 控制参数，无符号 8 位整数 
     *         对于调整焦距、光圈和变倍：0. 调大 1. 调小
     *         对于雨刷和红外补光：0. 停止 1. 启动
     */
    uint8_t getParam() const;

    /**
     * @brief 设置逻辑通道号
     * @param no 逻辑通道号，无符号 8 位整数
     */
    void setChannelNo(uint8_t no);

    /**
     * @brief 设置控制参数
     * @param param 控制参数，无符号 8 位整数 
     *              对于调整焦距、光圈和变倍：0. 调大 1. 调小
     *              对于雨刷和红外补光：0. 停止 1. 启动
     */
    void setParam(uint8_t param);

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 控制参数，占用 1 字节
    uint8_t param;
};

#endif // T9302_H
