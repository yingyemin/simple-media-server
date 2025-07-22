#ifndef T9201_H
#define T9201_H

#include <cstdint>
#include <string>

/**
 * @class T9201
 * @brief 平台下发远程录像回放请求类，用于封装平台下发的远程录像回放请求相关信息
 */
class T9201 {
public:
    /**
     * @brief 构造函数，用于初始化类的成员变量
     */
    T9201();

    /**
     * @brief 获取服务器 IP 地址
     * @return 服务器 IP 地址的字符串表示
     */
    std::string getIp() const;

    /**
     * @brief 获取实时视频服务器 TCP 端口号
     * @return 实时视频服务器 TCP 端口号，无符号 16 位整数
     */
    uint16_t getTcpPort() const;

    /**
     * @brief 获取实时视频服务器 UDP 端口号
     * @return 实时视频服务器 UDP 端口号，无符号 16 位整数
     */
    uint16_t getUdpPort() const;

    /**
     * @brief 获取逻辑通道号
     * @return 逻辑通道号，无符号 8 位整数
     */
    uint8_t getChannelNo() const;

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
     *         2. 子码流 (如果此通道只传输音频, 此字段置 0)
     */
    uint8_t getStreamType() const;

    /**
     * @brief 获取存储器类型
     * @return 存储器类型，无符号 8 位整数
     *         0. 所有存储器
     *         1. 主存储器
     *         2. 灾备存储器
     */
    uint8_t getStorageType() const;

    /**
     * @brief 获取回放方式
     * @return 回放方式，无符号 8 位整数
     *         0. 正常回放
     *         1. 快进回放
     *         2. 关键帧快退回放
     *         3. 关键帧播放
     *         4. 单帧上传
     */
    uint8_t getPlaybackMode() const;

    /**
     * @brief 获取快进或快退倍数
     * @return 快进或快退倍数，无符号 8 位整数
     *         0. 无效
     *         1. 1 倍
     *         2. 2 倍
     *         3. 4 倍
     *         4. 8 倍
     *         5. 16 倍 (回放控制为 1 和 2 时, 此字段内容有效, 否则置 0)
     */
    uint8_t getPlaybackSpeed() const;

    /**
     * @brief 获取开始时间
     * @return 开始时间的字符串表示，格式为 YYMMDDHHMMSS
     *         回放方式为 4 时, 该字段表示单帧上传时间
     */
    std::string getStartTime() const;

    /**
     * @brief 获取结束时间
     * @return 结束时间的字符串表示，格式为 YYMMDDHHMMSS
     *         回放方式为 4 时, 该字段无效, 为 0 表示一直回放
     */
    std::string getEndTime() const;

    /**
     * @brief 设置服务器 IP 地址
     * @param ip 服务器 IP 地址的字符串表示
     */
    void setIp(const std::string& ip);

    /**
     * @brief 设置实时视频服务器 TCP 端口号
     * @param port 实时视频服务器 TCP 端口号，无符号 16 位整数
     */
    void setTcpPort(uint16_t port);

    /**
     * @brief 设置实时视频服务器 UDP 端口号
     * @param port 实时视频服务器 UDP 端口号，无符号 16 位整数
     */
    void setUdpPort(uint16_t port);

    /**
     * @brief 设置逻辑通道号
     * @param no 逻辑通道号，无符号 8 位整数
     */
    void setChannelNo(uint8_t no);

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
     *             2. 子码流 (如果此通道只传输音频, 此字段置 0)
     */
    void setStreamType(uint8_t type);

    /**
     * @brief 设置存储器类型
     * @param type 存储器类型，无符号 8 位整数
     *             0. 所有存储器
     *             1. 主存储器
     *             2. 灾备存储器
     */
    void setStorageType(uint8_t type);

    /**
     * @brief 设置回放方式
     * @param mode 回放方式，无符号 8 位整数
     *             0. 正常回放
     *             1. 快进回放
     *             2. 关键帧快退回放
     *             3. 关键帧播放
     *             4. 单帧上传
     */
    void setPlaybackMode(uint8_t mode);

    /**
     * @brief 设置快进或快退倍数
     * @param speed 快进或快退倍数，无符号 8 位整数
     *              0. 无效
     *              1. 1 倍
     *              2. 2 倍
     *              3. 4 倍
     *              4. 8 倍
     *              5. 16 倍 (回放控制为 1 和 2 时, 此字段内容有效, 否则置 0)
     */
    void setPlaybackSpeed(uint8_t speed);

    /**
     * @brief 设置开始时间
     * @param time 开始时间的字符串表示，格式为 YYMMDDHHMMSS
     *             回放方式为 4 时, 该字段表示单帧上传时间
     */
    void setStartTime(const std::string& time);

    /**
     * @brief 设置结束时间
     * @param time 结束时间的字符串表示，格式为 YYMMDDHHMMSS
     *             回放方式为 4 时, 该字段无效, 为 0 表示一直回放
     */
    void setEndTime(const std::string& time);

private:
    // 服务器 IP 地址
    std::string ip;
    // 实时视频服务器 TCP 端口号，占用 2 字节
    uint16_t tcpPort;
    // 实时视频服务器 UDP 端口号，占用 2 字节
    uint16_t udpPort;
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 音视频资源类型，占用 1 字节
    uint8_t mediaType;
    // 码流类型，占用 1 字节
    uint8_t streamType;
    // 存储器类型，占用 1 字节
    uint8_t storageType;
    // 回放方式，占用 1 字节
    uint8_t playbackMode;
    // 快进或快退倍数，占用 1 字节
    uint8_t playbackSpeed;
    // 开始时间，格式为 YYMMDDHHMMSS，在数据传输时以 6 字节 BCD 码表示
    std::string startTime;
    // 结束时间，格式为 YYMMDDHHMMSS，在数据传输时以 6 字节 BCD 码表示
    std::string endTime;
};

#endif // T9201_H
