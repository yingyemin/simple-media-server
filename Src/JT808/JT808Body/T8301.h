#ifndef T8301_H
#define T8301_H

#include <vector>
#include <string>

class T8301 {
public:
    // 嵌套类 Event
    class Event {
    private:
        int id;         // 事件 ID，占用 1 字节
        std::string content; // 内容，长度可变

    public:
        Event();
        ~Event();

        // Getter 方法
        int getId() const;
        const std::string& getContent() const;

        // Setter 方法，支持链式调用
        Event& setId(int newId);
        Event& setContent(const std::string& newContent);
    };

private:
    int type;           // 设置类型：0.清空 1.更新(先清空,后追加) 2.追加 3.修改 4.指定删除，占用 1 字节
    std::vector<Event> events; // 事件项

public:
    T8301();
    ~T8301();

    // Getter 方法
    int getType() const;
    const std::vector<Event>& getEvents() const;

    // Setter 方法，支持链式调用
    T8301& setType(int newType);
    T8301& setEvents(const std::vector<Event>& newEvents);

    // 添加事件方法
    T8301& addEvent(int id, const std::string& content);
};

#endif // T8301_H
