#include "Address.h"
#include "Log/Logger.h"

#include <cstring>

Address::Address()
{
    _family = AF_INET;
}

Address::~Address()
{
}

void Address::setFamily(uint8_t family)
{
    _family = family;
}

void Address::setPort(uint16_t port)
{
    _port = port;
    switch (_family)
    {
    case AF_INET6:
        _sin6.sin6_port = htons(port);
        break;
    case AF_INET:
        _sin.sin_port = htons(port);
        break;
    default:
        break;
    }
}

std::string Address::toString()
{
    std::string ip;
    switch (_family)
    {
    case AF_INET6:
        ip.resize(INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &_sin6.sin6_addr, (char*)ip.c_str(), INET6_ADDRSTRLEN);
        break;
    case AF_INET:
        ip.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &_sin.sin_addr, (char*)ip.c_str(), INET_ADDRSTRLEN);
        break;
    default:
        break;
    }
    return ip;
}

int Address::fromString(const std::string& ip)
{
    if (inet6Valid(ip, _sin6))
    {
        _family = AF_INET6;
        // inet_pton(AF_INET6, ip.c_str(), &_sin6.sin6_addr);
    }
    else if (inetValid(ip, _sin))
    {
        _family = AF_INET;
        // inet_pton(AF_INET, ip.c_str(), &_sin.sin_addr);
    }
    else
    {
        return -1;
    }
    return 0;
}

bool Address::equal(const Address::Ptr& addr)
{
    if (_family != addr->_family)
    {
        return false;
    }
    switch (_family)
    {
    case AF_INET6:
        return memcmp(&_sin6, &addr->_sin6, sizeof(_sin6)) == 0;
    case AF_INET:
        return memcmp(&_sin, &addr->_sin, sizeof(_sin)) == 0;
    default:
        break;
    }
    return false;
}

bool Address::equal(const Address::Ptr& addrA, const Address::Ptr& addrB)
{
    return addrA->equal(addrB);
}

bool Address::inet6Valid(const std::string& ip, sockaddr_in6& sin6)
{
    return inet_pton(AF_INET6, ip.c_str(), &sin6.sin6_addr) == 1;
}

bool Address::inetValid(const std::string& ip, sockaddr_in& sin)
{
    return inet_pton(AF_INET, ip.c_str(), &sin.sin_addr) == 1;
}