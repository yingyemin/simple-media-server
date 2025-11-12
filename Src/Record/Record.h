#ifndef Record_H
#define Record_H

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <mutex>

#include "Common/UrlParser.h"
#include "Common/HookManager.h"


// using namespace std;

class RecordTemplate
{
public:
    using Ptr = std::shared_ptr<RecordTemplate>;

    int segment_duration = 600000; //单位毫秒，切片时长
    int segment_count = 0; //限制切片数量
    int duration = 0; //单位毫秒，限制录制时长
};

class Record : public std::enable_shared_from_this<Record>
{
public:
    using Ptr = std::shared_ptr<Record>;

    Record();
    ~Record();

public:
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void setOnClose(const std::function<void()>& cb) = 0;
    virtual std::string getFormat() = 0;
    virtual RecordTemplate::Ptr getTemplate() {return _template;}
    virtual std::string getStatus() {return "recording";}
    virtual uint64_t getCreateTime() {return _createTime;}

    virtual void setTaskId(const std::string& taskId)
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

    static void addRecord(const std::string& uri, const std::string& taskId, const Record::Ptr& record);
    static void delRecord(const std::string& uri, const std::string& taskId);
    static Record::Ptr getRecord(const std::string& uri, const std::string& taskId);
    static void for_each_record(const std::function<void(const Record::Ptr& record)>& func);

protected:
    uint64_t _createTime;
    std::string _taskId;
    UrlParser _urlParser;
    OnRecordInfo _recordInfo;
    RecordTemplate::Ptr _template;

private:
    static std::mutex _mtx;
    static std::unordered_map<std::string/*uri*/, std::unordered_map<std::string/*taskid*/, Record::Ptr>> _mapRecordWriter;
};


#endif //Record_H
