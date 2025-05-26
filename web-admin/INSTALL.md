# MediaServer Web管理界面安装指南

## 快速启动

### 方法一：使用启动脚本（推荐）

#### Linux/macOS
```bash
cd web-admin
./start.sh
```

#### Windows
```cmd
cd web-admin
start.bat
```

### 方法二：手动安装

#### 1. 安装Node.js
确保已安装Node.js (版本 >= 16.0.0)
- 下载地址：https://nodejs.org/

#### 2. 安装依赖
```bash
cd web-admin
npm install
```

#### 3. 启动开发服务器
```bash
npm run dev
```

#### 4. 访问管理界面
打开浏览器访问：http://localhost:3000

## 生产部署

### 1. 构建项目
```bash
npm run build
```

### 2. 部署到Web服务器
将 `dist` 目录下的文件部署到Web服务器（如Nginx、Apache等）

### 3. 配置反向代理
配置Web服务器将API请求转发到MediaServer

#### Nginx配置示例
```nginx
server {
    listen 80;
    server_name your-domain.com;
    
    # 静态文件
    location / {
        root /path/to/web-admin/dist;
        try_files $uri $uri/ /index.html;
    }
    
    # API代理
    location /api/ {
        proxy_pass http://localhost:80/api/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

## 故障排除

### 1. 端口冲突
如果3000端口被占用，可以修改 `vite.config.js` 中的端口配置：
```javascript
server: {
  port: 3001, // 修改为其他端口
  // ...
}
```

### 2. API连接失败
- 确保MediaServer已启动并运行在80端口
- 检查防火墙设置
- 确认API接口地址配置正确

### 3. 依赖安装失败
- 尝试使用国内镜像：`npm install --registry https://registry.npmmirror.com`
- 清除缓存：`npm cache clean --force`
- 删除node_modules重新安装：`rm -rf node_modules && npm install`

## 功能说明

管理界面提供以下功能：

1. **仪表盘** - 实时监控服务器状态
2. **系统配置** - 服务器和协议配置管理
3. **流媒体管理** - 流列表和客户端管理
4. **协议管理** - RTSP、RTMP、WebRTC等协议管理
5. **录制点播** - 录制任务和点播控制
6. **系统监控** - 性能监控和事件循环状态

## 技术支持

如有问题，请查看：
1. README.md 文件
2. 项目文档
3. 控制台错误信息
