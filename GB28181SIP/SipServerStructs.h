#ifndef SipServerStructs_H
#define SipServerStructs_H

#include <string>
#include <vector>
#include <cstdint>

// struct SipServerInfo {
// 	std::string IP;
// 	int Port;
// 	std::string ID;
// 	std::string Realm;
// 	std::string Password;
// 	std::string Nonce;
// 	std::string ExternIP;
// 	int heartBeatTime; // 单位秒
// };

struct MediaServerInfo {
	std::string ServerId;
	std::string IP;
	int Port;
	int RtpPort;
	int PlayWait;
	int RtmpPort;
	int RtspPort;
	int HttpServerPort;
	int JT1078Port;
	std::string Secret;
	bool SinglePortMode = false;
};

struct RecordItem {
	std::string			DeviceID;
	std::string			Name;
	std::string			FilePath;
	std::string			Address;
	std::string			StartTime;
	std::string			EndTime;
	uint8_t				Secrecy;
	std::string			Type;
	uint64_t			FileSize;
};


enum class PresetCommand
{
	SET = 129,
	CALL = 130,
	DEL = 131
};


enum class PtzCommand :int
{
	LEFT,
	RIGHT,
	UP,
	DOWN,
	UPLEFT,
	UPRIGHT,
	DOWNLEFT,
	DOWNRIGHT,
	ZOOMIN,
	ZOOMOUT,
	STOP
};

enum REQUEST_MESSAGE_TYPE
{
	REQUEST_TYPE_UNKNOWN = 0,
	KEEPALIVE,                   // 保活心跳
	QUERY_CATALOG,               //   查询目录
	QUERY_DEVICEINFO,               //   查询设备信息
	DEVICE_CONTROL_PTZ,         // 设备控制-云台
	DEVICE_QUERY_PRESET,        // 设备查询-预置位
	DEVICE_CONTROL_PRESET,       // 设备控制-预置位
	DEVICE_CONTROL_HOMEPOSITION, // 设备控制-看守位

	DEVICE_RECORD_QUERY, // 录像文件查询

	REQUEST_TRANSPARENT_CONTROL,	//透传控制类消息
	REQUEST_CALL_INVITE,            // 点播
	REQUEST_CALL_PLAYBACK,          // 回放
	REQUEST_CALL_LIVE,              // 直播
	REQUEST_CALL_DOWNLOAD,          // 下载
	REQUEST_CALL_BYE,               // 挂断

	REQUEST_CALL_MESSAGE,            //播放控制

	
	REQUEST_TYPE_MAX
};

enum manscdp_cmd_category_e
{
	MANSCDP_CMD_CATEGORY_CONTROL,
	MANSCDP_CMD_CATEGORY_QUERY,
	MANSCDP_CMD_CATEGORY_NOTIFY,
	MANSCDP_CMD_CATEGORY_RESPONSE,

	MANSCDP_CMD_CATEGORY_MAX,
	MANSCDP_CMD_CATEGORY_UNKNOWN = MANSCDP_CMD_CATEGORY_MAX
};

enum manscdp_cmdtype_e
{
	//< Control
	MANSCDP_CONTROL_CMD_DEVICE_CONTROL,      ///<设备控制
	MANSCDP_CONTROL_CMD_DEVICE_CONFIG,       ///<设备配置

	//< Query
	MANSCDP_QUERY_CMD_DEVICE_STATUS,        ///<设备控制
	MANSCDP_QUERY_CMD_CATALOG,              ///<设备目录查询
	MANSCDP_QUERY_CMD_DEVICE_INFO,          ///<设备信息查询
	MANSCDP_QUERY_CMD_RECORD_INFO,          ///<文件目录检索
	MANSCDP_QUERY_CMD_ALARM,                ///<报警查询
	MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD,      ///<设备配置查询
	MANSCDP_QUERY_CMD_PRESET_QUERY,         ///<预置位查询
	MANSCDP_QUERY_CMD_MOBILE_POSITION,      ///<移动设备位置数据查询

	//< Notify
	MANSCDP_NOTIFY_CMD_KEEPALIVE,           ///<设备状态信息报送，保活
	MANSCDP_NOTIFY_CMD_ALARM,               ///<报警通知
	MANSCDP_NOTIFY_CMD_MEDIA_STATUS,        ///<媒体通知
	MANSCDP_NOTIFY_CMD_BROADCASE,           ///<语音广播通知
	MANSCDP_NOTIFY_CMD_MOBILE_POSITION,     ///<移动设备位置通知

	//< Response
	MANSCDP_RESOPNSE_CMD_DEVICE_CONTROL,    ///<设备控制响应
	MANSCDP_RESOPNSE_CMD_DEVICE_CONFIG,     ///<设备配置响应
	MANSCDP_RESOPNSE_CMD_DEVICE_STATUS,     ///<设备状态查询响应
	MANSCDP_RESOPNSE_CMD_DEVICE_CATALOG,    ///<设备目录查询响应


