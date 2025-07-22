#include "T8103.h"
#include <type_traits>

// 心跳与超时设置
const int HEARTBEAT_INTERVAL = 0x0001; // 终端心跳发送间隔（秒）
const int TCP_RESPONSE_TIMEOUT = 0x0002; // TCP消息应答超时时间（秒）
const int UDP_RESPONSE_TIMEOUT = 0x0004; // UDP消息应答超时时间（秒）
const int SMS_RESPONSE_TIMEOUT = 0x0006; // SMS消息应答超时时间（秒）

// 重传次数
const int TCP_RETRANSMIT_COUNT = 0x0003; // TCP消息重传次数
const int UDP_RETRANSMIT_COUNT = 0x0005; // UDP消息重传次数
const int SMS_RETRANSMIT_COUNT = 0x0007; // SMS消息重传次数

// 中间的保留

// 服务器配置， 值为string
const int MAIN_SERVER_APN = 0x0010; // 主服务器APN
const int MAIN_SERVER_USERNAME = 0x0011; // 主服务器用户名
const int MAIN_SERVER_PASSWORD = 0x0012; // 主服务器密码
const int MAIN_SERVER_ADDR = 0x0013; // 主服务器地址
const int BACKUP_SERVER_APN = 0x0014; // 备份服务器APN
const int BACKUP_SERVER_USERNAME = 0x0015; // 备份服务器用户名
const int BACKUP_SERVER_PASSWORD = 0x0016; // 备份服务器密码
const int BACKUP_SERVER_ADDR = 0x0017; // 备份服务器地址

// 端口设置
const int TCP_PORT = 0x0018; // 服务器TCP端口
const int UDP_PORT = 0x0019; // 服务器UDP端口

// 中间的保留

// 位置汇报策略
const int LOCATION_REPORTING_STRATEGY = 0x0020; // 位置汇报策略
// 位置汇报相关
const uint32_t PositionReportMode = 0x0021;           // 位置汇报方案
const uint32_t DriverUnloginReportInterval = 0x0022;  // 驾驶员未登录汇报时间间隔

// 中间保留

const uint32_t SleepReportInterval = 0x0027;          // 休眠时汇报时间间隔
const uint32_t EmergencyAlarmReportInterval = 0x0028; // 紧急报警时汇报时间间隔
const uint32_t DefaultReportInterval = 0x0029;        // 缺省时间汇报间隔

// 中间保留

// 距离汇报相关
const uint32_t DefaultDistanceInterval = 0x002C;      // 缺省距离汇报间隔
const uint32_t DriverUnloginDistanceInterval = 0x002D;// 驾驶员未登录汇报距离间隔
const uint32_t SleepDistanceInterval = 0x002E;        // 休眠时汇报距离间隔
const uint32_t EmergencyAlarmDistanceInterval = 0x002F;// 紧急报警时汇报距离间隔

// 其他设置
const uint32_t MissingAngle = 0x0030;                 // 拐点补传角度

// 中间保留

// 电话号码，值为string
const int MonitoringPlatformPhone = 0x0040; // 监控平台电话号码
const int RecoveryFactoryPhone = 0x0042;    // 恢复出厂设置电话号码
const int MonitoringPlatformSmsPhone = 0x0043;// 监控平台SMS电话号码
const int SmsTextAlarmPhone = 0x0044;      // 接收终端SMS文本报警号码
const int MonitoringSmsPhone = 0x0048;      // 监听电话号码（此处与模型答案有出入，根据信息调整）
const int PrivilegeSmsPhone = 0x0049;       // 监管平台特权短信号码

// 通话设置
const uint32_t TerminalAnswerStrategy = 0x0045;       // 终端电话接听策略
const uint32_t MaxCallDuration = 0x0046;             // 每次最长通话时间
const uint32_t MaxMonthlyCallDuration = 0x0047;      // 当月最长通话时间

// 报警设置
const uint32_t AlarmHideFlag = 0x0050;                // 报警屏蔽字
const uint32_t AlarmSmsSwitch = 0x0051;              // 报警发送文本SMS开关
const uint32_t AlarmCameraSwitch = 0x0052;           // 报警拍摄开关
const uint32_t AlarmStorageFlag = 0x0053;            // 报警拍摄存储标志
const uint32_t KeyAlarmFlag = 0x0054;                // 关键报警标志

// 车辆控制
const uint32_t MaxSpeedLimit = 0x0055;               // 最高速度
const uint32_t OverSpeedDuration = 0x0056;           // 超速持续时间
// 驾驶时间相关
const uint32_t CONTINUOUS_DRIVE_LIMIT = 0x0057;  // 连续驾驶时间门限(分钟)
const uint32_t DAILY_DRIVE_LIMIT = 0x0058;      // 当天累计驾驶时间门限(分钟) 
const uint32_t MIN_BREAK_TIME = 0x0059;         // 最小休息时间(分钟)
const uint32_t MAX_PARKING_TIME = 0x005A;       // 最长停车时间(分钟)

// 中间保留

// 视频参数
const uint32_t IMAGE_QUALITY = 0x0070;    // 图像质量(1-10)
const uint32_t BRIGHTNESS = 0x0071;       // 亮度(0-255)
const uint32_t CONTRAST = 0x0072;         // 对比度(0-127) 
const uint32_t SATURATION = 0x0073;       // 饱和度(0-127)
const uint32_t COLOR = 0x0074;            // 色度(0-255)

// 中间保留

// 车辆信息
const uint32_t ODOMETER = 0x0080;        // 里程表读数(公里)
const uint32_t PROVINCE_ID = 0x0081;     // 省域ID
const uint32_t CITY_ID = 0x0082;         // 市域ID
const uint32_t VEHICLE_LICENSE = 0x0083;  // 车牌号
const uint32_t VEHICLE_COLOR = 0x0084;    // 车牌颜色(0-9)

// 默认构造函数
T8103::T8103() = default;

// 析构函数
T8103::~T8103() = default;

