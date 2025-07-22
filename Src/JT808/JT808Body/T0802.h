#ifndef T0802_H
#define T0802_H

#include <memory>
#include <vector>

#include "T0200.h"

class T0802 {
private:
    int responseSerialNo; // 应答流水号，占用 2 字节
    std::vector<std::unique_ptr<struct Item>> items; // 检索项

public:
    T0802();
    ~T0802();

    // Getter 方法
    int getResponseSerialNo() const;
    const std::vector<std::unique_ptr<Item>>& getItems() const;

    // Setter 方法，支持链式调用
    T0802& setResponseSerialNo(int newResponseSerialNo);
    T0802& addItem(std::unique_ptr<Item> newItem);
};

struct Item {
    int id;           // 多媒体数据 ID，占用 4 字节
    int type;         // 多媒体类型：0.图像 1.音频 2.视频，占用 1 字节
    int channelId;    // 通道 ID，占用 1 字节
    int event;        // 事件项编码，占用 1 字节
    std::unique_ptr<T0200_VersionMinus1And0> location; // 位置信息，占用 28 字节
    std::unique_ptr<T0200_Version1> location2019; // 位置信息，占用 28 字节

    Item() : id(0), type(0), channelId(0), event(0), location(nullptr), location2019(nullptr) {}
};

#endif // T0802_H
