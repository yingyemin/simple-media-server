/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <cassert>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <type_traits>
#include <functional>

using namespace std;

//缓存基类
class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;

    Buffer() = default;
    virtual ~Buffer() = default;

    //返回数据长度
    virtual char *data() const = 0;
    virtual size_t size() const = 0;

    virtual std::string toString() const {
        return std::string(data(), size());
    }

    virtual void substr(size_t offset = 0, size_t size = 0) {}

    virtual size_t getCapacity() const {
        return size();
    }
};

// 用于存储申请后，长度不变的内存
class StreamBuffer : public Buffer {
public:
    using Ptr = std::shared_ptr<StreamBuffer>;

    static Ptr create();

    StreamBuffer(size_t capacity = 0);

    StreamBuffer(const char *data, size_t size = 0);

    ~StreamBuffer() override;

    //在写入数据时请确保内存是否越界
    char *data() const override;

    //有效数据大小
    size_t size() const override;

    size_t bufferSize() const;

    void resetSize();

    //分配内存大小
    void setCapacity(size_t capacity);

    //设置有效数据大小
    virtual void setSize(size_t size);

    //赋值数据
    void assign(const char *data, size_t size = 0);

    // 移动数据
    void move(char *data, size_t size = 0, int free = true);

    size_t getCapacity() const override {
        return _capacity;
    }

    void substr(size_t offset = 0, size_t size = 0);

private:
    bool _free = true;
    size_t _size = 0;
    size_t _capacity = 0;
    size_t _offset = 0;
    char* _data = nullptr;
};

// 用于存储需要动态增长的内存，如帧数据
class StringBuffer : public Buffer {
public:
    using Ptr = shared_ptr<StringBuffer>;

    ~StringBuffer() override = default;

    StringBuffer();

    StringBuffer(std::string str);

    StringBuffer &operator=(std::string str);

    StringBuffer(const char *str);

    StringBuffer &operator=(const char *str);

    StringBuffer(StringBuffer &&that);

    StringBuffer &operator=(StringBuffer &&that);

    StringBuffer(const StringBuffer &that);

    StringBuffer &operator=(const StringBuffer &that);

    char *data() const override;

    size_t size() const override;

    StringBuffer &erase(size_t pos = 0, size_t n = std::string::npos);

    StringBuffer &append(const StringBuffer &str);

    StringBuffer &append(const std::string &str);

    StringBuffer &append(const char *data);

    StringBuffer &append(const char *data, size_t len);

    void push_back(char c);

    StringBuffer &insert(size_t pos, const char *s, size_t n);

    StringBuffer &assign(const string& data);

    StringBuffer &assign(const char *data);

    StringBuffer &assign(const char *data, size_t len);

    void clear();

    char &operator[](size_t pos);

    const char &operator[](size_t pos) const;

    size_t capacity() const;

    void reserve(size_t size);

    void resize(size_t size, char c = '\0');

    bool empty() const;

    std::string substr(size_t pos, size_t n = std::string::npos) const;

    void substr(size_t offset = 0, size_t size = 0);

    std::string buffer() {return _str;}

private:
    void moveData();

private:
    size_t _erase_head;
    size_t _erase_tail;
    std::string _str;
};

#endif //BUFFER_H
