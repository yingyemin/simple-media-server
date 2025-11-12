#include "DnsCache.h"
#include "Log/Logger.h"

#include <cstring>
#include <iostream>

using namespace std;

struct sockaddr_storage makeSockaddr(const char *host, uint16_t port) {
    struct sockaddr_storage storage;
    memset(&storage, 0,sizeof(storage));

    struct in_addr addr;
    struct in6_addr addr6;
    if (1 == inet_pton(AF_INET, host, &addr)) {
        // host是ipv4
        reinterpret_cast<struct sockaddr_in &>(storage).sin_addr = addr;
        reinterpret_cast<struct sockaddr_in &>(storage).sin_family = AF_INET;
        reinterpret_cast<struct sockaddr_in &>(storage).sin_port = htons(port);
        return storage;
    }
    if (1 == inet_pton(AF_INET6, host, &addr6)) {
        // host是ipv6
        reinterpret_cast<struct sockaddr_in6 &>(storage).sin6_addr = addr6;
        reinterpret_cast<struct sockaddr_in6 &>(storage).sin6_family = AF_INET6;
        reinterpret_cast<struct sockaddr_in6 &>(storage).sin6_port = htons(port);
        return storage;
    }
    throw std::invalid_argument(string("Not ip address: ") + host);
}

DnsCache& DnsCache::instance()
{
    static DnsCache instance;
    return instance;
}

bool DnsCache::getDomainIP(const char *host, uint16_t port, struct sockaddr_storage &addr,
                           int ai_family, int ai_socktype, int ai_protocol, int expire_sec) {
    bool flag = getDomainIP(host, addr, ai_family, ai_socktype, ai_protocol, expire_sec);
    if (flag) {
        switch (addr.ss_family ) {
            case AF_INET : ((sockaddr_in *) &addr)->sin_port = htons(port); break;
            case AF_INET6 : ((sockaddr_in6 *) &addr)->sin6_port = htons(port); break;
            default: break;
        }
    }
    return flag;
}

bool DnsCache::getDomainIP(const char *host, sockaddr_storage &storage, int ai_family,
                     int ai_socktype, int ai_protocol, int expire_sec) {
    try {
        storage = makeSockaddr(host, 0);
        return true;
    } catch (...) {
        auto item = getCacheDomainIP(host, expire_sec);
        if (!item) {
            item = getSystemDomainIP(host);
            if (item) {
                setCacheDomainIP(host, item);
            }
        }
        if (item) {
            auto addr = getPerferredAddress(item.get(), ai_family, ai_socktype, ai_protocol);
            memcpy(&storage, addr->ai_addr, addr->ai_addrlen);
        }
        return (bool)item;
    }
}

std::shared_ptr<struct addrinfo> DnsCache::getCacheDomainIP(const char *host, int expireSec) {
    lock_guard<mutex> lck(_mtx);
    auto it = _dns_cache.find(host);
    if (it == _dns_cache.end()) {
        //没有记录
        return nullptr;
    }
    if (it->second.create_time + expireSec < time(nullptr)) {
        //已过期
        _dns_cache.erase(it);
        return nullptr;
    }
    return it->second.addr_info;
}

void DnsCache::setCacheDomainIP(const char *host, std::shared_ptr<struct addrinfo> addr) {
    lock_guard<mutex> lck(_mtx);
    DnsItem item;
    item.addr_info = std::move(addr);
    item.create_time = time(nullptr);
    _dns_cache[host] = std::move(item);
}

std::shared_ptr<struct addrinfo> DnsCache::getSystemDomainIP(const char *host) {
    struct addrinfo *answer = nullptr;
    //阻塞式dns解析，可能被打断
    int ret = -1;
    do {
        ret = getaddrinfo(host, nullptr, nullptr, &answer);
    } while (0/*ret == -1 && get_uv_error(true) == UV_EINTR*/);

    if (!answer) {
        logWarn << "getaddrinfo failed: " << host;
        return nullptr;
    }
    return std::shared_ptr<struct addrinfo>(answer, freeaddrinfo);
}

struct addrinfo * DnsCache::getPerferredAddress(struct addrinfo *answer, int ai_family, int ai_socktype, int ai_protocol) {
    auto ptr = answer;
    while (ptr) {
        if (ptr->ai_family == ai_family && ptr->ai_socktype == ai_socktype && ptr->ai_protocol == ai_protocol) {
            return ptr;
        }
        ptr = ptr->ai_next;
    }
    return answer;
}
