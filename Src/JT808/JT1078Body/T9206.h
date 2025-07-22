#ifndef T9206_H
#define T9206_H

#include <cstdint>
#include <string>

/**
 * @class T9206
 * @brief 平台下发文件上传指令类，用于封装文件上传相关的指令信息
 */
class T9206 {
public:
    /**
     * @brief 构造函数，初始化类的成员变量为默认值
     */
    T9206();

    /**
     * @brief 获取服务器地址
     * @return 服务器地址的字符串表示
     */
    std::string getIp() const;

    /**
     * @brief 获取端口号
     * @return 端口号，无符号 16 位整数
     */
    uint16_t getPort() const;

    /**
     * @brief 获取用户名
     * @return 用户名的字符串表示
     */
    std::string getUsername() const;

    /**
     * @brief 获取密码
     * @return 密码的字符串表示
     */
    std::string getPassword() const;

    /**
     * @brief 获取文件上传路径
     * @return 文件上传路径的字符串表示
     */
    std::string getPath() const;

    /**
     * @brief 获取逻辑通道号
     * @return 逻辑通道号，无符号 8 位整数
     */
    uint8_t getChannelNo() const;

    /**
     * @brief 获取开始时间
     * @return 开始时间的字符串表示，格式为 YYMMDDHHMMSS
     */
    std::string getStartTime() const;

    /**
     * @brief 获取结束时间
     * @return 结束时间的字符串表示，格式为 YYMMDDHHMMSS
     */
    std::string getEndTime() const;

    /**
     * @brief 获取报警标志 0 - 31 位
     * @return 报警标志 0 - 31 位，无符号 32 位整数
     */
    uint32_t getWarnBit1() const;

    /**
     * @brief 获取报警标志 32 - 63 位
     * @return 报警标志 32 - 63 位，无符号 32 位整数
     */
    uint32_t getWarnBit2() const;

    /**
     * @brief 获取音视频资源类型
     * @return 音视频资源类型，无符号 8 位整数
     *         0. 音视频
     *         1. 音频
     *         2. 视频
     *         3. 视频或音视频
     */
    uint8_t getMediaType() const;

    /**
     * @brief 获取码流类型
     * @return 码流类型，无符号 8 位整数
     *         0. 所有码流
     *         1. 主码流
     *         2. 子码流
     */
    uint8_t getStreamType() const;

    /**
     * @brief 获取存储位置
     * @return 存储位置，无符号 8 位整数
     *         0. 所有存储器
     *         1. 主存储器
     *         2. 灾备存储器
     */
    uint8_t getStorageType() const;

    /**
     * @brief 获取任务执行条件
     * @return 任务执行条件，无符号 8 位整数，用 bit 位表示
     *         [0] WIFI 下可下载
     *         [1] LAN 连接时可下载
     *         [2] 3G/4G 连接时可下载
     */
    uint8_t getCondition() const;

    /**
     * @brief 设置服务器地址
     * @param ip 服务器地址的字符串表示
     */
    void setIp(const std::string& ip);

    /**
     * @brief 设置端口号
     * @param port 端口号，无符号 16 位整数
     */
    void setPort(uint16_t port);

    /**
     * @brief 设置用户名
     * @param username 用户名的字符串表示
     */
    void setUsername(const std::string& username);

    /**
     * @brief 设置密码
     * @param password 密码的字符串表示
     */
    void setPassword(const std::string& password);

    /**
     * @brief 设置文件上传路径
     * @param path 文件上传路径的字符串表示
     */
    void setPath(const std::string& path);

    /**
     * @brief 设置逻辑通道号
     * @param no 逻辑通道号，无符号 8 位整数
     */
    void setChannelNo(uint8_t no);

    /**
     * @brief 设置开始时间
     * @param time 开始时间的字符串表示，格式为 YYMMDDHHMMSS
     */
    void setStartTime(const std::string& time);

    /**
     * @brief 设置结束时间
     * @param time 结束时间的字符串表示，格式为 YYMMDDHHMMSS
     */
    void setEndTime(const std::string& time);

    /**
     * @brief 设置报警标志 0 - 31 位
     * @param bit 报警标志 0 - 31 位，无符号 32 位整数
     */
    void setWarnBit1(uint32_t bit);

    /**
     * @brief 设置报警标志 32 - 63 位
     * @param bit 报警标志 32 - 63 位，无符号 32 位整数
     */
    void setWarnBit2(uint32_t bit);

    /**
     * @brief 设置音视频资源类型
     * @param type 音视频资源类型，无符号 8 位整数
     *             0. 音视频
     *             1. 音频
     *             2. 视频
     *             3. 视频或音视频
     */
    void setMediaType(uint8_t type);

    /**
     * @brief 设置码流类型
     * @param type 码流类型，无符号 8 位整数
     *             0. 所有码流
     *             1. 主码流
     *             2. 子码流
     */
    void setStreamType(uint8_t type);

    /**
     * @brief 设置存储位置
     * @param type 存储位置，无符号 8 位整数
     *             0. 所有存储器
     *             1. 主存储器
     *             2. 灾备存储器
     */
    void setStorageType(uint8_t type);

    /**
     * @brief 设置任务执行条件
     * @param condition 任务执行条件，无符号 8 位整数，用 bit 位表示
     *                  [0] WIFI 下可下载
     *                  [1] LAN 连接时可下载
     *                  [2] 3G/4G 连接时可下载
     */
    void setCondition(uint8_t condition);

private:
    // 服务器地址
    std::string ip;
    // 端口号，占用 2 字节
    uint16_t port;
    // 用户名
    std::string username;
    // 密码
    std::string password;
    // 文件上传路径
    std::string path;
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 开始时间，格式为 YYMMDDHHMMSS，在数据传输时以 6 字节 BCD 码表示
    std::string startTime;
    // 结束时间，格式为 YYMMDDHHMMSS，在数据传输时以 6 字节 BCD 码表示
    std::string endTime;
    // 报警标志 0 - 31 位，参考 808 协议文档报警标志位定义，占用 4 字节
    uint32_t warnBit1;
    // 报警标志 32 - 63 位，占用 4 字节
    uint32_t warnBit2;
    // 音视频资源类型，占用 1 字节
    uint8_t mediaType;
    // 码流类型，占用 1 字节
    uint8_t streamType;
    // 存储位置，占用 1 字节
    uint8_t storageType;
    // 任务执行条件，占用 1 字节，默认值为 -1 对应的无符号值 255
    uint8_t condition;
};

#endif // T9206_H
