#ifndef T0705_H
#define T0705_H

#include <string>
#include <vector>

class T0705 {
private:
    class Item {
    private:
        int id; // CAN ID，占用 4 字节
        std::vector<unsigned char> data; // CAN DATA，占用 8 字节

    public:
        Item();
        ~Item();

        // Getter 方法
        int getId() const;
        const std::vector<unsigned char>& getData() const;

        // Setter 方法，支持链式调用
        Item& setId(int id);
        Item& setData(const std::vector<unsigned char>& data);
    };

    int total; // 数据项个数(大于 0)，占用 2 字节
    std::string dateTime; // CAN 总线数据接收时间(HHMMSSMSMS)，格式为 BCD，占用 5 字节
    std::vector<Item> items; // CAN 总线数据项

public:
    T0705();
    ~T0705();

    // Getter 方法
    int getTotal() const;
    const std::string& getDateTime() const;
    const std::vector<Item>& getItems() const;

    // Setter 方法，支持链式调用
    T0705& setTotal(int total);
    T0705& setDateTime(const std::string& dateTime);
    T0705& setItems(const std::vector<Item>& items);
};

#endif // T0705_H
