# MediaServer 管理界面

这是一个基于 Vue 3 + Element Plus 开发的 MediaServer 全功能管理界面，提供了完整的流媒体服务器管理功能。

## 功能特性

### 🎯 核心功能
- **仪表盘**: 实时监控服务器状态、流量统计、协议分布等
- **系统配置**: 服务器基础配置、协议配置、Hook回调、CDN配置
- **流媒体管理**: 流列表查看、客户端管理、流控制
- **协议管理**: RTSP、RTMP、WebRTC、SRT、GB28181、JT1078 协议管理
- **录制点播**: 录制任务管理、点播控制
- **系统监控**: 服务器状态、事件循环、性能统计

### 🛠 技术栈
- **前端框架**: Vue 3 + Composition API
- **UI组件库**: Element Plus
- **状态管理**: Pinia
- **路由管理**: Vue Router 4
- **图表库**: ECharts + Vue-ECharts
- **HTTP客户端**: Axios
- **构建工具**: Vite

## 快速开始

### 环境要求
- Node.js >= 16.0.0
- npm >= 8.0.0

### 安装依赖
```bash
cd web-admin
npm install
```

### 开发模式
```bash
npm run dev
```
访问 http://localhost:3000

### 生产构建
```bash
npm run build
```

### 预览构建结果
```bash
npm run preview
```

## 项目结构

```
web-admin/
├── public/                 # 静态资源
├── src/
│   ├── api/               # API接口封装
│   │   └── index.js       # 统一API接口
│   ├── components/        # 公共组件
│   ├── router/            # 路由配置
│   │   └── index.js       # 路由定义
│   ├── views/             # 页面组件
│   │   ├── Dashboard.vue  # 仪表盘
│   │   ├── config/        # 配置管理
│   │   │   ├── ServerConfig.vue      # 服务器配置
│   │   │   ├── ProtocolsConfig.vue   # 协议配置
│   │   │   ├── HookConfig.vue        # Hook配置
│   │   │   └── CdnConfig.vue         # CDN配置
│   │   ├── streams/       # 流媒体管理
│   │   │   ├── StreamsList.vue       # 流列表
│   │   │   └── ClientsList.vue       # 客户端列表
│   │   ├── protocols/     # 协议管理
│   │   │   ├── RtspManagement.vue    # RTSP管理
│   │   │   ├── RtmpManagement.vue    # RTMP管理
│   │   │   ├── WebrtcManagement.vue  # WebRTC管理
│   │   │   ├── SrtManagement.vue     # SRT管理
│   │   │   ├── Gb28181Management.vue # GB28181管理
│   │   │   └── Jt1078Management.vue  # JT1078管理
│   │   ├── record/        # 录制点播
│   │   │   ├── RecordTasks.vue       # 录制任务
│   │   │   └── VodManagement.vue     # 点播管理
│   │   └── monitor/       # 系统监控
│   │       ├── ServerMonitor.vue     # 服务器监控
│   │       ├── LoopsMonitor.vue      # 事件循环监控
│   │       └── PerformanceMonitor.vue # 性能监控
│   ├── App.vue            # 根组件
│   └── main.js            # 应用入口
├── index.html             # HTML模板
├── package.json           # 项目配置
├── vite.config.js         # Vite配置
└── README.md              # 项目说明
```

## API接口说明

### 配置管理
- `GET /api/v1/config` - 获取配置
- `POST /api/v1/config` - 更新配置

### 流媒体管理
- `GET /api/v1/getSourceList` - 获取流列表
- `GET /api/v1/getSourceInfo` - 获取流信息
- `POST /api/v1/closeSource` - 关闭流
- `GET /api/v1/getClientList` - 获取客户端列表
- `POST /api/v1/closeClient` - 关闭客户端

### 服务器信息
- `GET /api/v1/getServerInfo` - 获取服务器信息
- `GET /api/v1/version` - 获取版本信息
- `GET /api/v1/getLoopList` - 获取事件循环列表

