@echo off
chcp 65001 >nul
title MediaServer Web管理界面

echo ===================================
echo MediaServer Web管理界面启动脚本
echo ===================================

:: 检查Node.js是否安装
node -v >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: Node.js 未安装，请先安装 Node.js ^(版本 ^>= 16.0.0^)
    pause
    exit /b 1
)

:: 检查npm是否安装
npm -v >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: npm 未安装，请先安装 npm
    pause
    exit /b 1
)

:: 显示Node.js版本
for /f "tokens=*" %%i in ('node -v') do set NODE_VERSION=%%i
echo Node.js 版本: %NODE_VERSION% ✓

:: 检查是否存在node_modules目录
if not exist "node_modules" (
    echo 未找到 node_modules 目录，正在安装依赖...
    npm install
    if %errorlevel% neq 0 (
        echo 错误: 依赖安装失败
        pause
        exit /b 1
    )
    echo 依赖安装完成 ✓
) else (
    echo 依赖已安装 ✓
)

:: 检查MediaServer是否运行
echo 检查MediaServer状态...
curl -s http://localhost:80/api/v1/version >nul 2>&1
if %errorlevel% equ 0 (
    echo MediaServer 运行正常 ✓
) else (
    echo 警告: 无法连接到MediaServer ^(http://localhost:80^)
    echo 请确保MediaServer已启动并运行在80端口
    echo 继续启动Web管理界面...
)

echo.
echo 启动Web管理界面...
echo 访问地址: http://localhost:3000
echo 按 Ctrl+C 停止服务
echo.

:: 启动开发服务器
npm run dev

pause
