#ifndef T8804_H
#define T8804_H

class T8804 {
private:
    int command;              // 录音命令：0.停止录音 1.开始录音，占用 1 字节
    int time;                 // 录音时间(秒) 0.表示一直录音，占用 2 字节
    int save;                 // 保存标志：0.实时上传 1.保存，占用 1 字节
    int audioSamplingRate;    // 音频采样率：0.8K 1.11K 2.23K 3.32K 其他保留，占用 1 字节

public:
    T8804();
    ~T8804() = default;

    // Getter 方法
    int getCommand() const;
    int getTime() const;
    int getSave() const;
    int getAudioSamplingRate() const;

    // Setter 方法，支持链式调用
    T8804& setCommand(int newCommand);
    T8804& setTime(int newTime);
    T8804& setSave(int newSave);
    T8804& setAudioSamplingRate(int newAudioSamplingRate);
};

#endif // T8804_H
