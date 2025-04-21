#ifndef Record_H
#define Record_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <mutex>

#include "Common/UrlParser.h"
#include "Common/HookManager.h"


using namespace std;

class RecordTemplate
{
public:
    using Ptr = shared_ptr<RecordTemplate>;

    int segment_duration = 600000; //单位毫秒，切片时长
    int segment_count = 0; //限制切片数量
    int duration = 0; //单位毫秒，限制录制时长
};

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

    virtual void setTaskId(const string& taskId)
    {
        _taskId = taskId;
    }

    virtual std::string getTaskId()
    {
        return _taskId;
    }

    virtual UrlParser getUrlParser()
    {
        return _urlParser;
    }

    static void addRecord(const string& uri, const string& taskId, const Record::Ptr& record);
    static void delRecord(const string& uri, const string& taskId);
    static Record::Ptr getRecord(const string& uri, const string& taskId);
    static void for_each_record(const function<void(const Record::Ptr& record)>& func);

protected:
    std::string _taskId;
    UrlParser _urlParser;
    OnRecordInfo _recordInfo;

private:
    static mutex _mtx;
    static unordered_map<string/*uri*/, unordered_map<string/*taskid*/, Record::Ptr>> _mapRecordWriter;
};


#endif //Record_H
