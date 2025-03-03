/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef Common_SourceDataQue_H_
#define Common_SourceDataQue_H_

#include <assert.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <functional>
#include "EventPoller/EventLoop.h"
#include "Log/Logger.h"

// using namespace std;

// GOP缓存最大长度下限值
#define RING_MIN_SIZE 32
#define LOCK_GUARD(mtx) std::lock_guard<decltype(mtx)> lck(mtx)

class ClientInfo
{
public:
    int port_;
    float bitrate_;
    string ip_;
    string protocol_;
    unordered_map<string, string> info_;
    function<void()> close_;
};

template <typename T>
class DataQueStorage;

template <typename T>
class DataQueReaderDispatcher;

/**
 * 环形缓存读取器
 * 该对象的事件触发都会在绑定的poller线程中执行
 * 所以把锁去掉了
 * 对该对象的一切操作都应该在poller线程中执行
 */
template <typename T>
class DataQueReader {
public:
    using Ptr = std::shared_ptr<DataQueReader>;
    friend class DataQueReaderDispatcher<T>;

    DataQueReader(std::shared_ptr<DataQueStorage<T>> storage);

    ~DataQueReader();

    void setReadCB(std::function<void(const T &)> cb);

    void setDetachCB(std::function<void()> cb);

    void setGetInfoCB(std::function<ClientInfo()> cb);

    void setMessageCB(std::function<void(const ClientInfo &data)> cb);

private:
    void onRead(const T &data, bool /*is_key*/);
    void onMessage(const ClientInfo &data);
    void onDetach() const;
    ClientInfo getInfo();

    void flushGop();

private:
    std::shared_ptr<DataQueStorage<T>> _storage;
    std::function<void(void)> _detach_cb;
    std::function<void(const T &)> _read_cb;
    std::function<ClientInfo()> _info_cb;
    std::function<void(const ClientInfo &data)> _msg_cb;
};

template <typename T>
class DataQueStorage {
public:
    using Ptr = std::shared_ptr<DataQueStorage>;
    using GopType = list<list<std::pair<bool, T>>>;
    
    DataQueStorage(size_t max_size, size_t max_gop_size);

    ~DataQueStorage() = default;

    /**
     * 写入环形缓存数据
     * @param in 数据
     * @param is_key 是否为关键帧
     * @return 是否触发重置环形缓存大小
     */
    void write(T in, bool is_key = true);

    Ptr clone() const;

    const GopType &getCache() const;

    void clearCache();

private:
    DataQueStorage() = default;

    void popFrontGop();

private:
    bool _started = false;
    bool _have_idr;
    size_t _size;
    size_t _max_size;
    size_t _max_gop_size;
    GopType _data_cache;
};

template <typename T>
class DataQue;

/**
 * 环形缓存事件派发器，只能一个poller线程操作它
 * @tparam T
 */
template <typename T>
class DataQueReaderDispatcher : public std::enable_shared_from_this<DataQueReaderDispatcher<T>> {
public:
    using Ptr = std::shared_ptr<DataQueReaderDispatcher>;
    using DataQueReaderT = DataQueReader<T>;
    using DataQueStorageT = DataQueStorage<T>;
    using onChangeInfoCB = std::function<ClientInfo(ClientInfo &&info)>;

    friend class DataQue<T>;

    ~DataQueReaderDispatcher();

private:
    DataQueReaderDispatcher(
        const typename DataQueStorageT::Ptr &storage, std::function<void(int, bool)> onSizeChanged);

    void write(T in, bool is_key = true);

    void sendMessage(const ClientInfo &data);

    std::shared_ptr<DataQueReaderT> attach(const EventLoop::Ptr &loop, bool use_cache);

    void onSizeChanged(bool add_flag);

    void clearCache();

    std::list<ClientInfo> getInfoList(const onChangeInfoCB &on_change);

private:
    std::atomic_int _reader_size;
    std::function<void(int, bool)> _on_size_changed;
    typename DataQueStorageT::Ptr _storage;
    std::unordered_map<void *, std::weak_ptr<DataQueReaderT>> _reader_map;
};

