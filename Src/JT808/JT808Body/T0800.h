#ifndef T0800_H
#define T0800_H

class T0800 {
private:
    int id;        // 多媒体数据 ID，占用 4 字节
    int type;      // 多媒体类型：0.图像 1.音频 2.视频，占用 1 字节
    int format;    // 多媒体格式编码：0.JPEG 1.TIF 2.MP3 3.WAV 4.WMV，占用 1 字节
    int event;     // 事件项编码，占用 1 字节
    int channelId; // 通道 ID，占用 1 字节

public:
    T0800();
    ~T0800();

    // Getter 方法
    int getId() const;
    int getType() const;
    int getFormat() const;
    int getEvent() const;
    int getChannelId() const;

    // Setter 方法，支持链式调用
    T0800& setId(int newId);
    T0800& setType(int newType);
    T0800& setFormat(int newFormat);
    T0800& setEvent(int newEvent);
    T0800& setChannelId(int newChannelId);
};

#endif // T0800_H
