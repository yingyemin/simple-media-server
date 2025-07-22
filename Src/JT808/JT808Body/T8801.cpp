#include "T8801.h"

// 默认构造函数
T8801::T8801()
    : channelId(0), command(0), time(0), save(0), resolution(0),
      quality(0), brightness(0), contrast(0), saturation(0), chroma(0) {}

// Getter 方法实现
int T8801::getChannelId() const {
    return channelId;
}

int T8801::getCommand() const {
    return command;
}

int T8801::getTime() const {
    return time;
}

int T8801::getSave() const {
    return save;
}

int T8801::getResolution() const {
    return resolution;
}

int T8801::getQuality() const {
    return quality;
}

int T8801::getBrightness() const {
    return brightness;
}

int T8801::getContrast() const {
    return contrast;
}

int T8801::getSaturation() const {
    return saturation;
}

int T8801::getChroma() const {
    return chroma;
}

// Setter 方法实现，支持链式调用
T8801& T8801::setChannelId(int newChannelId) {
    channelId = newChannelId;
    return *this;
}

T8801& T8801::setCommand(int newCommand) {
    command = newCommand;
    return *this;
}

T8801& T8801::setTime(int newTime) {
    time = newTime;
    return *this;
}

T8801& T8801::setSave(int newSave) {
    save = newSave;
    return *this;
}

T8801& T8801::setResolution(int newResolution) {
    resolution = newResolution;
    return *this;
}

T8801& T8801::setQuality(int newQuality) {
    quality = newQuality;
    return *this;
}

T8801& T8801::setBrightness(int newBrightness) {
    brightness = newBrightness;
    return *this;
}

T8801& T8801::setContrast(int newContrast) {
    contrast = newContrast;
    return *this;
}

T8801& T8801::setSaturation(int newSaturation) {
    saturation = newSaturation;
    return *this;
}

T8801& T8801::setChroma(int newChroma) {
    chroma = newChroma;
    return *this;
}
