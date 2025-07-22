#ifndef T9207_H
#define T9207_H

#include <cstdint>

/**
 * @class T9207
 * @brief 该类表示文件上传控制消息，用于封装文件上传控制相关信息
 */
class T9207 {
public:
    /**
     * @brief 构造函数，初始化成员变量
     */
    T9207();

    /**
     * @brief 获取应答流水号
     * @return 应答流水号，无符号 16 位整数
     */
    uint16_t getResponseSerialNo() const;

    /**
     * @brief 获取上传控制指令
     * @return 上传控制指令，无符号 8 位整数
     *         0. 暂停
     *         1. 继续
     *         2. 取消
     */
    uint8_t getCommand() const;

    /**
     * @brief 设置应答流水号
     * @param serialNo 应答流水号，无符号 16 位整数
     */
    void setResponseSerialNo(uint16_t serialNo);

    /**
     * @brief 设置上传控制指令
     * @param cmd 上传控制指令，无符号 8 位整数
     *            0. 暂停
     *            1. 继续
     *            2. 取消
     */
    void setCommand(uint8_t cmd);

private:
    // 应答流水号，占用 2 字节
    uint16_t responseSerialNo;
    // 上传控制指令，占用 1 字节
    uint8_t command;
};

#endif // T9207_H