template <typename T>
class DataQue : public std::enable_shared_from_this<DataQue<T>> {
public:
    using Ptr = std::shared_ptr<DataQue>;
    using DataQueReaderT = DataQueReader<T>;
    using DataQueStorageT = DataQueStorage<T>;
    using DataQueReaderDispatcherT = DataQueReaderDispatcher<T>;
    using onReaderChanged = std::function<void(int size)>;
    using onWriteFunc = std::function<void(T in, bool is_key)>;
    using onGetInfoCB = std::function<void(std::list<ClientInfo> &info_list)>;

    DataQue(size_t max_size = 1024, onReaderChanged cb = nullptr, size_t max_gop_size = 1);

    ~DataQue();

    void write(T in, bool is_key = true);

    void sendMessage(const ClientInfo &data);

    int getOnWriteSize();

    void addOnWrite(void* key, onWriteFunc onWrite);

    void delOnWrite(void* key);

    void addBytes(int bytes);

    uint64_t getBytes();

    std::shared_ptr<DataQueReaderT> attach(const EventLoop::Ptr &loop, bool use_cache = true);

    int readerCount();

    void clearCache();

    void getInfoList(const onGetInfoCB &cb, const typename DataQueReaderDispatcherT::onChangeInfoCB &on_change = nullptr);

private:
    void onSizeChanged(const EventLoop::Ptr &loop, int size, bool add_flag);

private:
    struct HashOfPtr {
        std::size_t operator()(const EventLoop::Ptr &key) const { return (std::size_t)key.get(); }
    };

private:
    std::mutex _mtx_map;
    std::atomic_int _total_count { 0 };
    std::atomic_int _total_bytes { 0 };
    typename DataQueStorageT::Ptr _storage;
    std::unordered_map<void*, onWriteFunc> _on_write_map;
    onReaderChanged _on_reader_changed;
    std::unordered_map<EventLoop::Ptr, typename DataQueReaderDispatcherT::Ptr, HashOfPtr> _dispatcher_map;
};


///////////////////////////////////////////////////////////////////////

using namespace std;

template <typename T>
DataQueReader<T>::DataQueReader(std::shared_ptr<DataQueStorage<T>> storage)
{
    _storage = std::move(storage);
    setReadCB(nullptr);
    setDetachCB(nullptr);
    setGetInfoCB(nullptr);
    setMessageCB(nullptr);
}

template <typename T>
DataQueReader<T>::~DataQueReader()
{
    logInfo << "~_DataQueReader";
    onDetach();
}

template <typename T>
void DataQueReader<T>::setReadCB(std::function<void(const T &)> cb) 
{
    if (!cb) {
        _read_cb = [](const T &) {};
    } else {
        _read_cb = std::move(cb);
        flushGop();
    }
}

template <typename T>
void DataQueReader<T>::setDetachCB(std::function<void()> cb) 
{
    _detach_cb = cb ? std::move(cb) : []() {};
}

template <typename T>
void DataQueReader<T>::setGetInfoCB(std::function<ClientInfo()> cb) 
{
    _info_cb = cb ? std::move(cb) : []() { return ClientInfo(); };
}

template <typename T>
void DataQueReader<T>::setMessageCB(std::function<void(const ClientInfo &data)> cb) 
{
    _msg_cb = cb ? std::move(cb) : [](const ClientInfo &data) {};
}

template <typename T>
void DataQueReader<T>::onRead(const T &data, bool /*is_key*/) 
{ 
    _read_cb(data); 
}

template <typename T>
void DataQueReader<T>::onMessage(const ClientInfo &data)
{
    _msg_cb(data); 
}

template <typename T>
void DataQueReader<T>::onDetach() const
{ 
    _detach_cb(); 
}

template <typename T>
ClientInfo DataQueReader<T>::getInfo()
{ 
    if (_info_cb) {
        return _info_cb();
    }

    ClientInfo info;
    return info;
}

template <typename T>
void DataQueReader<T>::flushGop() 
{
    if (!_storage) {
        return;
    }
    // _storage->getCache().for_each([this](const list<std::pair<bool, T>> &lst) {
    //     lst.for_each([this](const std::pair<bool, T> &pr) { onRead(pr.second, pr.first); });
    // });
    auto cache = _storage->getCache();
    for (auto& list : cache) {
        for (auto& pr : list) {
            onRead(pr.second, pr.first);
        }
    }
}

