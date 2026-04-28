@echo off
chcp 65001 >nul
SETLOCAL ENABLEDELAYEDEXPANSION

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo ===============================================
echo   TypeScript 监控启动器
echo ===============================================
echo.

:: 检查 Node.js 是否安装
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [错误] 未检测到 Node.js 环境
    echo 下载地址：https://nodejs.org/
    echo.
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('node --version 2^>nul') do set "NODE_VERSION=%%i"
echo [信息] Node.js: %NODE_VERSION%
echo.

:: 向上查找 tsconfig.json（最多 4 层）
echo [信息] 正在查找 tsconfig.json...
set "SEARCH_DIR=%SCRIPT_DIR%"
set "PROJECT_DIR="
set "FOUND=0"

for /l %%i in (0,1,4) do (
    if exist "!SEARCH_DIR!tsconfig.json" (
        set "PROJECT_DIR=!SEARCH_DIR!"
        set "FOUND=1"
        goto :found
    )
    if "!SEARCH_DIR:~-1!"=="\" (
        set "SEARCH_DIR=!SEARCH_DIR:~0,-1!"
    )
    for %%A in ("!SEARCH_DIR!") do set "SEARCH_DIR=%%~dpA"
)

:found
if "!FOUND!"=="0" (
    echo [错误] 向上查找 4 层未找到 tsconfig.json
    echo.
    echo 请确保项目根目录有 tsconfig.json
    echo.
    pause
    exit /b 1
)

echo [成功] 找到配置文件：!PROJECT_DIR!tsconfig.json
echo [信息] 项目目录：!PROJECT_DIR!
echo.

:: 检查 TypeScript 是否可用（优先检查本地，再检查全局）
set "TSC_FOUND=0"

:: 1. 先检查项目目录的 node_modules
if exist "!PROJECT_DIR!node_modules\.bin\tsc.cmd" (
    echo [信息] 发现本地 TypeScript (项目目录)
    set "PATH=!PROJECT_DIR!node_modules\.bin;%PATH%"
    set "TSC_FOUND=1"
    goto :check_version
)

:: 2. 检查脚本目录的 node_modules
if exist "%SCRIPT_DIR%node_modules\.bin\tsc.cmd" (
    echo [信息] 发现本地 TypeScript (脚本目录)
    set "PATH=%SCRIPT_DIR%node_modules\.bin;%PATH%"
    set "TSC_FOUND=1"
    goto :check_version
)

:: 3. 检查全局 tsc
where tsc >nul 2>nul
if %errorlevel% equ 0 (
    echo [信息] 发现全局 TypeScript
    set "TSC_FOUND=1"
    goto :check_version
)

:: 4. 都没找到，询问是否安装
echo [警告] 未检测到 TypeScript
echo.
set /p "INSTALL=是否全局安装 TypeScript? (Y/N): "
if /i "!INSTALL!"=="Y" (
    echo.
    echo [提示] 正在全局安装 TypeScript...
    call npm install -g typescript
    if %errorlevel% neq 0 (
        echo [错误] 安装 TypeScript 失败，请手动安装
        echo 命令：npm install -g typescript
        echo.
        pause
        exit /b 1
    )
    echo [完成] TypeScript 安装成功
    set "TSC_FOUND=1"
) else (
    echo.
    echo [错误] 未安装 TypeScript，无法继续
    echo.
    pause
    exit /b 1
)

:check_version
:: 显示 TypeScript 版本
for /f "tokens=*" %%i in ('tsc --version 2^>nul') do set "TSC_VERSION=%%i"
echo [信息] TypeScript: %TSC_VERSION%
echo.

echo ===============================================
echo 启动监控脚本...
echo ===============================================
echo.

:: 设置项目目录环境变量，供 JS 脚本使用
set "TS_WATCH_PROJECT_DIR=!PROJECT_DIR!"

:: 启动 Node 脚本
node "%SCRIPT_DIR%ts-watch-wrapper.js"

:: 如果脚本退出，显示提示
echo.
echo ===============================================
echo 监控已退出
echo ===============================================
pause