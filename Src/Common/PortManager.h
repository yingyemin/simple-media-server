#ifndef PortManager_H
#define PortManager_H

#include <set>
#include <string>
#include <memory>
#include <mutex>

using namespace std;

class PortInfo
{
public:
    int maxPort_;
    int minPort_;
    set<int> portFreeList_;
    set<int> portUseList_;
};

class PortManager : public enable_shared_from_this<PortManager>
{
public:
    using Ptr = shared_ptr<PortManager>;

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
    mutex _mtx;
    set<int> _portUseList;
    set<int> _portFreeList;
};


#endif //PortManager_H