///////////////////////////////////////////////////////////////////////

template <typename T>
DataQueStorage<T>::DataQueStorage(size_t max_size, size_t max_gop_size) 
{
    // gop缓存个数不能小于32
    if (max_size < RING_MIN_SIZE) {
        max_size = RING_MIN_SIZE;
    }
    _max_size = max_size;
    _max_gop_size = max_gop_size;
    clearCache();
}

/**
 * 写入环形缓存数据
 * @param in 数据
 * @param is_key 是否为关键帧
 * @return 是否触发重置环形缓存大小
 */
template <typename T>
void DataQueStorage<T>::write(T in, bool is_key) 
{
    if (is_key) {
        _have_idr = true;
        _started = true;
        if (!_data_cache.back().empty()) {
            //当前gop列队还没收到任意缓存
            _data_cache.emplace_back();
        }
        if (_data_cache.size() > _max_gop_size) {
            // GOP个数超过限制，那么移除最早的GOP
            popFrontGop();
        }
    }

    if (!_have_idr && _started) {
        //缓存中没有关键帧，那么gop缓存无效
        return;
    }
    _data_cache.back().emplace_back(std::make_pair(is_key, std::move(in)));
    if (++_size > _max_size) {
        // GOP缓存溢出
        while (_data_cache.size() > 1) {
            //先尝试清除老的GOP缓存
            popFrontGop();
        }
        if (_size > _max_size) {
            //还是大于最大缓冲限制，那么清空所有GOP
            clearCache();
        }
    }
}

template <typename T>
std::shared_ptr<DataQueStorage<T>> DataQueStorage<T>::clone() const
{
    Ptr ret(new DataQueStorage<T>());
    ret->_size = _size;
    ret->_have_idr = _have_idr;
    ret->_started = _started;
    ret->_max_size = _max_size;
    ret->_max_gop_size = _max_gop_size;
    ret->_data_cache = _data_cache;
    return ret;
}

template <typename T>
const list<list<std::pair<bool, T>>> &DataQueStorage<T>::getCache() const 
{ 
    return _data_cache; 
}

template <typename T>
void DataQueStorage<T>::clearCache()
{
    _size = 0;
    _have_idr = false;
    _data_cache.clear();
    _data_cache.emplace_back();
}


template <typename T>
void DataQueStorage<T>::popFrontGop()
{
    if (!_data_cache.empty()) {
        _size -= _data_cache.front().size();
        _data_cache.pop_front();
        if (_data_cache.empty()) {
            _data_cache.emplace_back();
        }
    }
}

///////////////////////////////////////////////////////////////////////

template <typename T>
DataQueReaderDispatcher<T>::~DataQueReaderDispatcher() 
{
    logInfo << "DataQueReaderDispatcher<T>::~DataQueReaderDispatcher() ";
    decltype(_reader_map) reader_map;
    reader_map.swap(_reader_map);
    for (auto &pr : reader_map) {
        auto reader = pr.second.lock();
        if (reader) {
            reader->onDetach();
        }
    }
}

template <typename T>
DataQueReaderDispatcher<T>::DataQueReaderDispatcher(
    const typename DataQueStorageT::Ptr &storage, std::function<void(int, bool)> onSizeChanged) 
{
    _reader_size = 0;
    _storage = storage;
    _on_size_changed = std::move(onSizeChanged);
    assert(_on_size_changed);
}

template <typename T>
void DataQueReaderDispatcher<T>::write(T in, bool is_key) 
{
    // logInfo << "write: " << _reader_map.size();
    for (auto it = _reader_map.begin(); it != _reader_map.end();) {
        auto reader = it->second.lock();
        // logInfo << "reader: " << reader;
        if (!reader) {
            it = _reader_map.erase(it);
            --_reader_size;
            onSizeChanged(false);
            continue;
        }
        reader->onRead(in, is_key);
        ++it;
    }
    _storage->write(std::move(in), is_key);
}

