# CultivationOpenRealm

基于 Unreal Engine 5.8 的修真题材开放世界项目，核心目标是打造一套可复用的 **CORUI 游戏 UI 框架**。

## 项目概况

| 项目 | 说明 |
|------|------|
| 引擎 | Unreal Engine 5.8 |
| 脚本 | TypeScript（via Puerts / V8 9.4.146.24） |
| 目标平台 | Win64 / Android / Mac / iOS / Linux / OHOS |
| UI 框架 | CORUI（自研层级化 UI 系统） |
| 流程治理 | OpenFeel（`.openfeel/`，需求/阶段/审查/归档） |
| 当前状态 | v1.6.2 已归档，阶段 `v1.6.3-stage-07` 执行中 |

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
┌──────────────────────────────────────────────────────────────┐
│ TypeScript 业务层                                             │
│  CorStart → Manager 层（UIManager / DataManager / GameManager）│
│  UI Mixin（blueprint.mixin 注入蓝图）· ViewModel 层 · EventBus │
├──────────────────────────────────────────────────────────────┤
│ Puerts 桥接层                                                  │
│  JsEnvManagerSubsystem（JS 虚拟机启动 / 生命周期）              │
├──────────────────────────────────────────────────────────────┤
│ CORUI C++ 层                                                  │
│  UCORUIManager（6 层 Push/Pop 协调）· UCORWindow（位标志状态机）│
│  UCORStreamableManager（异步加载）· 数据表 JSON/CSV 管线        │
├──────────────────────────────────────────────────────────────┤
│ Unreal Engine 5.8                                             │
└──────────────────────────────────────────────────────────────┘
```

### 核心模块

| 模块 | 类型 | 位置 | 说明 |
|------|------|------|------|
| `CultivationOpenRealm` | Runtime | `Source/CultivationOpenRealm` | 项目主模块（CorGameInstance / GameMode） |
| `CORUI` | Runtime | `Plugins/CORUI/Source/CORUI` | UI 框架核心：层级容器、窗口状态机、异步加载、数据表管线 |
| `JsEnvManager` | Runtime | `Plugins/CORUI/Source/JsEnvManager` | JS 虚拟机启动与生命周期 |
| `PuertsMixinGenerator` | Editor | `Plugins/CORUI/Source/PuertsMixinGenerator` | 蓝图→TS Mixin 生成、UMG 设计器扩展（DrawCall 预估面板） |
| `Puerts` | 第三方 | `Plugins/Puerts` | TypeScript/V8 桥接（腾讯开源，V8 库需单独下载） |

### UI 层级

| 层级 | 用途 |
|------|------|
| Background | 背景层 |
| Normal | 常规 UI |
| Top | 顶层 / 弹出层 |
| Message | 消息层（不支持关闭） |
| Debug | 调试层（仅开发环境） |

### 关键设计模式

- **层级化管理**：5 个功能层级，每层独立 Canvas Container + ZOrder 控制，支持 Push/Pop 栈操作
- **窗口状态机**：`UCORWindow` 位标志状态（Shown/Hidden/Covered/HiddenUnder），自动处理窗口间遮盖与恢复
- **Puerts Mixin 注入**：TS 类通过 `blueprint.mixin()` 注入蓝图，直接访问控件成员，无需手动绑定
- **子系统架构**：`UCORUIManager`、`UCORStreamableManager`、`UJsEnvManagerSubsystem` 均继承 `UGameInstanceSubsystem`
- **单例 + UISingle 通信**：TS 管理器层单例，UI 间通过 UISingle 键值对数据包传递参数

### v1.6.2 新增能力

| 能力 | 位置 | 说明 |
|------|------|------|
| 事件总线 | `TypeScript/Utils/Event/EventBus.ts`、`TypeScript/Game/Events/GameEvents.ts` | 类型安全的事件契约（`data:table-cached`、`ui:stopped`、`toast:shown` 等），支持 Once、异常隔离、生命周期解除 |
| ViewModel 层 | `TypeScript/Game/UI/ViewModel/ViewModelBase.ts` | patch 合并 + 变更通知，首帧只渲染一次，屏蔽显式 `undefined` |
| 数据表管线 | `CORUI` C++ + `TypeScript/Manager/DataManager.ts` | JSON/CSV 导入导出、`LoadTableByPath` 运行时按路径加载、TS 侧缓存并广播事件 |
| 调试命令 | `Plugins/CORUI/Source/CORUI` | `CORUI.Travel`、`CORUI.VerifyOwnership`（仅非 Shipping）、`CORUI.DrawCallOverlay`（编辑器） |
| 设计器 DrawCall 面板 | `PuertsMixinGenerator` | UMG 设计器左上角显示预估 DC，含分类计数、扫描数、未识别项、签名节流与 CVar 开关 |

## 快速开始

### 环境要求

- Unreal Engine 5.8
- **V8 预编译库 9.4.146.24**（见上方"首次克隆必读"）
- Node.js 18+ 与 npm（TypeScript 编译）
- Git LFS（`Content/` 下的 `.uasset` / `.umap`）

### 启动步骤

1. 克隆仓库
2. 按上文补下 V8 库到 `Plugins/Puerts/ThirdParty/`
3. 生成 UE 项目文件（右键 `.uproject` → Generate Visual Studio project files）
4. 编译 Editor Target（或直接打开 `.uproject` 让引擎编译）
5. 安装依赖并编译 TypeScript：

   ```bash
   npm install
   npm run build      # 或 npm run watch 监听编译；也可 npm run typecheck 只做类型检查
   ```

   源码在 `TypeScript/`，产物输出到 `Content/JavaScript/`
6. 打开编辑器运行；也可用 `Plugins/CORUI/Tools/watch-ts/` 的批处理做监听编译

### 创建新 UI

1. 在 UMG 中创建控件蓝图（继承 `UCORWindow` 或 `UCORUserWidget`）
2. 用 PuertsMixinGenerator 生成 TS Mixin 模板（蓝图编辑器工具栏按钮）
3. 实现 Mixin 生命周期方法（`OnOpen`、`OnClose`、`OnShow`、`OnHide`）
4. 在对应 Manager 中调用 `UIManager.PushUI("UI_XXX")` 打开界面
5. 复杂数据建议走 ViewModel + EventBus，避免 UI 直接互相调用

## 调试与验证

| 命令 / 开关 | 作用 | 说明 |
|---|---|---|
| `CORUI.Travel <LevelPath>` | 在当前 PIE/Game 世界切图 | 仅非 Shipping；用于验证 UI 会话拆建 |
| `CORUI.VerifyOwnership` | 全量 GC 后校验 UI 所有权链 | 仅非 Shipping；输出 `RESULT=PASS/FAIL/SKIP` 单行 |
| `CORUI.DrawCallOverlay 0/1` | 关闭 / 开启设计器 DrawCall 面板 | 编辑器 CVar，默认开启 |

- 日志：`Saved/Logs/CultivationOpenRealm.log`，关键前缀 `[CORUI]`、`[PmgDesigner]`、`[CORUI][Owner]`
- 观澜 UI 自检：`TestUIManager.AutoDemo = true` 后打开 `UI_TestHub`，自动跑测试套件（9/9 PASS 为基线）

## 项目目录

| 目录 | 说明 |
|------|------|
| `Source/CultivationOpenRealm/` | C++ 项目主模块（GameInstance 等） |
| `Plugins/CORUI/` | CORUI 框架插件（3 个模块 + `Tools/watch-ts`） |
| `Plugins/Puerts/` | Puerts 桥接插件（V8 库需单独下载） |
| `TypeScript/` | TS 业务：`Manager/`、`Game/UI/`、`Game/Events/`、`Utils/`、`Config/` |
| `Typing/` | 类型声明（`cpp/`、`ffi/`、`puerts/`、`ue/`） |
| `Content/` | UE 资产（UI 蓝图、数据表、地图、JavaScript 产物） |
| `Config/` | 引擎与插件配置 |
| `docs/` | 设计稿、评审记录、问题清单、交接文档 |
| `.openfeel/` | OpenFeel 流程治理（需求、阶段、审查、归档） |

## 文档索引

- [AGENTS.md](./AGENTS.md) — **给 AI 编码助手的入口说明**（速查、配方、陷阱、验证）
- [docs/ai/framework-usage.md](./docs/ai/framework-usage.md) — 框架使用手册（C++/TS API、生命周期、排障）
- [docs/ai/port-to-new-project.md](./docs/ai/port-to-new-project.md) — 移植到新项目的清单与 AI 任务书
- [CORUI 插件分析（2026-04 初版，2026-11 更新）](./开发文档_CORUI插件分析.md)
- `docs/design/` — UI 设计稿与截图（[ui-test-suite.md](./docs/design/ui-test-suite.md)、崩溃分析）
- `docs/reviews/` — 代码评审与修复记录（如 [corui-review-2026-11-05.md](./docs/reviews/corui-review-2026-11-05.md)）
- `docs/issues/` — 问题清单（OpenFeel 反馈与守卫问题）
- `docs/handoff/` — 跨会话交接

## 开发流程

项目使用 OpenFeel 做流程治理：

- 需求（REQ）是工作单元，阶段（stage）是版本容器
- 变更走 `feel_req` / `feel_stage` / `feel_flow` 记录，审查（REV）阻塞未闭环的版本归档
- v1.6.2 的 6 条需求（事件总线、ViewModel、数据表、Travel、F1 所有权、DrawCall）已全部完成并归档

## 依赖插件

| 插件 | 说明 |
|------|------|
| Puerts | TypeScript/JavaScript 运行时（腾讯开源） |
| EnhancedInput | UE 增强输入系统 |
| UMG | Unreal Motion Graphics UI 框架 |
