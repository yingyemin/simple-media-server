#ifndef Record_H
#define Record_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <mutex>


using namespace std;

class Record : public enable_shared_from_this<Record>
{
public:
    using Ptr = shared_ptr<Record>;

    Record();
    ~Record();

public:
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void setOnClose(const function<void()>& cb) = 0;

    static void addRecord(const string& uri, const string& taskId, const Record::Ptr& record);
    static void delRecord(const string& uri, const string& taskId);
    static Record::Ptr getRecord(const string& uri, const string& taskId);
    static void for_each_record(const function<void(const Record::Ptr& record)>& func);

private:
    static mutex _mtx;
    static unordered_map<string/*uri*/, unordered_map<string/*taskid*/, Record::Ptr>> _mapRecordWriter;
};


#endif //Record_H