template <typename T>
void DataQueReaderDispatcher<T>::sendMessage(const ClientInfo &data) 
{
    for (auto it = _reader_map.begin(); it != _reader_map.end();) {
        auto reader = it->second.lock();
        if (!reader) {
            it = _reader_map.erase(it);
            --_reader_size;
            onSizeChanged(false);
            continue;
        }
        reader->onMessage(data);
        ++it;
    }
}

template <typename T>
std::shared_ptr<DataQueReader<T>> DataQueReaderDispatcher<T>::attach(const EventLoop::Ptr &loop, bool use_cache) 
{
    if (!loop->isCurrent()) {
        throw std::runtime_error("You can attach DataQue only in it's loop thread");
    }

    std::weak_ptr<DataQueReaderDispatcher<T>> weak_self = this->shared_from_this();
    auto on_dealloc = [weak_self, loop](DataQueReader<T> *ptr) {
        loop->async([weak_self, ptr]() {
            auto strong_self = weak_self.lock();
            if (strong_self && strong_self->_reader_map.erase(ptr)) {
                --strong_self->_reader_size;
                strong_self->onSizeChanged(false);
            }
            delete ptr;
        }, true, true);
    };

    std::shared_ptr<DataQueReaderT> reader(new DataQueReader<T>(use_cache ? _storage : nullptr), on_dealloc);
    _reader_map[reader.get()] = reader;
    ++_reader_size;
    onSizeChanged(true);
    return reader;
}

template <typename T>
void DataQueReaderDispatcher<T>::onSizeChanged(bool add_flag) 
{ 
    _on_size_changed(_reader_size, add_flag); 
}

template <typename T>
void DataQueReaderDispatcher<T>::clearCache() 
{
    if (_reader_size == 0) {
        _storage->clearCache();
    }
}

