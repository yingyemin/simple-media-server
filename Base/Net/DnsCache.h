#ifndef DnsCache_h
#define DnsCache_h

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

using namespace std;

class DnsItem {
public:
    std::shared_ptr<struct addrinfo> addr_info;
    time_t create_time;
};

class DnsCache {
public:
    static DnsCache &instance();

    bool getDomainIP(const char *host, uint16_t port, struct sockaddr_storage &addr,
                           int ai_family, int ai_socktype, int ai_protocol, int expire_sec = 60);

    bool getDomainIP(const char *host, sockaddr_storage &storage, int ai_family = AF_INET,
                     int ai_socktype = SOCK_STREAM, int ai_protocol = IPPROTO_TCP, int expire_sec = 60);

private:
    std::shared_ptr<struct addrinfo> getCacheDomainIP(const char *host, int expireSec);

    void setCacheDomainIP(const char *host, std::shared_ptr<struct addrinfo> addr);

    std::shared_ptr<struct addrinfo> getSystemDomainIP(const char *host);

    struct addrinfo *getPerferredAddress(struct addrinfo *answer, int ai_family, int ai_socktype, int ai_protocol);

private:
    mutex _mtx;
    unordered_map<string, DnsItem> _dns_cache;
};

#endif //DnsCache_h