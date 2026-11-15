# CultivationOpenRealm

基于 Unreal Engine 5.5 的修真题材开放世界项目，核心目标是打造一套可复用的 **CORUI 游戏 UI 框架**。

## 项目概况

| 项目 | 说明 |
|------|------|
| 引擎 | Unreal Engine 5.8 |
| 脚本 | TypeScript (via Puerts / V8) |
| 目标平台 | Win64 / Android / Mac / iOS / Linux |
| UI 框架 | CORUI（自研层级化 UI 系统） |

## ⚠️ 首次克隆必读：V8 预编译库不随仓库分发

**克隆后不能直接编译**，必须先补下 Puerts 的 V8 预编译库。

- **缺什么**：`Plugins/Puerts/ThirdParty/v8_9.4.146.24/`（含 `Inc/`、`Lib/`、`Bin/`）
- **为什么不在仓库里**：这些库体积过大（单个 `wee8.lib` 最大约 188 MB，超过 GitHub 单文件 100 MB 限制），
  且二进制不适合进版本库，仓库历史已清理
- **后果**：不补库直接 Generate/Build 会报找不到 V8 库或链接失败

### 下载方式（与当前工程匹配的版本 9.4.146.24）

1. 直链下载：

   https://github.com/puerts/backend-v8/releases/download/V8_9.4.146.24_240430/v8_bin_9.4.146.24.tgz

2. 解压到 `Plugins/Puerts/ThirdParty/`，确保得到：

   ```
   Plugins/Puerts/ThirdParty/v8_9.4.146.24/{Inc,Lib,Bin}
   ```

3. 版本必须与 `Plugins/Puerts/Source/JsEnv/JsEnv.build.cs` 里的 `UseV8Version` 一致
   （当前为 `V9_4_146_24`，工程默认值）。

其他可选方式：

- **Puerts Unreal 发布包**：从 [Puerts Releases](https://github.com/Tencent/puerts/releases)
  下载对应 UE 版本的 Unreal 发布包，解压到项目 `Plugins/`，包内已含 V8 库
- **官方安装文档**：https://puerts.github.io/docs/puerts/unreal/install/
- 其他 V8 版本（8.4.371.19 / 10.6.194 / 11.8.172）见
  [backend-v8 releases](https://github.com/puerts/backend-v8/releases)，切换后需同步改 `UseV8Version`

> `.gitignore` 已忽略 `Plugins/Puerts/ThirdParty/v8_*/Lib/`、`Bin/` 与 `*.lib` 等二进制，
> 补进来的库文件不会被误提交。

## 技术架构

```
┌─────────────────────────────────────────────────┐
│                  TypeScript 层                    │
│  CorStart → Manager 层 → UI Mixin 注入蓝图        │
├─────────────────────────────────────────────────┤
│                  Puerts 桥接层                    │
│  JsEnvManagerSubsystem（JS 虚拟机生命周期）         │
├─────────────────────────────────────────────────┤
│                  CORUI C++ 层                     │
│  UCORUIManager（6层Push/Pop协调）                  │
│  UCORWindow（位标志状态机）                         │
│  UCORStreamableManager（异步加载）                  │
├─────────────────────────────────────────────────┤
│              Unreal Engine 5.5                    │
└─────────────────────────────────────────────────┘
```

### 核心模块

| 模块 | 类型 | 说明 |
|------|------|------|
| `CultivationOpenRealm` | Runtime | 项目主模块（CorGameInstance） |
| `CORUI` | Runtime | UI 框架核心，6 层级管理、窗口状态机、异步加载 |
| `JsEnvManager` | Runtime | JS 虚拟机启动/生命周期管理 |
| `PuertsMixinGenerator` | Editor | 蓝图→TS Mixin 文件自动生成、UMG 设计器扩展 |

### UI 层级

| 层级 | 用途 |
|------|------|
| Background | 背景层 |
| Normal | 常规 UI |
| Top | 顶层/弹出层 |
| Message | 消息层（不支持关闭） |
| Debug | 调试层（仅开发环境） |

### 关键设计模式

- **层级化管理**：5 个功能层级，每层独立 Canvas Container + ZOrder 控制，支持 Push/Pop 栈操作
- **窗口状态机**：`UCORWindow` 位标志状态（Shown/Hidden/Covered/HiddenUnder），自动处理窗口间遮盖与恢复
- **Puerts Mixin 注入**：TS 类通过 `blueprint.mixin()` 注入蓝图，直接访问控件成员，无需手动绑定
- **子系统架构**：`UCORUIManager` 和 `UCORStreamableManager` 均继承 `UGameInstanceSubsystem`，自动生命周期管理
- **单例+UISingle 通信**：TS 管理器层单例模式，UI 间通过 UISingle 键值对数据包传递参数

## 快速开始

### 环境要求

- Unreal Engine 5.5
- Node.js（用于 TypeScript 编译）
- Git LFS

### 启动步骤

1. 克隆仓库
2. 生成 UE 项目文件（右键 `.uproject` → Generate Visual Studio project files）
3. 打开 `.uproject` 启动编辑器
4. TypeScript 代码位于 `TypeScript/` 目录，编译输出到 `Content/JavaScript/`

### 创建新 UI

1. 在 UMG 中创建控件蓝图（继承 `UCORWindow` 或 `UCORUserWidget`）
2. 使用 PuertsMixinGenerator 生成 TS Mixin 模板（蓝图编辑器工具栏按钮）
3. 实现 Mixin 类中的生命周期方法（`OnOpen`、`OnClose`、`OnShow`、`OnHide`）
4. 在合适的 Manager 中调用 `UIManager.PushUI("UI_XXX")` 打开界面

## 项目目录

| 目录 | 说明 |
|------|------|
| `Source/` | C++ 源码（项目模块 + CORUI 插件） |
| `Plugins/CORUI/` | CORUI UI 框架插件 |
| `Plugins/Puerts/` | Puerts TypeScript/V8 桥接插件（腾讯） |
| `TypeScript/` | TS 业务逻辑（Manager 层、UI Mixin、工具类） |
| `Typing/` | TypeScript 类型声明（UE、Puerts、CORUI） |
| `Content/` | UE 资产（UI 蓝图、数据表、地图） |
| `Config/` | 引擎与插件配置 |

## 分析文档

详细架构分析与设计讨论见：

- [开发文档 - CORUI 插件分析](./开发文档_CORUI插件分析.md)

## 依赖插件

| 插件 | 说明 |
|------|------|
| Puerts | TypeScript/JavaScript 运行时（腾讯开源） |
| EnhancedInput | UE 增强输入系统 |
| UMG | Unreal Motion Graphics UI 框架 |