### RTSP协议
- `POST /api/v1/rtsp/play/start` - 开始RTSP拉流
- `POST /api/v1/rtsp/play/stop` - 停止RTSP拉流
- `GET /api/v1/rtsp/play/list` - 获取RTSP拉流列表
- `POST /api/v1/rtsp/publish/start` - 开始RTSP推流
- `POST /api/v1/rtsp/publish/stop` - 停止RTSP推流
- `GET /api/v1/rtsp/publish/list` - 获取RTSP推流列表

### RTMP协议
- `POST /api/v1/rtmp/play/start` - 开始RTMP拉流
- `POST /api/v1/rtmp/play/stop` - 停止RTMP拉流
- `GET /api/v1/rtmp/play/list` - 获取RTMP拉流列表
- `POST /api/v1/rtmp/publish/start` - 开始RTMP推流
- `POST /api/v1/rtmp/publish/stop` - 停止RTMP推流
- `GET /api/v1/rtmp/publish/list` - 获取RTMP推流列表

### WebRTC协议
- `POST /api/v1/rtc/play` - WebRTC播放
- `POST /api/v1/rtc/publish` - WebRTC推流
- `POST /api/v1/rtc/pull/start` - 开始WebRTC拉流
- `POST /api/v1/rtc/pull/stop` - 停止WebRTC拉流
- `GET /api/v1/rtc/pull/list` - 获取WebRTC拉流列表

### 录制管理
- `POST /api/v1/record/start` - 开始录制
- `POST /api/v1/record/stop` - 停止录制
- `GET /api/v1/record/list` - 获取录制列表

### 点播管理
- `POST /api/v1/vod/start` - 开始点播
- `POST /api/v1/vod/stop` - 停止点播
- `POST /api/v1/vod/control` - 控制点播

## 配置说明

### 代理配置
在 `vite.config.js` 中配置了API代理：
```javascript
server: {
  proxy: {
    '/api': {
      target: 'http://localhost:80',
      changeOrigin: true
    }
  }
}
```

### 环境变量
可以通过环境变量配置API地址：
- `VITE_API_BASE_URL` - API基础地址

## 部署说明

### 1. 构建项目
```bash
npm run build
```

### 2. 部署到Web服务器
将 `dist` 目录下的文件部署到Web服务器（如Nginx、Apache等）

### 3. 配置反向代理
在Web服务器中配置API反向代理，将 `/api` 请求转发到MediaServer的API端口。

Nginx配置示例：
```nginx
location /api/ {
    proxy_pass http://localhost:80/api/;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
}
```

## 功能说明

### 仪表盘
- 实时显示服务器状态和统计信息
- 流量趋势图表
- 协议分布饼图
- 最近活动流列表

### 系统配置
- **服务器配置**: 基础配置、SSL证书、日志配置等
- **协议配置**: HTTP、RTSP、RTMP、WebRTC、SRT等协议服务器配置
- **Hook配置**: HTTP回调和Kafka消息配置
- **CDN配置**: CDN模式、服务器配置、流过滤等

### 流媒体管理
- **流列表**: 查看所有活跃流，支持搜索和过滤
- **客户端管理**: 查看和管理连接的客户端

### 协议管理
- 支持RTSP、RTMP、WebRTC、SRT、GB28181、JT1078等协议
- 拉流/推流管理
- 服务器状态监控

### 录制点播
- **录制任务**: 创建、管理录制任务
- **点播管理**: 点播控制、进度管理

### 系统监控
- **服务器监控**: CPU、内存、网络等系统资源监控
- **事件循环监控**: 事件循环性能监控
- **性能统计**: API性能统计和分析

## 开发指南

### 添加新页面
1. 在 `src/views/` 下创建新的Vue组件
2. 在 `src/router/index.js` 中添加路由配置
3. 在 `src/App.vue` 中添加菜单项

### 添加新API
1. 在 `src/api/index.js` 中添加API接口定义
2. 在组件中导入并使用API

### 自定义主题
可以通过修改Element Plus的CSS变量来自定义主题样式。

## 许可证

MIT License
