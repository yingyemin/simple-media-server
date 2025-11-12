#ifndef Address_h
#define Address_h

#include <memory>
#include <string>
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cstdint>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

class Address : public std::enable_shared_from_this<Address>
{
public:
    using Ptr = std::shared_ptr<Address>;

    Address();
    ~Address();

public:
    void setFamily(uint8_t family);
    void setPort(uint16_t port);
    std::string toString();
    int fromString(const std::string& ip);
    bool equal(const Address::Ptr& addr);

    sockaddr_in* getAddr4() {return &_sin;}
    sockaddr_in6* getAddr6() {return &_sin6;}
    uint16_t getPort() {return _port;}

    static bool equal(const Address::Ptr& addrA, const Address::Ptr& addrB);
    static bool inet6Valid(const std::string& ip, sockaddr_in6& sin6);
    static bool inetValid(const std::string& ip, sockaddr_in& sin);

private:
    uint8_t _family;
    struct sockaddr_in _sin;
    struct sockaddr_in6 _sin6;
    uint16_t _port;
};

#endif //DnsCache_h