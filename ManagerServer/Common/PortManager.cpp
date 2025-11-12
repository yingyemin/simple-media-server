#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "PortManager.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

PortManager::PortManager()
{

}

void PortManager::init(int minPort, int maxPort)
{
    _minPort = minPort;
    _maxPort = maxPort;

    maxPort += 1;
    for (int i = minPort; i < maxPort; ++i) {
        _portFreeList.emplace(i);
    }
}

int PortManager::get()
{
    lock_guard<mutex> lck(_mtx);
    auto iter = _portFreeList.begin();
    if (iter == _portFreeList.end()) {
        return -1;
    }

    int port = *iter;
    _portFreeList.erase(iter);
    _portUseList.emplace(port);

    return port;
}

void PortManager::put(int port)
{
    lock_guard<mutex> lck(_mtx);
    _portUseList.erase(port);
    _portFreeList.emplace(port);
}

PortInfo PortManager::getPortInfo()
{
    lock_guard<mutex> lck(_mtx);
    PortInfo info;
    info.maxPort_ = _maxPort;
    info.minPort_ = _minPort;
    info.portFreeList_ = _portFreeList;
    info.portUseList_ = _portUseList;

    return info;
}
