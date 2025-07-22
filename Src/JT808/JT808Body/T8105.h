#ifndef T8105_H
#define T8105_H

#include <string>

class T8105 {
private:
    // 命令字常量
    static constexpr int WirelessUpgrade = 1;
    static constexpr int ConnectToServer = 2;
    static constexpr int TerminalShutdown = 3;
    static constexpr int TerminalReset = 4;
    static constexpr int RestoreFactorySettings = 5;
    static constexpr int CloseDataCommunication = 6;
    static constexpr int CloseAllWirelessCommunication = 7;

    int command;  // 命令字，占用 1 字节
    std::string parameter;  // 命令参数，长度可变

public:
    T8105();
    ~T8105();

    // 获取静态常量值
    static int getWirelessUpgrade();
    static int getConnectToServer();
    static int getTerminalShutdown();
    static int getTerminalReset();
    static int getRestoreFactorySettings();
    static int getCloseDataCommunication();
    static int getCloseAllWirelessCommunication();

    // Getter 方法
    int getCommand() const;
    const std::string& getParameter() const;

    // Setter 方法，支持链式调用
    T8105& setCommand(int newCommand);
    T8105& setParameter(const std::string& newParameter);
};

#endif // T8105_H
