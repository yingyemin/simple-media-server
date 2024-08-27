/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include <cstdlib>
#include <stdexcept>
#include "Buffer.h"

StreamBuffer::~StreamBuffer()
{
    if (_data && _free) {
        delete[] _data;
        _data = nullptr;
    }
}

StreamBuffer::StreamBuffer(size_t capacity) 
{
    if (capacity) {
        setCapacity(capacity);
    }
}

StreamBuffer::StreamBuffer(const char *data, size_t size) 
{
    assign(data, size);
}

StreamBuffer::Ptr StreamBuffer::create() 
{
    return Ptr(new StreamBuffer);
}

char* StreamBuffer::data() const 
{
    return _data + _offset;
}

size_t StreamBuffer::size() const 
{
    return _size;
}

void StreamBuffer::setCapacity(size_t capacity) 
{
    if (_data) {
        do {
            if (capacity > _capacity) {
                //请求的内存大于当前内存，那么重新分配
                break;
            }

            if (_capacity < 2 * 1024) {
                //2K以下，不重复开辟内存，直接复用
                return;
            }

            if (2 * capacity > _capacity) {
                //如果请求的内存大于当前内存的一半，那么也复用
                return;
            }
        } while (false);

        delete[] _data;
    }
    _data = new char[capacity];
    _capacity = capacity;
    _size = capacity - 1;
}

void StreamBuffer::setSize(size_t size) 
{
    if (size > _capacity) {
        throw std::invalid_argument("Buffer::setSize out of range");
    }
    _size = size;
}

void StreamBuffer::assign(const char *data, size_t size) 
{
    if (size <= 0) {
        size = strlen(data);
    }
    setCapacity(size + 1);
    memcpy(_data, data, size);
    _data[size] = '\0';
    setSize(size);
}

void StreamBuffer::move(char *data, size_t size, int free) 
{
    _free = free;
    if (size <= 0) {
        size = strlen(data);
    }
    _capacity = size + 1;
    _data = data;
    setSize(size);
}

void StreamBuffer::substr(size_t offset, size_t size) 
{
    assert(offset + size <= _size);
    if (!size) {
        size = _size - offset;
    }
    _size = size;
    _capacity = size + 1;
    _offset += offset;
}


///////////////////////////////////////////////////////////////////

StringBuffer::StringBuffer() 
{
    _erase_head = 0;
    _erase_tail = 0;
}

StringBuffer::StringBuffer(std::string str)
{
    _str = std::move(str);
    _erase_head = 0;
    _erase_tail = 0;
}

StringBuffer& StringBuffer::operator=(std::string str) 
{
    _str = std::move(str);
    _erase_head = 0;
    _erase_tail = 0;
    return *this;
}

StringBuffer::StringBuffer(const char *str) {
    _str = str;
    _erase_head = 0;
    _erase_tail = 0;
}

StringBuffer &StringBuffer::operator=(const char *str) {
    _str = str;
    _erase_head = 0;
    _erase_tail = 0;
    return *this;
}

StringBuffer::StringBuffer(StringBuffer &&that) {
    _str = std::move(that._str);
    _erase_head = that._erase_head;
    _erase_tail = that._erase_tail;
    that._erase_head = 0;
    that._erase_tail = 0;
}

StringBuffer &StringBuffer::operator=(StringBuffer &&that) {
    _str = std::move(that._str);
    _erase_head = that._erase_head;
    _erase_tail = that._erase_tail;
    that._erase_head = 0;
    that._erase_tail = 0;
    return *this;
}

StringBuffer::StringBuffer(const StringBuffer &that) {
    _str = that._str;
    _erase_head = that._erase_head;
    _erase_tail = that._erase_tail;
}

StringBuffer& StringBuffer::operator=(const StringBuffer &that) 
{
    _str = that._str;
    _erase_head = that._erase_head;
    _erase_tail = that._erase_tail;
    return *this;
}

char *StringBuffer::data() const 
{
    return (char *) _str.data() + _erase_head;
}

size_t StringBuffer::size() const 
{
    return _str.size() - _erase_tail - _erase_head;
}

