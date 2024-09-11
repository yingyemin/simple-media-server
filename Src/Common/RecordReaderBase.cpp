#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderBase.h"
#include "Logger.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"

using namespace std;

function<RecordReaderBase::Ptr(const string& path)> RecordReaderBase::_createRecordReader;

void RecordReaderBase::registerCreateFunc(const function<RecordReaderBase::Ptr(const string& path)>& func)
{
    _createRecordReader = func;
}

RecordReaderBase::Ptr RecordReaderBase::createRecordReader(const string& path)
{
    if (_createRecordReader) {
        return _createRecordReader(path);
    }

    return nullptr;
}