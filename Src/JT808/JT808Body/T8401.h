#ifndef T8401_H
#define T8401_H

#include <string>
#include <vector>

class T8401 {
public:
    // 嵌套类 Contact
    class Contact {
    private:
        int sign;       // 标志，占用 1 字节
        int phoneLength; // 占一个字节
        std::string phone; // 电话号码，长度可变，按 1 字节为单位计算长度
        int nameLength; // 占一个字节
        std::string name;  // 联系人，长度可变，按 1 字节为单位计算长度

    public:
        Contact();
        ~Contact();

        // Getter 方法
        int getSign() const;
        const std::string& getPhone() const;
        const std::string& getName() const;

        // Setter 方法，支持链式调用
        Contact& setSign(int newSign);
        Contact& setPhone(const std::string& newPhone);
        Contact& setName(const std::string& newName);
    };

private:
    int type;           // 类型，占用 1 字节
    std::vector<Contact> contacts; // 联系人项，总单元 1 字节表示列表长度

public:
    T8401();
    ~T8401();

    // Getter 方法
    int getType() const;
    const std::vector<Contact>& getContacts() const;

    // Setter 方法，支持链式调用
    T8401& setType(int newType);
    T8401& setContacts(const std::vector<Contact>& newContacts);

    // 添加联系人方法
    T8401& addContact(const Contact& contact);
};

#endif // T8401_H
