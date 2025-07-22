#ifndef T9301_H
#define T9301_H

#include <cstdint>

/**
 * @class T9301
 * @brief 该类表示云台旋转消息，用于封装云台旋转相关的控制信息
 */
class T9301 {
public:
    /**
     * @brief 构造函数，初始化成员变量
     */
    T9301();

    /**
     * @brief 获取逻辑通道号
     * @return 逻辑通道号，无符号 8 位整数
     */
    uint8_t getChannelNo() const;

    /**
     * @brief 获取云台旋转方向
     * @return 云台旋转方向，无符号 8 位整数
     *         0. 停止
     *         1. 上
     *         2. 下
     *         3. 左
     *         4. 右
     */
    uint8_t getDirection() const;

    /**
     * @brief 获取云台旋转速度
     * @return 云台旋转速度，无符号 8 位整数，范围 0 - 255
     */
    uint8_t getSpeed() const;

    /**
     * @brief 设置逻辑通道号
     * @param no 逻辑通道号，无符号 8 位整数
     */
    void setChannelNo(uint8_t no);

    /**
     * @brief 设置云台旋转方向
     * @param direction 云台旋转方向，无符号 8 位整数
     *                  0. 停止
     *                  1. 上
     *                  2. 下
     *                  3. 左
     *                  4. 右
     */
    void setDirection(uint8_t direction);

    /**
     * @brief 设置云台旋转速度
     * @param speed 云台旋转速度，无符号 8 位整数，范围 0 - 255
     */
    void setSpeed(uint8_t speed);

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 云台旋转方向，占用 1 字节
    uint8_t direction;
    // 云台旋转速度，占用 1 字节，范围 0 - 255
    uint8_t speed;
};

#endif // T9301_H
