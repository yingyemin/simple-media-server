#ifndef T8801_H
#define T8801_H

class T8801 {
private:
    int channelId;  // 通道 ID(大于 0)，占用 1 字节
    int command;    // 拍摄命令：0 表示停止拍摄；65535 表示录像；其它表示拍照张数，占用 2 字节
    int time;       // 拍照间隔/录像时间(秒) 0 表示按最小间隔拍照或一直录像，占用 2 字节
    int save;       // 保存标志：1.保存 0.实时上传，占用 1 字节
    int resolution; // 分辨率，占用 1 字节
    int quality;    // 图像/视频质量(1~10)：1.代表质量损失最小 10.表示压缩比最大，占用 1 字节
    int brightness; // 亮度(0~255)，占用 1 字节
    int contrast;   // 对比度(0~127)，占用 1 字节
    int saturation; // 饱和度(0~127)，占用 1 字节
    int chroma;     // 色度(0~255)，占用 1 字节

public:
    T8801();
    ~T8801() = default;

    // Getter 方法
    int getChannelId() const;
    int getCommand() const;
    int getTime() const;
    int getSave() const;
    int getResolution() const;
    int getQuality() const;
    int getBrightness() const;
    int getContrast() const;
    int getSaturation() const;
    int getChroma() const;

    // Setter 方法，支持链式调用
    T8801& setChannelId(int newChannelId);
    T8801& setCommand(int newCommand);
    T8801& setTime(int newTime);
    T8801& setSave(int newSave);
    T8801& setResolution(int newResolution);
    T8801& setQuality(int newQuality);
    T8801& setBrightness(int newBrightness);
    T8801& setContrast(int newContrast);
    T8801& setSaturation(int newSaturation);
    T8801& setChroma(int newChroma);
};

#endif // T8801_H
