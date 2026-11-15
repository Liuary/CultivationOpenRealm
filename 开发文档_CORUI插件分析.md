# CultivationOpenRealm 项目开发文档 — CORUI 插件分析

> **项目名称**: CultivationOpenRealm（修真开放世界）  
> **插件名称**: CORUI（Cultivation Open Realm UI）  
> **作者**: Liuary  
> **核心依赖**: UE 5.8 + Puerts (TypeScript/JavaScript 绑定)  
> **文档日期**: 2026-04-28 初版（基于 v1.0）  
> **最近更新**: 2026-11-15（v1.6.2 复核）

> **阅读提示**：第 3~6 章为 2026-04 的初版分析，模块划分仍成立；
> 第 7 章「不足之处」是当时的快照，其中部分条目已在 v1.1 ~ v1.6.2 修复。
> 请一律对照文末 **[第 8 章：演进与现状](#8-v11--v162-演进与现状2026-11-复核)** 阅读。

---

## 目录

1. [项目概述](#1-项目概述)
2. [CORUI 插件整体结构](#2-corui-插件整体结构)
3. [模块详解](#3-模块详解)
   - 3.1 [CORUI 核心模块](#31-corui-核心模块)
   - 3.2 [JsEnvManager 模块](#32-jsenvmanager-模块)
   - 3.3 [PuertsMixinGenerator 模块](#33-puertsmixingenator-模块)
4. [TypeScript 项目结构](#4-typescript-项目结构)
5. [数据流与交互机制](#5-数据流与交互机制)
6. [特点总结](#6-特点总结)
7. [不足之处与改进建议](#7-不足之处与改进建议)

---

## 1. 项目概述

CultivationOpenRealm 是一个基于 Unreal Engine 5 的修真题材开放世界游戏项目。UI 系统采用 **CORUI** 插件作为统一管理方案，该插件构建在 **Puerts**（腾讯开源的 UE TypeScript 绑定方案）之上，允许开发者使用 TypeScript 编写 UI 逻辑，同时通过 C++ 提供高效的底层 UI 管理能力。

> Puerts 插件路径：[`Plugins/Puerts/`](Plugins/Puerts/)  
> CORUI 插件路径：[`Plugins/CORUI/`](Plugins/CORUI/)  
> TypeScript 源码路径：[`TypeScript/`](TypeScript/)

---

## 2. CORUI 插件整体结构

CORUI 插件内部由 **3 个模块** 组成，定义在 [`CORUI.uplugin`](Plugins/CORUI/CORUI.uplugin:26) 中：

| 模块名 | 类型 | 加载阶段 | 职责 |
|--------|------|----------|------|
| **CORUI** | Runtime | Default | 核心 UI 管理，包含层级容器、窗口系统、数据工具 |
| **JsEnvManager** | Runtime | Default | Puerts JS 虚拟机生命周期管理 |
| **PuertsMixinGenerator** | Editor | PostEngineInit | 编辑器工具：蓝图 Mixin 代码生成 + UMG 设计器扩展 |

### 文件目录结构

```
Plugins/CORUI/
├── CORUI.uplugin              # 插件描述文件
├── Config/
│   └── DefaultCORUI.ini       # 结构体重定向（LayerConfig → UILayerConfig）
├── Content/
│   └── Config/
│       └── DA_UILayerTypeZOrderConfigData.uasset  # 层级ZOrder配置数据资产
├── Resources/                 # 插件图标资源
├── Source/
│   ├── CORUI/                 # 核心 UI 模块
│   ├── JsEnvManager/          # JS 虚拟机管理器
│   └── PuertsMixinGenerator/  # Mixin 生成器（编辑器模块）
└── Tools/
    └── watch-ts/              # TypeScript 监控编译工具
```

---

## 3. 模块详解

### 3.1 CORUI 核心模块

位于 [`Plugins/CORUI/Source/CORUI/`](Plugins/CORUI/Source/CORUI/)，是 UI 系统的核心。

#### 3.1.1 总体架构

```
UCORUIManager (GameInstanceSubsystem)
├── 管理 UI 层级 (EUILayerType: None/BackGround/Normal/Top/Message/Debug)
│   └── 每层级拥有一个 UCORCanvasContainer
│       └── 内部维护 WindowStack (TArray<UCORWindow*>)
│           └── 窗口状态机 (EWindowState: None/Shown/Hidden/Covered/HiddenUnder)
├── WidgetPool (FUserWidgetPool) 用于窗口对象复用
└── 配置驱动 (UCORUIManagerSetting + UUILayerTypeZOrderConfigData)
```

#### 3.1.2 关键类

##### [`UCORUIManager`](Plugins/CORUI/Source/CORUI/Public/CORUIManager.h) — UI 管理器

继承自 [`UGameInstanceSubsystem`](Plugins/CORUI/Source/CORUI/Public/CORUIManager.h:16)，作为全局唯一的 UI 管理器。

- **初始化流程** ([`CORUIManager.cpp:16`](Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:16)):
  1. 读取 `UUILayerTypeZOrderConfigData` 数据资产，获取各层级 ZOrder 配置
  2. 按 ZOrder 对层级排序（ZOrder 越小越靠前显示）
  3. 绑定 `FGameModeEvents::GameModeInitializedEvent` 事件，等待世界初始化

- **UI 创建流程** ([`InitUI`](Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:119)):
  1. 创建根 `UCanvasPanel`
  2. 为每个层级创建一个 `UCORCanvasContainer` 并添加到根 Canvas
  3. 通过 `AddViewportWidgetContent` 添加到视口
  4. 广播 `OnUIManagerInit` 事件（TypeScript 侧监听此事件进入启动流程）

- **窗口管理 API**:
  - [`PushUI`](Plugins/CORUI/Source/CORUI/Public/CORUIManager.h:120) — 同步/异步加载窗口资产并推入层级
  - [`PopUI`](Plugins/CORUI/Source/CORUI/Public/CORUIManager.h:129) — 从层级移除窗口
  - `NativeUpdateUI` — 刷新所有层级窗口状态

##### [`UCORCanvasContainer`](Plugins/CORUI/Source/CORUI/Public/UI/Container/CORCanvasContainer.h) — 层级容器

继承自 [`UCanvasPanel`](Plugins/CORUI/Source/CORUI/Public/UI/Container/CORCanvasContainer.h:14)，每个层级一个实例。

- 维护两个数据结构：
  - `Windows` (`TMap<FName, TWeakObjectPtr<UCORWindow>>`) — 名称到窗口的映射
  - `WindowsStack` (`TArray<TWeakObjectPtr<UCORWindow>>`) — 窗口逻辑栈
- `PushUI` 将窗口添加到画布并推入栈顶
- `PopUI` 支持按名称移除，对 [`UCORFullScreen`](Plugins/CORUI/Source/CORUI/Private/UI/Container/CORCanvasContainer.cpp:95) 特殊处理（会弹出上方所有窗口）
- `UpdateWindowState` 从栈顶向下递归更新窗口状态

##### [`UCORWindow`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORWindow.h) — 基础窗口

继承自 [`UCORUserWidget`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORWindow.h:31)，是 UI 系统中最核心的可视组件。

**属性**:
- `WindowLayer` — 所属层级
- `WindowZOrder` — 层内相对 ZOrder
- `WindowName` — 唯一标识名
- `UnderWindowHide` — 是否隐藏底部窗口（全屏窗口专用）

**状态机** ([`NativeUpdateState`](Plugins/CORUI/Source/CORUI/Private/UI/Widget/CORWindow.cpp:27)):
```
EWindowState (位标志组合):
├── None        = 0       // 初始/无状态
├── Shown       = 1 << 0  // 已显示
├── Hidden      = 1 << 1  // 已隐藏（被上层要求隐藏）
├── Covered     = 1 << 2  // 已被覆盖
└── HiddenUnder = 1 << 3  // 标记隐藏底部窗口
```

状态更新从栈顶向下传播，上层窗口的可见性状态影响下层窗口。

##### [`UCORUserWidget`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORUserWidget.h) — 基础控件

继承自 `UUserWidget`，提供完整的声明周期方法：
- `NativeOpen` / `NativeClose` — 打开/关闭（遍历子控件递归调用）
- `NativeShow` / `NativeHide` — 显/隐（防重复调用）
- `OnOpen/OnClose/OnShow/OnHide` — 蓝图可重写的事件

##### 控件子类

| 类 | 基类 | 功能 | 是否完善 |
|----|------|------|----------|
| [`CORFullScreen`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORFullScreen.h) | UCORWindow | 全屏窗口，默认 `UnderWindowHide=true` | ✅ 基础完善 |
| [`CORButton`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORButton.h) | UButton | 空子类，为后续扩展预留 | ❌ 仅占位 |
| [`CORImage`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORImage.h) | UImage | 支持通过路径设置图片，支持异步/同步加载 | ✅ 基础完善 |
| [`CORTextBlock`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORTextBlock.h) | UTextBlock | 空子类，为后续扩展预留 | ❌ 仅占位 |
| [`CORProgressBar`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORProgressBar.h) | UProgressBar | 空子类，为后续扩展预留 | ❌ 仅占位 |

#### 3.1.3 工具类

##### [`CORSoftObjectPath`](Plugins/CORUI/Source/CORUI/Public/Core/Streamable/CORSoftObjectPath.h)

封装 `FSoftObjectPath`，用于在 TypeScript 侧传递资源引用。非 UObject 类型，通过 Puerts 手动注册导出。

##### [`UCORStreamableManager`](Plugins/CORUI/Source/CORUI/Public/Core/Streamable/CORStreamableManager.h)

继承自 `UGameInstanceSubsystem`，封装 `FStreamableManager` 的异步加载能力，供 TypeScript 调用。

- `RequestAsyncLoad` / `RequestAsyncLoadList` — 单资源/多资源异步加载
- `ReleaseHandle` — 释放句柄
- 使用 `int32` 句柄 ID 传递而非直接传递 `TSharedPtr<FStreamableHandle>`，避免 TypeScript 侧引用问题

##### [`UCORDataLibrary`](Plugins/CORUI/Source/CORUI/Public/Core/Utils/CORDataLibrary.h)

蓝图函数库，将 `UDataTable` 的行/全表数据导出为 JSON 字符串，自动将字段名首字母大写（PascalCase 转换）。

##### [`WeakObjectWrapper`](Plugins/CORUI/Source/CORUI/Public/UI/Utils/WeakObjectWrapper.h)

封装 `TWeakObjectPtr<UObject>`，使 TypeScript 侧能够安全持有 UE 对象的弱引用。

#### 3.1.4 配置系统

- [`UCORUIManagerSetting`](Plugins/CORUI/Source/CORUI/Config/CORUIManagerSetting.h) — 项目设置 `Project → Plugins → COR UIManager Settings`
  - `LayerTypeZOrderConfigPath` — 指向数据资产的软引用
  - `DefaultZOrderMultiplier` — 默认 ZOrder 倍率（默认值 100）
- [`UUILayerTypeZOrderConfigData`](Plugins/CORUI/Source/CORUI/Config/UILayerTypeZOrderConfigData.h) — 数据资产，存储层级到 ZOrder 的映射

#### 3.1.5 Puerts 类型导出

通过 [`AutoRegister.cpp`](Plugins/CORUI/Source/CORUI/Private/Core/AutoRegister/AutoRegister.cpp) 中定义的结构体在静态构造阶段注册导出到 TypeScript 的类型：

```cpp
// 导出到 TypeScript 的类型
CORSoftObjectPath          // 软对象路径
WeakObjectWrapper          // 弱指针包装
UCORStreamableManager      // 流式加载管理器
TArray<CORSoftObjectPath>  // 路径数组容器
```

> **注意**: [`CORUIManager.cpp`](Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:403) 中的 `UCORUIManager` Puerts 注册代码被注释掉了，当前的 `UCORUIManager` 并未直接导出到 TypeScript。

---

### 3.2 JsEnvManager 模块

位于 [`Plugins/CORUI/Source/JsEnvManager/`](Plugins/CORUI/Source/JsEnvManager/)，负责 Puerts JS 虚拟机的创建和管理。

#### [`UJsEnvManagerSubsystem`](Plugins/CORUI/Source/JsEnvManager/Public/JsEnvManagerSubsystem.h)

继承自 `UGameInstanceSubsystem`，声明了对 `UCORUIManager` 的依赖（`InitializeDependency`），确保 UI 管理器先初始化。

**启动流程**：
1. 监听 `GameModeInitializedEvent`
2. 延迟一帧后调用 `StartJsEnv`
3. 读取 `UJsEnvManagerSetting` 配置
4. 创建 `puerts::FJsEnv`，指定模块加载路径为 `JavaScript`，启动模块为 `CorStart`
5. 可选开启调试端口等待调试器连接

#### [`UJsEnvManagerSetting`](Plugins/CORUI/Source/JsEnvManager/Public/JsEnvManagerSetting.h)

配置项：
- `StartModuleName` — 启动模块名（默认 `CorStart`）
- `DebugPort` — 调试端口（默认 8080）
- `bWaitDebug` — 是否等待调试器连接

---

### 3.3 PuertsMixinGenerator 模块

位于 [`Plugins/CORUI/Source/PuertsMixinGenerator/`](Plugins/CORUI/Source/PuertsMixinGenerator/)，是一个**编辑器模块**，提供两个功能：

#### 3.3.1 TypeScript Mixin 文件生成

在蓝图编辑器工具栏添加"Generate Puerts Mixin File"按钮（`BP`、`WBP` 编辑器均支持）。

**生成逻辑** ([`GeneratePuertsMixinFile`](Plugins/CORUI/Source/PuertsMixinGenerator/Private/PuertsMixinGenerator.cpp:194)):
1. 获取当前打开的蓝图资产
2. 根据资产路径和命名约定生成 `.ts` 文件
3. 根据蓝图基类自动生成对应的生命周期方法模板
4. 将所有已生成的 mixin 文件路径写入 import 列表文件

**生命周期方法生成规则**:
- `UUserWidget` 子类 → `Construct()`、`Destruct()`
- `UCORUserWidget` 子类 → `OnOpen()`、`OnClose()`、`OnShow()`、`OnHide()`
- `UCORWindow` 子类 → `OnScriptInit(Single: UISingle)`

#### 3.3.2 UMG 设计器扩展

[`FPmgDesignerExtension`](Plugins/CORUI/Source/PuertsMixinGenerator/Public/PmgDesignerExtension.h) 在 UMG 设计器视口左上角显示控件信息：
- 当前编辑的控件名称
- 预估的 `DrawCall` 数量

DrawCall 计算遍历控件树，根据资源切换、裁剪状态、Canvas ZOrder 等因素进行估算。

---

## 4. TypeScript 项目结构

TypeScript 源码位于 [`TypeScript/`](TypeScript/)，是游戏逻辑的主要开发语言。

```
TypeScript/                        # 顶层 tsconfig.json 与入口
├── CorStart.ts               # 入口启动模块（Puerts 启动入口）
├── PuertsImportList.ts       # Mixin 文件导入列表（自动生成）
├── Config/
│   └── PreLoadDataTable.ts   # 预加载数据表配置
├── Game/
│   ├── Events/
│   │   └── GameEvents.ts     # 事件契约（v1.6.2 新增，配合 Utils/Event/EventBus）
│   └── UI/
│       ├── GameLoading/UI_GameLoading.ts
│       ├── GameStart/UI_GameStart.ts、UI_GameStartSingleType.ts
│       ├── ViewModel/ViewModelBase.ts   # ViewModel 基类（v1.6.2）
│       └── Test/                        # 观澜 UI 测试套件 Mixin
│           UI_TestHub.ts、UI_TestData.ts、UI_TestStack.ts、UI_TestToast.ts 等
├── Manager/
│   ├── UIManager.ts           # UI 管理器（TS 侧，封装 PushUI/PopUI）
│   ├── GameManager.ts / GameManagerBase.ts
│   ├── GameLoadingManager.ts / GameStartManager.ts
│   ├── DataManager.ts         # 数据表缓存 + 事件广播（v1.6.2）
│   ├── TestUIManager.ts       # 测试套件入口（AutoDemo 开关）
│   └── TimeManager.ts
└── Utils/
    ├── CommonUtil.ts、TypeUitls.ts
    ├── Event/EventBus.ts      # 类型安全事件总线（v1.6.2）
    ├── Log/Log.ts、LogType.ts
    ├── Mixin/MixinUtils.ts
    └── UISingle/UISingle.ts、UISingleType.ts

Typing/                      # 顶层类型声明（cpp / ffi / puerts / ue）
```

> 初版此处把 `Typing/` 画在 `TypeScript/` 内部；实际它是与 `TypeScript/` 平级的独立目录。

### 启动流程

1. `CorStart.ts` 是 Puerts 的入口模块
2. UIManager 监听 UCORUIManager 的 `OnUIManagerInit` 事件
3. 初始化完成后，通过 `PushUI` 打开启动界面
4. 各 Manager 类按依赖关系初始化

---

## 5. 数据流与交互机制

### UI 窗口生命周期

```
PushUI
  ↓
  NativePushUI
    ├── 检查 RootCanvas / WindowClass / CDO 有效性
    ├── 从 WidgetPool 创建或复用窗口实例
    ├── 窗口添加到 CanvasContainer.PushUI
    │   └── 设置 ZOrder、Slot 参数、入栈
    ├── 窗口 NativeOpen
    │   └── 设置 Collapsed 不可见（等待状态更新后再显示）
    └── NativeUpdateUI
        └── 从高到低遍历所有层级 → UpdateWindowState
            └── 栈顶到栈底传播状态 → NativeUpdateState
                ├── 上层显示/覆盖 → 当前层 Covered
                ├── 上层要求隐藏 → 当前层 Hidden
                └── 无上层/上层不干预 → 当前层 Shown
```

### C++ ↔ TypeScript 数据流

```
TypeScript (CorStart.ts)
    │ 通过 Puerts 绑定调用 C++ API
    │ PushUI / PopUI / GetLayer / GetLayerZOrder
    ▼
C++ UCORUIManager
    │ 广播事件 → TypeScript 监听
    │ OnUIManagerInit → UIManager.ts 初始化
    │ OnUIManagerDeinit → 清理
    ▼
TypeScript 逻辑（UIManager.ts）
    │ 通过 Mixin 注入蓝图逻辑
    ▼
C++ UCORWindow (蓝图实例)
    │ OnScriptInit / OnOpen / OnClose / ...
    ▼
TypeScript Mixin 类
```

---

## 6. 特点总结

### 6.1 架构优势

1. **分层 UI 管理**：5 层 UI 层级（背景/常规/顶层/消息/调试），ZOrder 松耦合配置，支持灵活调整层叠顺序
2. **TypeScript 全栈开发**：借助 Puerts，UI 逻辑 100% TypeScript 开发，兼顾开发效率与 C++ 性能
3. **窗口状态机完备**：位标志状态设计 (Shown/Hidden/Covered/HiddenUnder)，支持复杂的层叠交互场景
4. **设计器集成**：Mixin 生成器 + DrawCall 预估显示，提升编辑器内开发体验
5. **WidgetPool 对象复用**：利用 `FUserWidgetPool` 减少窗口反复创建销毁的开销
6. **弱引用安全传递**：`WeakObjectWrapper` 避免 TypeScript 侧持有悬挂指针导致崩溃
7. **异步资源加载封装**：`CORStreamableManager` 将 UE 的流式加载适配到 TypeScript 调用

### 6.2 开发工具链

- 提供了完整的 `watch-ts` 批处理脚本，支持 TypeScript 文件变更时自动编译
- PuertsMixinGenerator 自动为蓝图生成 TypeScript Mixin 模板，减少重复编码
- 数据表 JSON 导出工具，方便 TypeScript 消费 UE 数据表内容

### 6.3 扩展性

- CORUI 插件采用 GameInstanceSubsystem 架构，与 UE 生命周期深度融合
- 控件子类体系（CORButton/CORImage/CORTextBlock 等）预留了扩展点
- UI 单例模式封装（UISingle），便于实现全局唯一的 UI 界面管理

---

## 7. 不足之处与改进建议

### 7.1 代码完整性问题

| 问题 | 具体表现 | 影响 | 建议 |
|------|----------|------|------|
| **控件子类为空** | [`CORButton`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORButton.h:13)、[`CORTextBlock`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORTextBlock.h:13)、[`CORProgressBar`](Plugins/CORUI/Source/CORUI/Public/UI/Widget/CORProgressBar.h:13) 均为空子类，无额外实现 | 这些类与 UE 原生控件无区别，增加继承深度却无收益 | 移除或为每个类添加实际功能（如样式统一管理、事件增强等） |
| **Puerts 导出被注释** | [`CORUIManager.cpp:403-425`](Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:403) 的 Puerts 注册代码全部注释 | TypeScript 侧无法直接调用 `PushUI`/`PopUI`，只能通过事件间接通信 | 取消注释或确认是否已通过其他方式导出（如蓝图可调用的 `UFUNCTION`） |
| **重复导出注册** | `CORSoftObjectPath` 的 Puerts 注册在 [`CORSoftObjectPath.cpp:5-25`](Plugins/CORUI/Source/CORUI/Private/Core/Streamable/CORSoftObjectPath.cpp:5)（注释）和 [`AutoRegister.cpp:28-39`](Plugins/CORUI/Source/CORUI/Private/Core/AutoRegister/AutoRegister.cpp:28)（活跃）各有一份 | 两份代码逻辑几乎一致，容易导致维护不一致 | 清理被注释的旧注册代码，统一到 `AutoRegister.cpp` |
| **JsEnvManager.cpp 为空** | [`JsEnvManager.cpp`](Plugins/CORUI/Source/JsEnvManager/Private/JsEnvManager.cpp) 仅有模块启动/关闭的空实现 | 无实际作用，可考虑移除或整合到其他文件 | 移除空文件或添加实际逻辑 |

### 7.2 架构与设计问题

| 问题 | 具体表现 | 影响 | 建议 |
|------|----------|------|------|
| **UI 管理器未直接导出到 TS** | `UCORUIManager` 的 Puerts 注册被注释，TS 侧通过 `UIManager.ts` 调用时可能是蓝图事件桥接 | 额外的 IPC 开销，调用路径不直观 | 恢复 Puerts 导出，让 TS 直接调用 `UCORUIManager` |
| **所有 Manager 均为 GameInstanceSubsystem** | `UCORUIManager`、`UCORStreamableManager`、`UJsEnvManagerSubsystem` 都继承自 GameInstanceSubsystem | 生命周期绑定在 GameInstance，无法在编辑器 PIE 之外独立使用 | 考虑将纯工具类改为普通 UObject，仅保留有状态的管理器为 Subsystem |
| **线程锁的适用性存疑** | `FScopeLock` 用于多处 UI 操作，但 UI 操作通常应在 GameThread 执行 | 可能引入死锁风险，且锁的粒度较大 | 评估是否为过渡设计，考虑使用 `AsyncTask` 或 `GameThread` 调度替代 |
| **缺少 UI 动画系统** | 插件未提供内建动画支持 | UI 显示/隐藏/切换缺乏过渡效果，影响用户体验 | 考虑集成通用补间动画方案 |

### 7.3 健壮性与错误处理

| 问题 | 具体表现 | 影响 | 建议 |
|------|----------|------|------|
| **异步加载无错误恢复** | [`AsyncPushUI`](Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:285) 在加载失败时仅打印错误日志，无重试或回退机制 | 资源加载失败时 UI 完全不可用 | 添加重试逻辑或降级显示默认 UI |
| **StreamableManager 无资源卸载跟踪** | [`CORStreamableManager`](Plugins/CORUI/Source/CORUI/Public/Core/Streamable/CORStreamableManager.h) 只记录 Handle 不记录对应的资源路径 | 无法批量释放特定类型的资源 | 增加资源路径到 Handle 的映射 |
| **PopUI 可能产生悬挂弱指针** | [`WindowsStack`](Plugins/CORUI/Source/CORUI/Private/UI/Container/CORCanvasContainer.cpp:135) 移除窗口实例后，若 WidgetPool 未及时回收可能导致弱指针悬空 | 潜在的内存访问错误 | 添加弱指针有效性检查并增加日志 |
| **空文件名为 NAME_None 时处理不一致** | `PopUI(NAME_None)` 弹出栈顶窗口，而 `FindWindow(NAME_None)` 返回 nullptr | 行为不一致可能引起困惑 | 明确约定或统一行为 |

### 7.4 编辑器工具问题

| 问题 | 具体表现 | 影响 | 建议 |
|------|----------|------|------|
| **DrawCall 预估值精度有限** | [`CalculateDrawCallRecursive`](Plugins/CORUI/Source/PuertsMixinGenerator/Private/PmgDesignerExtension.cpp:312) 仅根据资源和裁剪状态估算 | 数值仅供参考，不能完全代表实际渲染性能 | 标注为"预估"值，考虑集成 UE 的 SLATE 渲染分析工具 |
| **Mixin 生成逻辑硬编码** | 路径拼接 [`GeneratePuertsMixinFile`](Plugins/CORUI/Source/PuertsMixinGenerator/Private/PuertsMixinGenerator.cpp:234) 使用字符串拼接 | 项目目录结构调整可能导致生成路径错误 | 使用 UE 的路径工具函数 |
| **配置文件遗留重定向** | [`DefaultCORUI.ini`](Plugins/CORUI/Config/DefaultCORUI.ini:1) 中有 `LayerConfig → UILayerConfig` 的结构体重定向 | 表明发生过重命名，但相关源文件仍需清理 | 检查是否所有旧名称引用已更新 |

### 7.5 开发体验与文档

| 问题 | 具体表现 | 影响 | 建议 |
|------|----------|------|------|
| **缺少 TypeScript 类型声明** | 插件内无 `.d.ts` 文件为 CORUI 的 C++ 导出提供类型信息 | TS 侧开发无智能提示和类型检查 | 为所有 Puerts 导出的 C++ 类型生成类型声明文件 |
| **Mixin 生成重复检测** | [`GeneratePuertsMixinFile`](Plugins/CORUI/Source/PuertsMixinGenerator/Private/PuertsMixinGenerator.cpp:237) 仅在文件不存在时才生成 | 蓝图更新后 mixin 文件不会同步更新 | 添加基于时间戳或哈希的增量更新检测 |
| **没有单元测试/自动化测试** | 项目中无测试目录或测试用例 | 代码修改缺乏回归保障 | 引入 UE 自动化测试框架 |

---

## 附录 A：类型枚举参考

### EUILayerType（UI 层级）

| 枚举值 | ZOrder 默认值（倍率=100） | 用途 |
|--------|---------------------------|------|
| `None` | 0 | 占位标记 |
| `BackGround` | 100 | 背景层（场景、世界空间 UI） |
| `Normal` | 200 | 常规 UI（主界面、面板） |
| `Top` | 300 | 顶层 UI（弹出面板、二级窗口） |
| `Message` | 400 | 消息层（Toast、提示，不支持关闭） |
| `Debug` | 500 | 调试层（仅开发/调试用） |

### EWindowState（窗口状态位标志）

| 标志 | 值 | 含义 |
|------|-----|------|
| `None` | 0x00 | 初始状态 |
| `Shown` | 0x01 | 窗口已显示 |
| `Hidden` | 0x02 | 窗口被上层要求隐藏 |
| `Covered` | 0x04 | 窗口被上层覆盖 |
| `HiddenUnder` | 0x08 | 标记需要隐藏底部窗口 |

---

## 附录 B：核心调用流程

### B.1 启动顺序

```
UE GameInstance 创建
  ├── UCORUIManager.Initialize()
  │   ├── 读取层级配置
  │   └── 绑定 GameModeInitializedEvent
  ├── UJsEnvManagerSubsystem.Initialize()
  │   ├── 声明依赖 UCORUIManager
  │   └── 绑定 GameModeInitializedEvent
  └── (等待 GameMode 初始化完成)
      
GameMode 初始化完成
  ├── UCORUIManager.OnUIBeginPlay()
  │   └── (延迟一帧) InitUI()
  │       ├── 创建 RootCanvas
  │       ├── 创建层级容器(CORCanvasContainer) × 5
  │       └── 广播 OnUIManagerInit
  └── UJsEnvManagerSubsystem.OnJsEnvBeginPlay()
      └── (延迟一帧) StartJsEnv()
          ├── 创建 puerts::FJsEnv
          ├── (可选) WaitDebugger
          └── 启动 CorStart 模块
              └── UIManager.ts 初始化
                  └── PushUI(UI_GameStart)
```

### B.2 UI 窗口 Push/Pop 流程

```
PushUI(path, callback, name, layer, async)
  ├── AsyncPushUI: StreamableManager.RequestAsyncLoad → NativePushUI
  └── SyncPushUI: TryLoadClass → NativePushUI
      └── NativePushUI
          ├── 验证参数有效
          ├── 从 WidgetPool 获取/创建窗口实例
          ├── 检查同名窗口 → PopUI
          ├── CanvasContainer.PushUI → 注册+入栈+设置ZOrder
          ├── Window.NativeOpen → 设置Collapsed(暂不可见)
          └── NativeUpdateUI → 层级状态刷新

PopUI(name, layer, bPopSelf)
  └── CanvasContainer.PopUI
      ├── FullScreen 类型 → 弹出目标及以上所有窗口
      ├── 普通类型 → 仅弹出目标窗口
      ├── NativeRemoveWindow → Hide + NativeClose + RemoveFromParent
      └── UpdateWindowState(EWindowState::None)
```

---

## 8. v1.1 ~ v1.6.2 演进与现状（2026-11 复核）

### 8.1 版本历程

| 版本 | 主题 |
|------|------|
| v1.1.0 | 升级 Puerts 到 Unreal_v1.0.9；修复控件 `RemoveFromParent` 不析构 |
| v1.2.0 | 整体迁移到 UE 5.8（目标设置、CORUI 兼容、编译与运行验证） |
| v1.3.0 | 恢复 Puerts ThirdParty V8 9.4 运行库（V8 库不入库，见 README） |
| v1.4.0 | 修复 `UI_GameStart` 的 null 赋值类型错误，补充 TS 编译说明 |
| v1.6.2 | 事件总线、ViewModel 层、数据表管线、Travel 实测、F1 所有权断言、DrawCall 面板修正（6 条需求全部归档） |

### 8.2 v1.6.2 新增能力

| 能力 | 位置 | 说明 |
|------|------|------|
| 事件总线 | `TypeScript/Utils/Event/EventBus.ts` + `Game/Events/GameEvents.ts` | 类型安全事件契约；Emit 快照、异常隔离、Once 去重、生命周期解除；`data:table-cached` / `ui:stopped` / `toast:shown` 已接线 |
| ViewModel 层 | `TypeScript/Game/UI/ViewModel/ViewModelBase.ts` | patch 合并 + 变更通知；首帧只渲染一次；显式 `undefined` 不清空字段 |
| 数据表管线 | CORUI C++（JSON/CSV 导入导出、`LoadTableByPath`）+ `Manager/DataManager.ts` | 原子导入、严格字段校验、唯一临时表；TS 侧缓存并广播事件 |
| 调试命令 | `Plugins/CORUI/Source/CORUI` | `CORUI.Travel`（同 GameInstance 换图）、`CORUI.VerifyOwnership`（仅非 Shipping，输出 `RESULT=PASS/FAIL/SKIP`） |
| DrawCall 面板 | `PuertsMixinGenerator` | 分类计数（Image/Text/Border/Box/Effect）、元素/强制断批/扫描/未识别统计、批次键补 layer 与字重、自愈重挂、签名节流、`CORUI.DrawCallOverlay` 开关 |
| 观澜测试套件 | `TypeScript/Game/UI/Test/*` + `Manager/TestUIManager.ts` | `TestUIManager.AutoDemo = true` 打开 `UI_TestHub` 自动跑自检 |

### 8.3 第 7 章问题清单现状

| 原问题（第 7 章） | 现状 | 说明 |
|------|------|------|
| 控件子类为空（CORButton 等） | 仍存在 | 未处理 |
| Puerts 导出被注释 | 仍存在 | `CORUIManager.cpp` 末尾注册仍注释；TS 走事件与 `UFUNCTION` 桥接 |
| 重复导出注册 | 未复核 | — |
| JsEnvManager.cpp 为空 | 仍存在 | 模块仅做启动/关闭 |
| UI 管理器未直接导出到 TS | 仍存在 | 同上 |
| 所有 Manager 均为 GameInstanceSubsystem | 保持设计 | 暂未调整 |
| 线程锁适用性存疑 | 未复核 | — |
| 缺少 UI 动画系统 | 仍缺 | 未集成补间方案 |
| 异步加载无错误恢复 | 仍缺 | 复审记录中列为后续 |
| StreamableManager 无卸载跟踪 | 未复核 | — |
| PopUI 悬挂弱指针 | 未复核 | — |
| **DrawCall 预估值精度有限** | **已修（v1.6.2）** | 分类/断批模型补全，面板明确标注「预估，非实测」；见 REQ-016 |
| Mixin 生成逻辑硬编码 | 未复核 | — |
| 配置遗留重定向 | 未复核 | — |
| **缺少 TypeScript 类型声明** | **已解决** | 顶层 `Typing/`（cpp / ffi / puerts / ue） |
| Mixin 生成重复检测 | 仍存在 | 仍仅文件不存在时生成 |
| **没有单元测试/自动化测试** | **部分** | 观澜 AutoDemo 运行期自检（非 UE Automation，需手工触发） |

### 8.4 架构补充（v1.6.2 之后）

- **UI 会话拆建**：同一 GameInstance 内换图时，UI 会话成对拆建、JS 环境保持单实例、数据表在新世界重新缓存（`CORUI.Travel` 实测）
- **F1 所有权**：`RootCanvas` / 层容器 / 池实例改为 `UPROPERTY` 持有；`CORUI.VerifyOwnership` 用不可达探针自证 GC 有效后断言存活（判定 6 项 + 2 项 INFO）
- **数据流新增一层**：C++ → TS 事件、TS → TS 事件总线，UI 之间不再直接互相调用
- **流程治理**：`.openfeel/` 承载需求、阶段、审查（REV）与版本归档；v1.6.2 已归档

### 8.5 构建与验证入口

```bash
# 编译
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" \
  CultivationOpenRealmEditor Win64 Development -Project="...\CultivationOpenRealm.uproject" -WaitMutex

# 运行期验证（PIE 中执行）
CORUI.VerifyOwnership     # 所有权断言，看 [CORUI][Owner] RESULT=
CORUI.Travel /Game/Map/TestMap   # UI 会话拆建验证
```

---

*本文档 2026-04-28 初版基于项目代码分析生成；2026-11-15 补充第 8 章并复核第 7 章现状。
未复核项表示本轮没有重新验证，不代表问题已消失。*