template <typename T>
std::list<ClientInfo> DataQueReaderDispatcher<T>::getInfoList(const onChangeInfoCB &on_change) 
{
    std::list<ClientInfo> ret;
    for (auto &pr : _reader_map) {
        auto reader = pr.second.lock();
        if (!reader) {
            continue;
        }
        auto info = reader->getInfo();
        if (info.ip_.empty()) {
            continue;
        }
        ret.emplace_back(on_change(std::move(info)));
    }
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////

template <typename T>
DataQue<T>::DataQue(size_t max_size, onReaderChanged cb, size_t max_gop_size) 
{
    _storage = std::make_shared<DataQueStorage<T>>(max_size, max_gop_size);
    _on_reader_changed = cb ? std::move(cb) : [](int size) {};
    //先触发无人观看
    _on_reader_changed(0);
}

template <typename T>
DataQue<T>::~DataQue() 
{
    logTrace << "DataQue<T>::~DataQue() ";
}

template <typename T>
void DataQue<T>::write(T in, bool is_key) 
{
    // logInfo << "write =================: " << this;
    int i = 0;
    for (auto& onWrite : _on_write_map) {
        onWrite.second(in, is_key);
    }

    LOCK_GUARD(_mtx_map);
        // logInfo << "_dispatcher_map =================: " << _dispatcher_map.size();
    for (auto &pr : _dispatcher_map) {
        // logInfo << "_dispatcher_map =================: " << pr.second;
        auto &second = pr.second;
        //切换线程后触发onRead事件
        pr.first->async([second, in, is_key]() { 
            second->write(const_cast<T &>(in), is_key); 
        }, true, false);
    }
    _storage->write(std::move(in), is_key);
}

template <typename T>
void DataQue<T>::sendMessage(const ClientInfo &data) 
{
    LOCK_GUARD(_mtx_map);
    for (auto &pr : _dispatcher_map) {
        auto &second = pr.second;
        // 切换线程后触发sendMessage
        pr.first->async([second, data]() { second->sendMessage(data); }, true, false);
    }
}

template <typename T>
void DataQue<T>::addOnWrite(void* key, onWriteFunc onWrite) 
{
    if (_on_write_map.find(key) != _on_write_map.end()) {
        return ;
    }
    _on_write_map.emplace(key, onWrite);

    // if (onWrite)
        onSizeChanged(nullptr, 1, true);
    // else
    //     onSizeChanged(nullptr, 1, false);

    if (!_storage || !onWrite) {
        return;
    }
    // _storage->getCache().for_each([this](const list<std::pair<bool, T>> &lst) {
    //     lst.for_each([this](const std::pair<bool, T> &pr) { _on_write(pr.second, pr.first); });
    // });
    auto cache = _storage->getCache();
    for (auto& list : cache) {
        for (auto& pr : list) {
            onWrite(pr.second, pr.first);
        }
    }
}

template <typename T>
int DataQue<T>::getOnWriteSize()
{
    return _on_write_map.size();
} 

template <typename T>
void DataQue<T>::delOnWrite(void* key)
{
    _on_write_map.erase(key);
    onSizeChanged(nullptr, 1, false);
} 

template <typename T>
void DataQue<T>::addBytes(int bytes)
{
    _total_bytes += bytes;
}

template <typename T>
uint64_t DataQue<T>::getBytes()
{
    return _total_bytes;
}

template <typename T>
std::shared_ptr<DataQueReader<T>> DataQue<T>::attach(const EventLoop::Ptr &loop, bool use_cache) 
{
    typename DataQueReaderDispatcher<T>::Ptr dispatcher;
    {
        LOCK_GUARD(_mtx_map);
        auto &ref = _dispatcher_map[loop];
        if (!ref) {
            std::weak_ptr<DataQue<T>> weak_self = this->shared_from_this();
            auto onSizeChanged = [weak_self, loop](int size, bool add_flag) {
                if (auto strong_self = weak_self.lock()) {
                    strong_self->onSizeChanged(loop, size, add_flag);
                }
            };
            auto onDealloc = [loop](DataQueReaderDispatcher<T> *ptr) { loop->async([ptr]() { delete ptr; }, true, true); };
            ref.reset(new DataQueReaderDispatcher<T>(_storage->clone(), std::move(onSizeChanged)), std::move(onDealloc));
        }
        dispatcher = ref;
    }

    return dispatcher->attach(loop, use_cache);
}

template <typename T>
int DataQue<T>::readerCount()
{ 
    return _total_count; 
}

template <typename T>
void DataQue<T>::clearCache() 
{
    LOCK_GUARD(_mtx_map);
    _storage->clearCache();
    for (auto &pr : _dispatcher_map) {
        auto &second = pr.second;
        //切换线程后清空缓存
        pr.first->async([second]() { second->clearCache(); }, false);
    }
}

template <typename T>
void DataQue<T>::getInfoList(const onGetInfoCB &cb, const typename DataQueReaderDispatcherT::onChangeInfoCB &on_change) 
{
    if (!cb) {
        return;
    }
    
    if (!on_change) {
        const_cast<typename DataQueReaderDispatcher<T>::onChangeInfoCB &>(on_change) = [](ClientInfo &&info) { return std::move(info); };
    }

    LOCK_GUARD(_mtx_map);

    auto info_vec = std::make_shared<std::vector<std::list<ClientInfo>>>();
    // 1、最少确保一个元素
    info_vec->resize(_dispatcher_map.empty() ? 1 : _dispatcher_map.size());
    std::shared_ptr<void> on_finished(nullptr, [cb, info_vec](void *) mutable {
        // 2、防止这里为空
        auto &lst = *info_vec->begin();
        for (auto &item : *info_vec) {
            if (&lst != &item) {
                lst.insert(lst.end(), item.begin(), item.end());
            }
        }
        cb(lst);
    });

    auto i = 0U;
    for (auto &pr : _dispatcher_map) {
        auto &second = pr.second;
        pr.first->async([second, info_vec, on_finished, i, on_change]() { 
            (*info_vec)[i] = second->getInfoList(on_change); 
        }, true);
        ++i;
    }
}

template <typename T>
void DataQue<T>::onSizeChanged(const EventLoop::Ptr &loop, int size, bool add_flag) 
{
    if (size == 0) {
        LOCK_GUARD(_mtx_map);
        _dispatcher_map.erase(loop);
    }

    if (add_flag) {
        ++_total_count;
    } else {
        --_total_count;
    }
    logTrace<< "_total_count: " << _total_count << this;
    _on_reader_changed(_total_count);
}


#endif // Common_SourceDataQue_H_
