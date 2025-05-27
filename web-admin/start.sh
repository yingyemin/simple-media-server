#!/bin/bash

# MediaServer Web管理界面启动脚本

echo "==================================="
echo "MediaServer Web管理界面启动脚本"
echo "==================================="

# 检查Node.js是否安装
if ! command -v node &> /dev/null; then
    echo "错误: Node.js 未安装，请先安装 Node.js (版本 >= 16.0.0)"
    exit 1
fi

# 检查npm是否安装
if ! command -v npm &> /dev/null; then
    echo "错误: npm 未安装，请先安装 npm"
    exit 1
fi

# 检查Node.js版本
NODE_VERSION=$(node -v | cut -d'v' -f2)
REQUIRED_VERSION="16.0.0"

if [ "$(printf '%s\n' "$REQUIRED_VERSION" "$NODE_VERSION" | sort -V | head -n1)" != "$REQUIRED_VERSION" ]; then
    echo "错误: Node.js 版本过低，当前版本: $NODE_VERSION，要求版本: >= $REQUIRED_VERSION"
    exit 1
fi

echo "Node.js 版本: $NODE_VERSION ✓"

# 检查是否存在node_modules目录
if [ ! -d "node_modules" ]; then
    echo "未找到 node_modules 目录，正在安装依赖..."
    npm install
    if [ $? -ne 0 ]; then
        echo "错误: 依赖安装失败"
        exit 1
    fi
    echo "依赖安装完成 ✓"
else
    echo "依赖已安装 ✓"
fi

# 检查MediaServer是否运行
echo "检查MediaServer状态..."
if curl -s http://localhost:80/api/v1/version > /dev/null 2>&1; then
    echo "MediaServer 运行正常 ✓"
else
    echo "警告: 无法连接到MediaServer (http://localhost:80)"
    echo "请确保MediaServer已启动并运行在80端口"
    echo "继续启动Web管理界面..."
fi

echo ""
echo "启动Web管理界面..."
echo "访问地址: http://localhost:3000"
echo "按 Ctrl+C 停止服务"
echo ""

# 启动开发服务器
npm run dev