	MANSCDP_CMD_TYPE_MAX,
	MANSCDP_CMD_TYPE_UNKNOWN = MANSCDP_CMD_TYPE_MAX
};

enum manscdp_devicecontrol_subcmd_e
{
	PTZCmd = 1,
	TeleBoot,
	RecordCmd,
	GuardCmd,
	AlarmCmd = 5,
	IFrameCmd,
	DragZoomIn,
	DragZoomOut,
	HomePosition
};

enum manscdp_deviceconfig_subcmd_e
{
	BasicParam = 1,
	SVACEncodeConfig,
	SVACDecodeConfig
};

typedef std::vector<manscdp_devicecontrol_subcmd_e> manscdp_devicecontrol_subcmd_t;
typedef  std::vector<manscdp_deviceconfig_subcmd_e> manscdp_deviceconfig_subcmd_t;
typedef  std::vector< std::string> manscdp_configdownload_subcmd_t;


struct manscdp_msgbody_header_t
{
	manscdp_cmd_category_e              cmd_category;
	manscdp_cmdtype_e                   cmd_type;
	std::string                         sn;
	std::string                         devid;
	manscdp_deviceconfig_subcmd_t       devcfg_subcmd;
	manscdp_devicecontrol_subcmd_t      devctl_subcmd;
	manscdp_configdownload_subcmd_t     cfgdownload_subcmd;
};


///< 云台 镜头变倍
struct ptz_cmd_zoom_t
{
	enum cmd_type_e {
		ZOOM_UNKNOWN = 0,
		ZOOM_OUT,
		ZOOM_IN,
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< 云台垂直方向控制
struct ptz_cmd_tilt_t
{
	enum cmd_type_e {
		TILT_UNKNOWN = 0,
		TILT_UP,
		TILT_DOWN,
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< 云台水平控制方向
struct ptz_cmd_pan_t
{
	enum cmd_type_e {
		PAN_UNKNOWN = 0,
		PAN_LEFT,
		PAN_RIGHT,
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< FI指令 光圈
struct fi_cmd_iris_t
{
	enum cmd_type_e {
		IFIS_UNKNOWN = 0,
		IFIS_SHRINK,            ///<缩小
		IFIS_AMPLIFICATION,     ///<放大
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< FI指令 聚焦 光圈
struct fi_cmd_focus_t
{
	enum cmd_type_e {
		FOCUS_UNKNOWN = 0,
		FOCUS_NEAR,
		FOCUS_FAR,
	};
	cmd_type_e      cmdtype;
	uint8_t         speed;
};

///< 预置位指令
struct preset_cmd_t
{
	enum cmd_type_e {
		PRESET_UNKNOWN = 0,
		PRESET_SET,
		PRESET_CALL,
		PRESET_DELE
	};
	cmd_type_e  cmdtype;
	uint8_t     index;
};

///< 巡航指令
struct patrol_cmd_t
{
	enum cmd_type_e {
		PATROL_UNKNOWN = 0,
		PATROL_ADD,
		PATROL_DELE,
		PATROL_SET_SPEED,
		PATROL_SET_TIME,    ///<设置停留时间
		PATROL_START,       ///<开始巡航
		PATROL_STOP
	};
	cmd_type_e  cmdtype;
	uint8_t     patrol_id;      ///<巡航组号
	uint8_t     preset_id;      ///<预置位号
	uint16_t    value;          ///<数据，速度和停留时间使用
};

///< 自动扫描指令
struct scan_cmd_t
{
	enum cmd_type_e {
		SCAN_UNKNOWN = 0,
		SCAN_START,
		SCAN_STOP,
		SCAN_SET_LEFT_BOADER,
		SCAN_SET_RIGHT_BOADER,
		SCAN_SET_SPEED
	};
	cmd_type_e  cmdtype;
	uint8_t     scan_id;
	uint16_t    speed;      ///< 设置scan速度使用
};

enum control_type_e
{
	CTRL_CMD_UNKNOWN = 0,
	PTZ_TYPE,       ///< PTZ控制
	FI_TYPE,        ///<光圈、聚焦控制
	PRESET_TYPE,    ///<预置位
	PATROL_TYPE,    ///<巡航
	SCAN_TYPE,      ///<扫描
	AUX_TYPE,       ///<辅助开关

	CONTROL_STOP    ///<停止控制
};

struct control_cmd_t
{
	uint8_t             first_byte;     ///< A5H
	uint8_t             version;        ///< 版本号
	uint8_t             check;          ///< 校验位

	control_type_e      ctrltype;

	struct {
		ptz_cmd_pan_t   ptz_pan;
		ptz_cmd_tilt_t  ptz_tilt;
		ptz_cmd_zoom_t  ptz_zoom;
	};

	struct {
		fi_cmd_focus_t  fi_focus;
		fi_cmd_iris_t   fi_iris;
	};

	preset_cmd_t        preset;

	patrol_cmd_t        patrol;

	scan_cmd_t          autoscan;
};

#endif