#ifndef JT808Packet_H
#define JT808Packet_H

#include "JT808Header.h"
#include "Net/Buffer.h"

enum JT808_VERSION {
    JT808_VERSION_2011 = 0,
    JT808_VERSION_2013,
    JT808_VERSION_2019,
};

// 转义字符定义
const uint8_t ESCAPE_CHAR = 0x7D;
const uint8_t ESCAPE_XOR_1 = 0x01;
const uint8_t ESCAPE_XOR_2 = 0x02;
// 消息起始和结束标识
const uint8_t MESSAGE_START_END_FLAG = 0x7E;

const size_t  MESSAGE_MAX_LENGTH     = 1200;
const size_t  HEADER_MIN_LENGTH     = 17;
const size_t  HEADER_MAX_LENGTH     = 21;
const size_t  MESSAGE_MIN_LENGTH    = 20;
const size_t  REGISTER_MIN_LENGTH   = 80;   //最短注册消息长度
// 消息体长度掩码
const uint16_t LENGTH_MASK = 0x03FF;  // 0b0000001111111111，用于提取消息体长度

//消息ID
enum CmdID {
    // 终端通用应答
    TerminalReply = 0x0001,
    // 平台通用应答
    PlatformReply = 0x8001,
    // 心跳
    Heartbeat = 0x0002,
    // 服务器补包请求
    ServerResendReq = 0x8003,
    // 终端补包请求
    TerminalResendReq = 0x0005,
    // 注册
    Register = 0x0100,
    // 注册应答
    RegisterResp = 0x8100,
    // 注销
    Logout = 0x0003,
    // 查询时间
    QueryTime = 0x0004,
    // 时间应答
    QueryTimeResp = 0x8004,
    // 鉴权
    Authenticate = 0x0102,
    // 设置参数
    SetParam = 0x8103,
    // 查询参数
    QueryParam = 0x8104,
    // 参数应答
    ParamResp = 0x0104,
    // 控制指令
    CtrlCmd = 0x8105,
    // 查询特定参数
    QuerySpecParam = 0x8106,
    // 查询属性
    QueryAttr = 0x8107,
    // 属性应答
    AttrResp = 0x0107,
    // 升级包下发
    UpgradePkg = 0x8108,
    // 升级结果
    UpgradeResult = 0x0108,
    // 位置汇报
    LocationReport = 0x0200,
    // 位置查询
    LocationQuery = 0x8201,
    // 位置应答
    LocationResp = 0x0201,
    // 位置追踪
    TrackControl = 0x8202,
    // 事件指令
    EventCmd = 0x8301,
    // 事件报告
    EventReport = 0x0301,
    // 提问指令
    QueryCmd = 0x8302,
    // 提问应答
    QueryResp = 0x0302,
    // 点播菜单
    InfotainmentMenu = 0x8303,
    // 点播控制
    InfotainmentCtrl = 0x0303,
    // 信息服务
    InfoService = 0x8304,
    // 电话回拨
    Callback = 0x8400,
    // 电话本设置
    PhonebookSet = 0x8401,
    // 车辆控制
    VehicleCtrl = 0x8500,
    // 车辆应答
    VehicleResp = 0x0500,
    // 区域设置
    AreaSet = 0x8600,
    // 区域删除
    AreaDelete = 0x8601,
    // 矩形设置
    RectSet = 0x8602,
    // 矩形删除
    RectDelete = 0x8603,
    // 多边形设置
    PolygonSet = 0x8604,
    // 多边形删除
    PolygonDelete = 0x8605,
    // 路线设置
    RouteSet = 0x8606,
    // 路线删除
    RouteDelete = 0x8607,
    // 数据采集命令
    DataCollectCmd = 0x8700,
    // 数据上传
    DataUpload = 0x0700,
    // 参数下传
    ParamDownCmd = 0x8701,
    // 确认报警
    AlarmConfirm = 0x8203,
    // 链路检测
    LinkTest = 0x8204,
    // 文本下发
    TextMsg = 0x8300,
    // 驾驶员信息
    DriverInfoReport = 0x0702,
    // 驾驶员请求
    DriverInfoReq = 0x8702,
    // 批量位置上传
    BatchLocUpload = 0x0704,
    // CAN总线数据
    CANData = 0x0705,
    // 多媒体事件
    MediaEvent = 0x0800,
    // 多媒体上传
    MediaUpload = 0x0801,
    // 多媒体应答
    MediaResp = 0x8800,
    // 拍摄命令
    SnapCmd = 0x8801,
    // 拍摄应答
    SnapResp = 0x0805,
    // 多媒体检索
    MediaSearch = 0x8802,
    // 检索应答
    SearchResp = 0x0802,
    // 查询区域
    QueryArea = 0x8608,
    // 区域应答
    AreaResp = 0x0608,
    // 运单上报
    WaybillReport = 0x0701,
    // 多媒体上传(重复)
    MediaUploadDup = 0x8803,
    // 录音开始
    RecStart = 0x8804,
    // 单条检索上传
    SingleMediaSearch = 0x8805,
    // 透传下行
    TransparentDown = 0x8900,
    // 透传上行
    TransparentUp = 0x0900,
    // 数据压缩
    DataCompression = 0x0901,
    // RSA公钥-平台
    RSAKeyPlatform = 0x8A00,
    // RSA公钥-终端
    RSAKeyTerminal = 0x0A00,
    // 查询终端音视频属性
    QueryTerminalMediaAttr = 0x9003,
    // 查询上传音视频属性
    QueryUploadMediaAttr = 0x1003,
    // 终端上传乘客流量
    TerminalUploadPassengerFlow = 0x1005,
    // 实时音视频传输请求
    LiveVideoTransferReq = 0x9101,
    // 音视频实时传输控制
    LiveVideoTransferCtrl = 0x9102,
    // 实时音视频传输状态通知
    LiveVideoTransferStatus = 0x9105,
    // 查询资源列表
    QueryResourceList = 0x9205,
    // 终端上传音视频资源列表
    TerminalUploadMediaResourceList = 0x1205,
    // 平台下发远程录像回放请求
    RecordVideoReq = 0x9201,
    // 回放控制
    RecordVideoCtrl = 0x9202,
    // 文件上传
    FileUpload = 0x9206,
    // 文件上传完成通知
    FileUploadComplete = 0x1206,
    // 文件上传控制
    FileUploadCtrl = 0x9207,
    // 云台旋转
    PTZCtrl = 0x9301,
    // 云台调整焦距控制
    PTZAdjustFocusCtrl = 0x9302,
    // 云台调整光圈控制
    PTZAdjustApertureCtrl = 0x9303,
    // 云台雨刷控制
    PTZAdjustRainCtrl = 0x9304,
    // 红外补光控制
    PTZAdjustIRCtrl = 0x9305,
    // 云台变倍控制
    PTZAdjustZoomCtrl = 0x9306,
};

class JT808Packet {
public:
    using Ptr = std::shared_ptr<JT808Packet>;
    
    JT808Packet();
    ~JT808Packet();

    static StringBuffer::Ptr pack(JT808Header& header, const StringBuffer::Ptr& body);

    static size_t unescape(const unsigned char* data, size_t length,unsigned char* unescapedData);
    static StringBuffer::Ptr escape(const unsigned char* data, size_t length);
    static unsigned char calcCheckSum(const unsigned char* buffer, size_t bufferSize);
    static std::string decodeBCDPhoneNumber(const std::string& bcdPhone,bool removePreZero = false);

public:
    JT808_VERSION _version;
    JT808Header _header;
    StringBuffer _content;
};

#endif //JT808Packet_H