StringBuffer &StringBuffer::erase(size_t pos, size_t n) 
{
    if (pos == 0) {
        //移除前面的数据
        if (n != std::string::npos) {
            //移除部分
            if (n > size()) {
                //移除太多数据了
                throw std::out_of_range("StringBuffer::erase out_of_range in head");
            }
            //设置起始便宜量
            _erase_head += n;
            data()[size()] = '\0';
            return *this;
        }
        //移除全部数据
        _erase_head = 0;
        _erase_tail = _str.size();
        data()[0] = '\0';
        return *this;
    }

    if (n == std::string::npos || pos + n >= size()) {
        //移除末尾所有数据
        if (pos >= size()) {
            //移除太多数据
            throw std::out_of_range("StringBuffer::erase out_of_range in tail");
        }
        _erase_tail += size() - pos;
        data()[size()] = '\0';
        return *this;
    }

    //移除中间的
    if (pos + n > size()) {
        //超过长度限制
        throw std::out_of_range("StringBuffer::erase out_of_range in middle");
    }
    _str.erase(_erase_head + pos, n);
    return *this;
}

StringBuffer &StringBuffer::append(const StringBuffer &str) 
{
    return append(str.data(), str.size());
}

StringBuffer &StringBuffer::append(const std::string &str) 
{
    return append(str.data(), str.size());
}

StringBuffer &StringBuffer::append(const char *data) 
{
    return append(data, strlen(data));
}

StringBuffer &StringBuffer::append(const char *data, size_t len) 
{
    if (len <= 0) {
        return *this;
    }
    if (_erase_head > _str.capacity() / 2) {
        moveData();
    }
    if (_erase_tail == 0) {
        _str.append(data, len);
        return *this;
    }
    _str.insert(_erase_head + size(), data, len);
    return *this;
}

void StringBuffer::push_back(char c) 
{
    if (_erase_tail == 0) {
        _str.push_back(c);
        return;
    }
    data()[size()] = c;
    --_erase_tail;
    data()[size()] = '\0';
}

StringBuffer &StringBuffer::insert(size_t pos, const char *s, size_t n) 
{
    _str.insert(_erase_head + pos, s, n);
    return *this;
}

StringBuffer &StringBuffer::assign(const string& data) 
{
    return assign(data.data(), data.size());
}

StringBuffer &StringBuffer::assign(const char *data) 
{
    return assign(data, strlen(data));
}

StringBuffer &StringBuffer::assign(const char *data, size_t len) 
{
    if (len <= 0) {
        return *this;
    }
    if (data >= _str.data() && data < _str.data() + _str.size()) {
        _erase_head = data - _str.data();
        if (data + len > _str.data() + _str.size()) {
            throw std::out_of_range("StringBuffer::assign out_of_range");
        }
        _erase_tail = _str.data() + _str.size() - (data + len);
        return *this;
    }
    _str.assign(data, len);
    _erase_head = 0;
    _erase_tail = 0;
    return *this;
}

void StringBuffer::clear() 
{
    _erase_head = 0;
    _erase_tail = 0;
    _str.clear();
}

char &StringBuffer::operator[](size_t pos) 
{
    if (pos >= size()) {
        throw std::out_of_range("StringBuffer::operator[] out_of_range");
    }
    return data()[pos];
}

const char &StringBuffer::operator[](size_t pos) const 
{
    return (*const_cast<StringBuffer *>(this))[pos];
}

size_t StringBuffer::capacity() const 
{
    return _str.capacity();
}

void StringBuffer::reserve(size_t size) 
{
    _str.reserve(size);
}

void StringBuffer::resize(size_t size, char c) 
{
    _str.resize(size, c);
    _erase_head = 0;
    _erase_tail = 0;
}

bool StringBuffer::empty() const 
{
    return size() <= 0;
}

std::string StringBuffer::substr(size_t pos, size_t n) const 
{
    if (n == std::string::npos) {
        //获取末尾所有的
        if (pos >= size()) {
            throw std::out_of_range("StringBuffer::substr out_of_range");
        }
        return _str.substr(_erase_head + pos, size() - pos);
    }

    //获取部分
    if (pos + n > size()) {
        throw std::out_of_range("StringBuffer::substr out_of_range");
    }
    return _str.substr(_erase_head + pos, n);
}

void StringBuffer::substr(size_t offset, size_t size) 
{
    assert(offset + _erase_head + size <= _erase_tail);
    if (!size) {
        size = _erase_tail - offset - _erase_head;
    }
    _erase_head += offset;
    _erase_tail = _erase_head + size;
}

void StringBuffer::moveData() 
{
    if (_erase_head) {
        _str.erase(0, _erase_head);
        _erase_head = 0;
    }
}