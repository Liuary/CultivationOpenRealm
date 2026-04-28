@echo off
chcp 65001 >nul
SETLOCAL ENABLEDELAYEDEXPANSION

set "SCRIPT_DIR=%~dp0"
:: 找到项目目录（向上查找三层）
rem 确保路径以反斜杠结尾
if "!SCRIPT_DIR:~-1!" NEQ "\" set "SCRIPT_DIR=!SCRIPT_DIR!\"

set "TARGET_DIR=!SCRIPT_DIR!"
set "LEVELS=4"  rem 需要向上查找的层数

rem 通过循环向上查找指定层数
for /l %%i in (1,1,!LEVELS!) do (
    cd /d "!TARGET_DIR!"
    cd ..
    set "TARGET_DIR=!CD!"
)

rem 确保路径以反斜杠结尾（如果需要）
if "!TARGET_DIR:~-1!" NEQ "\" set "TARGET_DIR=!TARGET_DIR!\"

set "TS_CONFIG=%TARGET_DIR%tsconfig.json"
set "LOG_FILE=%SCRIPT_DIR%ts_errors.log"

if not exist "%TS_CONFIG%" (
    echo [警告] 未找到tsconfig.json配置文件，正在生成默认配置...
    tsc --init --target es2016 --module commonjs --strict true --esModuleInterop true
    echo 已生成默认tsconfig.json配置文件
)

goto start_monitor

:: 主输出函数
:printHeader
cls
echo ===============================================
echo    TypeScript代码错误实时监控系统
echo ===============================================
echo 启动时间: %date% %time%
if defined SCRIPT_DIR echo 脚本目录: %SCRIPT_DIR%
if defined TARGET_DIR echo 项目目录: %TARGET_DIR%
if defined TS_CONFIG echo 配置文件: %TS_CONFIG%
if defined LOG_FILE echo 日志文件: %LOG_FILE%
echo ===============================================
echo.
echo 开始监控TypeScript代码错误...
echo 按 Ctrl+C 停止监控
echo.
goto :eof

:start_monitor
echo [%time%] 执行TypeScript编译检查...

set need_clear=1

:monitor_loop
:: 关键修改：每次循环清屏，只显示最新结果
if %need_clear% equ 1 (
    call :printHeader
    echo 检查中...
)

call tsc --noEmit --project "%TS_CONFIG%" > "%LOG_FILE%" 2>&1

if %errorlevel% equ 0 (
    call :printHeader
    echo [%time%] ✓ 编译检查通过，未发现错误
    set need_clear=1
) else (
    :: 只在首次发现错误时清屏并显示错误
    if %need_clear% equ 1 (
        call :printHeader
        echo [%time%] ✗ 发现TypeScript错误!
        echo ===============================================
        type "%LOG_FILE%"
        echo ===============================================
        echo [注意] 错误已记录到日志文件: %LOG_FILE%
    )
    set need_clear=0
)

timeout /t 5 /nobreak >nul
goto monitor_loop