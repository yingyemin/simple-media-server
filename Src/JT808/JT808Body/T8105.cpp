#include "T8105.h"

// 默认构造函数
T8105::T8105() : command(0), parameter("") {}

// 析构函数
T8105::~T8105() = default;

// 获取静态常量值的方法
int T8105::getWirelessUpgrade() {
    return WirelessUpgrade;
}

int T8105::getConnectToServer() {
    return ConnectToServer;
}

int T8105::getTerminalShutdown() {
    return TerminalShutdown;
}

int T8105::getTerminalReset() {
    return TerminalReset;
}

int T8105::getRestoreFactorySettings() {
    return RestoreFactorySettings;
}

int T8105::getCloseDataCommunication() {
    return CloseDataCommunication;
}

int T8105::getCloseAllWirelessCommunication() {
    return CloseAllWirelessCommunication;
}

// Getter 方法实现
int T8105::getCommand() const {
    return command;
}

const std::string& T8105::getParameter() const {
    return parameter;
}

// Setter 方法实现，支持链式调用
T8105& T8105::setCommand(int newCommand) {
    command = newCommand;
    return *this;
}

T8105& T8105::setParameter(const std::string& newParameter) {
    parameter = newParameter;
    return *this;
}
