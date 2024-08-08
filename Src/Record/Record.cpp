#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Record.h"

using namespace std;

mutex Record::_mtx;
unordered_map<string/*uri*/, unordered_map<string/*taskid*/, Record::Ptr>> Record::_mapRecordWriter;

Record::Record()
{

}

Record::~Record()
{
}

void Record::addRecord(const string& uri, const string& taskId, const Record::Ptr& record)
{
    lock_guard<mutex> lck(_mtx);
    if (_mapRecordWriter[uri].find(taskId) != _mapRecordWriter[uri].end()) {
        return ;
    }

    _mapRecordWriter[uri][taskId] = record;
}

void Record::delRecord(const string& uri, const string& taskId)
{
    lock_guard<mutex> lck(_mtx);
    _mapRecordWriter[uri].erase(taskId);
}

Record::Ptr Record::getRecord(const string& uri, const string& taskId)
{
    lock_guard<mutex> lck(_mtx);
    return _mapRecordWriter[uri][taskId];
}

void Record::for_each_record(const function<void(const Record::Ptr& record)>& func)
{
    unordered_map<string/*uri*/, unordered_map<string/*taskid*/, Record::Ptr>> mapTmp;
    {
        lock_guard<mutex> lck(_mtx);
        mapTmp = _mapRecordWriter;
    }

    for (auto& recordIter : mapTmp) {
        for(auto& task : recordIter.second) {
            func(task.second);
        }
    }
}
