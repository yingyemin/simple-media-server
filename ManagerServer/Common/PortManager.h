#ifndef PortManager_H
#define PortManager_H

#include <set>
#include <string>
#include <memory>
#include <mutex>

// using namespace std;

class PortInfo
{
public:
    int maxPort_;
    int minPort_;
    std::set<int> portFreeList_;
    std::set<int> portUseList_;
};

class PortManager : public std::enable_shared_from_this<PortManager>
{
public:
    using Ptr = std::shared_ptr<PortManager>;

    PortManager();
    ~PortManager() {}

public:
    void init(int minPort, int maxPort);
    int get();
    void put(int port);
    PortInfo getPortInfo();

private:
    int _minPort = 0;
    int _maxPort = 0;
    std::mutex _mtx;
    std::set<int> _portUseList;
    std::set<int> _portFreeList;
};


#endif //PortManager_H
