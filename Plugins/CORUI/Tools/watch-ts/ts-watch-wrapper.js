#!/usr/bin/env node
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

// 清屏
function clearScreen() {
    process.stdout.write('\x1Bc');
}

// 打印头部
function printHeader(projectDir, tsConfigPath) {
    console.log('===============================================');
    console.log('   TypeScript 代码编译监控 (监听模式)');
    console.log('===============================================');
    console.log(`启动时间：${new Date().toLocaleString('zh-CN')}`);
    console.log(`当前时间：${new Date().toLocaleTimeString()}`);
    console.log(`项目目录：${projectDir}`);
    console.log(`配置文件：${tsConfigPath}`);
    console.log('===============================================');
    console.log('');
    console.log('📁 正在监听文件变化...');
    console.log('💾 文件变化时自动编译输出 JS');
    console.log('按 Ctrl+C 停止监控');
    console.log('===============================================');
    console.log('');
}

// 检查 tsc 是否可用
function checkTsc() {
    return new Promise((resolve) => {
        const check = spawn('tsc', ['--version'], { shell: true });
        let output = '';
        
        check.stdout.on('data', (data) => {
            output += data.toString();
        });
        
        check.on('close', (code) => {
            if (code === 0) {
                console.log(`✅ TypeScript: ${output.trim()}`);
                resolve(true);
            } else {
                console.log('❌ 未找到 tsc 命令');
                resolve(false);
            }
        });
        
        check.on('error', (err) => {
            console.log(`❌ tsc 检查失败：${err.message}`);
            resolve(false);
        });
    });
}

// 等待用户按键后退出
function waitForExit(code = 1) {
    console.log('');
    console.log('按任意键退出...');
    process.stdin.resume();
    process.stdin.once('data', () => {
        process.exit(code);
    });
}

// 主函数 - 返回 Promise
function main() {
    return new Promise((resolve, reject) => {
        console.log('🔍 正在初始化...');
        console.log('');
        
        // 从环境变量获取项目目录（由 bat 脚本设置）
        const projectDir = process.env.TS_WATCH_PROJECT_DIR;
        
        if (!projectDir) {
            console.log('❌ 错误：未设置项目目录环境变量');
            console.log('请通过 ts-watch.bat 启动此脚本');
            reject(new Error('未设置项目目录环境变量'));
            return;
        }
        
        const tsConfigPath = path.join(projectDir, 'tsconfig.json');
        
        // 验证 tsconfig.json 存在
        if (!fs.existsSync(tsConfigPath)) {
            console.log(`❌ 错误：未找到 ${tsConfigPath}`);
            reject(new Error(`未找到配置文件：${tsConfigPath}`));
            return;
        }
        
        console.log(`✅ 配置文件：${tsConfigPath}`);
        console.log(`✅ 项目目录：${projectDir}`);
        console.log('');
        
        // 检查 tsc
        console.log('🔍 检查 TypeScript...');
        checkTsc().then((hasTsc) => {
            if (!hasTsc) {
                console.log('');
                console.log('💡 解决方案:');
                console.log('   1. 全局安装：npm install -g typescript');
                console.log('   2. 或本地安装：npm install typescript --save-dev');
                reject(new Error('未找到 TypeScript'));
                return;
            }
            
            console.log('');
            console.log('🚀 启动 TypeScript 监听服务...');
            console.log('');
            
            // 等待一下再清屏并切换目录
            setTimeout(() => {
                // 切换到项目目录
                process.chdir(projectDir);
                clearScreen();
                printHeader(projectDir, tsConfigPath);
                startWatch(tsConfigPath);
                resolve(); // 启动成功后 resolve
            }, 1000);
        }).catch(reject);
    });
}

let tsc = null;
let isCompiling = false;
let errorCount = 0;

function startWatch(tsConfigPath) {
    // ⚠️ 关键修改：移除 --noEmit，让 tsc 正常输出 JS 文件
    tsc = spawn('tsc', ['--watch', '--preserveWatchOutput'], {
        stdio: ['inherit', 'pipe', 'pipe'],
        env: { ...process.env, FORCE_COLOR: '1' },
        shell: true
    });
    
    let outputBuffer = '';
    
    tsc.stdout.on('data', (data) => {
        const text = data.toString();
        outputBuffer += text;
        
        // 检测新的编译周期开始
        if (text.includes('File change detected') || text.includes('Starting incremental compilation')) {
            isCompiling = true;
            errorCount = 0;
            clearScreen();
            printHeader(process.cwd(), tsConfigPath);
            console.log('⏱️  [' + new Date().toLocaleTimeString() + '] 检测到文件变化，重新编译中...');
            console.log('');
            // 打印原始输出
            process.stdout.write(text);
            return;
        }
        
        // 统计错误数量
        const errorMatches = text.match(/error TS\d+/g);
        if (errorMatches) {
            errorCount += errorMatches.length;
        }
        
        // 打印原始输出
        process.stdout.write(text);
        
        // 检测编译完成
        if (text.includes('Watching for file changes')) {
            isCompiling = false;
            console.log('');
            if (errorCount === 0) {
                console.log('✅ 编译成功 - JS 文件已生成');
            } else {
                console.log(`❌ 发现 ${errorCount} 个错误 - JS 文件可能不完整`);
            }
            console.log('⏳ 等待下一次文件变化...');
            console.log('===============================================');
            errorCount = 0;
        }
    });
    
    tsc.stderr.on('data', (data) => {
        const text = data.toString();
        console.error('❌ ' + text);
    });
    
    tsc.on('error', (err) => {
        console.log('');
        console.log('❌ tsc 进程错误:', err.message);
        waitForExit(1);
    });
    
    tsc.on('close', (code) => {
        console.log('');
        console.log(`⚠️  TypeScript 进程已退出，代码：${code}`);
        waitForExit(code);
    });
}

// 处理退出
process.on('SIGINT', () => {
    console.log('');
    console.log('👋 监控已停止，再见!');
    if (tsc) {
        tsc.kill();
    }
    process.exit(0);
});

// 运行主函数
main().catch((err) => {
    console.log('');
    console.log('❌ 启动失败:', err.message);
    console.log('');
    console.log('错误详情:');
    console.log(err.stack);
    waitForExit(1);
});