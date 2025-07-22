#include "T8401.h"

// Contact 类默认构造函数
T8401::Contact::Contact() : sign(0), phone(""), name("") {}

// Contact 类析构函数
T8401::Contact::~Contact() = default;

// Contact 类 Getter 方法实现
int T8401::Contact::getSign() const {
    return sign;
}

const std::string& T8401::Contact::getPhone() const {
    return phone;
}

const std::string& T8401::Contact::getName() const {
    return name;
}

// Contact 类 Setter 方法实现，支持链式调用
T8401::Contact& T8401::Contact::setSign(int newSign) {
    sign = newSign;
    return *this;
}

T8401::Contact& T8401::Contact::setPhone(const std::string& newPhone) {
    phone = newPhone;
    return *this;
}

T8401::Contact& T8401::Contact::setName(const std::string& newName) {
    name = newName;
    return *this;
}

// T8401 类默认构造函数
T8401::T8401() : type(0) {}

// T8401 类析构函数
T8401::~T8401() = default;

// T8401 类 Getter 方法实现
int T8401::getType() const {
    return type;
}

const std::vector<T8401::Contact>& T8401::getContacts() const {
    return contacts;
}

// T8401 类 Setter 方法实现，支持链式调用
T8401& T8401::setType(int newType) {
    type = newType;
    return *this;
}

T8401& T8401::setContacts(const std::vector<Contact>& newContacts) {
    contacts = newContacts;
    return *this;
}

// T8401 类添加联系人方法实现
T8401& T8401::addContact(const Contact& contact) {
    if (contacts.empty()) {
        contacts.reserve(2);
    }
    contacts.push_back(contact);
    return *this;
}
