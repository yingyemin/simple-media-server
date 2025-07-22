#include "T8804.h"

// 默认构造函数
T8804::T8804() : command(0), time(0), save(0), audioSamplingRate(0) {}

// Getter 方法实现
int T8804::getCommand() const {
    return command;
}

int T8804::getTime() const {
    return time;
}

int T8804::getSave() const {
    return save;
}

int T8804::getAudioSamplingRate() const {
    return audioSamplingRate;
}

// Setter 方法实现，支持链式调用
T8804& T8804::setCommand(int newCommand) {
    command = newCommand;
    return *this;
}

T8804& T8804::setTime(int newTime) {
    time = newTime;
    return *this;
}

T8804& T8804::setSave(int newSave) {
    save = newSave;
    return *this;
}

T8804& T8804::setAudioSamplingRate(int newAudioSamplingRate) {
    audioSamplingRate = newAudioSamplingRate;
    return *this;
}
