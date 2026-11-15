# AGENTS.md — 给 AI 编码助手的工程说明

> 本文件写给 **AI 编码助手**（Claude / GPT / Cursor / Copilot / DSH 等）。
> 人类开发入门见 [README.md](./README.md)，框架 API 详解见 [docs/ai/framework-usage.md](./docs/ai/framework-usage.md)。

## 0. 这是什么

**CultivationOpenRealm**：UE 5.8 + Puerts(TypeScript) 的修真题材项目，核心产出是一套可复用的 **CORUI UI 框架**。

分工：

| 层 | 语言 | 负责 |
|---|---|---|
| 视觉 | UMG 蓝图（`.uasset`） | 布局、控件、动画 |
| 逻辑 | TypeScript（`TypeScript/`） | 管理器、UI Mixin、事件、数据 |
| 能力 | C++（`Plugins/CORUI`） | 层级、窗口栈、资源加载、数据表、编辑器工具 |

## 1. 先决条件（不满足则一定失败）

1. **V8 预编译库不在仓库里**，必须在本地存在：
   `Plugins/Puerts/ThirdParty/v8_9.4.146.24/{Inc,Lib,Bin}`
   缺失时编译报找不到 V8 库，下载方式见 README「首次克隆必读」。
2. 编译前**关闭 UnrealEditor**（否则文件被锁）：
   ```bash
   "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" \
     CultivationOpenRealmEditor Win64 Development \
     -Project="C:\Users\Liuary\Dev\Mine\Unreal\CultivationOpenRealm\CultivationOpenRealm.uproject" -WaitMutex
   ```
3. TypeScript 编译：
   ```bash
   npm install
   npm run build        # 产物 → Content/JavaScript/
   npm run typecheck    # 只做类型检查
   ```
4. 别用 `git commit` 之外的方式改仓库状态（见第 7 节）。

## 2. 代码地图（先看这里再动手）

| 路径 | 内容 | 什么时候改 |
|---|---|---|
| `TypeScript/Manager/UIManager.ts` | TS 侧窗口开关入口 | 改 UI 打开方式 |
| `TypeScript/Manager/DataManager.ts` | 配置表加载/缓存/取行 | 改数据读取 |
| `TypeScript/Utils/Event/EventBus.ts` | 事件总线 | 加事件能力 |
| `TypeScript/Game/Events/GameEvents.ts` | 事件契约表 | **加事件必须改这里** |
| `TypeScript/Game/UI/ViewModel/ViewModelBase.ts` | ViewModel 基类 | 改状态驱动 |
| `TypeScript/Utils/UISingle/UISingle.ts` | UI 间传参包 | 传参约定 |
| `TypeScript/Game/UI/**` | 各窗口 Mixin | 写 UI 逻辑 |
| `Plugins/CORUI/Source/CORUI/` | C++：UIManager/Window/Container/Streamable/DataLibrary | 改框架能力 |
| `Plugins/CORUI/Source/PuertsMixinGenerator/` | 编辑器：Mixin 生成、DrawCall 面板 | 改编辑器工具 |
| `Plugins/CORUI/Source/CORUI/Private/Core/AutoRegister/AutoRegister.cpp` | Puerts 手动导出 | **新导出加这里** |
| `Typing/` | 自动生成的 `.d.ts` | 只读，不要手改 |
| `Content/` | UMG 蓝图、数据表、地图 | 只通过编辑器改 |

## 3. 核心概念（动手前必须理解）

- **层级**：`EUILayerType` = `None / BackGround / Normal / Top / Message / Debug`，每层一个 `UCORCanvasContainer`，ZOrder 可配（`DA_UILayerTypeZOrderConfigData`）
- **窗口**：`UCORWindow`（继承 `UCORUserWidget` → `UUserWidget`），位标志状态机，Push 入栈、Pop 出栈，池化复用
- **打开方式**：TS 侧 `UIManager.Instance.PushUI(...)` → C++ `PushUI`（同步/异步）
- **Mixin**：TS 类通过 `blueprint.mixin()` 注入蓝图；生命周期由 C++ 回调（见第 4.1 节）
- **传参**：`UISingle`（类型化键值包），不要用全局变量
- **事件**：`EventBus` + `GameEvents` 契约，UI 之间不直接互相调用
- **数据**：`DataManager` 按路径加载 DataTable，缓存为「行名 → 行数据」

## 4. 常见任务配方

### 4.1 新增一个 UI 窗口

1. UMG 里建控件蓝图，父类选 `UCORWindow`（需要 Push/Pop 语义）或 `UCORUserWidget`
2. 蓝图编辑器工具栏点 **PuertsMixinGenerator** 生成 Mixin（生成 `TypeScript/Game/.../<Name>.ts` 并刷新 `PuertsImportList.ts`）
3. 在生成的类里实现钩子（**不要改类名/文件头生成标志**）：

```ts
OnScriptInit(Single: UISingle) {}   // UCORWindow：最先执行，用于数据初始化
Construct() {}                      // UUserWidget：构造
Destruct() {}                       // UUserWidget：析构
OnOpen() {}                         // UCORUserWidget：真正打开
OnClose() {}                        // UCORUserWidget：关闭
OnShow() {}                         // 被重新显示（从被遮盖/隐藏恢复）
OnHide() {}                         // 被上层遮盖或隐藏
```

4. `npm run build` 后进 PIE 验证

### 4.2 打开 / 关闭窗口

```ts
import UIManager from '../Manager/UIManager';
import UISingle from '../Utils/UISingle/UISingle';
import UISingleType from '../Utils/UISingle/UISingleType';

const Single = new UISingle();
Single.SetSingle(UISingleType.Id, 1001);

// 第 1 参是【类路径】，必须带 _C 后缀
UIManager.Instance.PushUI(
    "/Game/UI/TestSuite/UI_TestData.UI_TestData_C",
    Single,
    "MyTestData",              // 窗口名（省略/传 "None" 时用 CDO 默认名）
    UE.EUILayerType.Normal,
    true                       // 异步加载
);

UIManager.Instance.PopUI("MyTestData", UE.EUILayerType.Normal, true);
```

### 4.3 UI 间传参（UISingle）

```ts
// 打开方
const Single = new UISingle();
Single.SetSingle(UISingleType.Id, 42);
Single.SetSingle(UISingleType.Name, "item_sword");

// 接收方（OnScriptInit 里）
OnScriptInit(Single: UISingle) {
    this.ItemId = Single.GetSingle<number>(UISingleType.Id);
}
```

### 4.4 读配置表

```ts
import DataManager from '../Manager/DataManager';

const Key = "DT_GameStart";
if (!DataManager.Instance.LoadTableByPath("/Game/Data/DT_GameStart.DT_GameStart", Key)) {
    return; // 加载失败
}
const Row = DataManager.Instance.GetRow<UE.RowStructGameStart>(Key, "Row1");
const All = DataManager.Instance.GetAllRows<UE.RowStructGameStart>(Key);
const Names = DataManager.Instance.GetRowNames(Key);
```

- 缓存后会自动广播 `data:table-cached` 事件
- 导入/导出 JSON、CSV 用 C++ 的 `UCORDataLibrary`（蓝图可调）

### 4.5 发事件 / 监听

```ts
import { GameEvents } from '../Game/Events/GameEvents';

const Off = GameEvents.On('data:table-cached', ({ key, rows }) => { /* ... */ });
Off(); // 取消订阅（On/Once 都返回取消函数）

GameEvents.Emit('data:table-cached', { key: "DT_X", rows: 2 });
```

**加新事件**：先改 `GameEvents.ts` 的 `GameEventMap`（接口名 + payload 类型），否则 `Emit/On` 会类型报错。

### 4.6 ViewModel 驱动 UI

```ts
import ViewModelBase from '../ViewModel/ViewModelBase';

interface MyState { count: number; title: string; }

class MyVM extends ViewModelBase<MyState> {
    constructor() { super({ count: 0, title: "" }); }
}

const VM = new MyVM();
const Off = VM.Subscribe((state) => { /* 更新控件 */ });
VM.SetState({ count: 1 });   // patch 合并，返回是否变化
VM.Dispose();
```

### 4.7 加调试命令

在 `Plugins/CORUI/Source/CORUI/Private/CORUI.cpp` 的 `StartupModule` 里，用 `FAutoConsoleCommand` 注册，**必须包在 `#if !UE_BUILD_SHIPPING` 内**（实现也要包）。参考现有 `CORUI.Travel` / `CORUI.VerifyOwnership`。

## 5. 硬约定与陷阱（AI 最容易踩）

1. **PushUI 第一参数是类路径**：`/Game/.../UI_X.UI_X_C`，不是资产路径 `/Game/.../UI_X`
2. **TS 传窗口名要显式给**：传 `undefined` 会被 Puerts 转成字符串；省略时用 `"None"` 让 C++ 回退 CDO 默认名
3. **`OnScriptInit` 先于 `OnOpen`**：数据初始化放这里；它在 C++ 回调里执行，此时窗口尚未 Open
4. **`OnScriptInit` 抛异常会自动回滚关闭窗口**，不要在里面做不可逆操作
5. **Mixin 文件由生成器产出**：手改生成标志/类名会导致 Import 列表失效；蓝图改了要重新生成（工具只在文件不存在时才生成，需先删旧文件）
6. **`PuertsImportList.ts` 是自动生成的**，不要手编
7. **文案不要写死在 TS**：UE 本地化采集不到 TS 字符串；文案放 String Table / DataTable（`FText`）
8. **配置表缓存是「行名 → 行数据」**，`GetRow` 按行名取，不是按下标
9. **不要手动恢复 `CORUIManager.cpp` 里那段注释的 Puerts 导出**：`UCORUIManager` 已由 UCLASS+UFUNCTION 自动导出；旧块类型名过期，取消注释会编译失败
10. **`Typing/ue/ue.d.ts` 是生成的**，不要改；要新增导出请改 `AutoRegister.cpp`
11. **Windows 编译需关编辑器**，否则链接失败
12. **`.uasset/.umap` 是二进制**，不要手写/手改，只能通过编辑器或脚本生成

## 6. 验证清单（改完必做）

1. **C++**：`Build.bat ... Result: Succeeded`
2. **TS**：`npm run typecheck`（或 `npm run build`）exit 0
3. **运行**：PIE 里跑目标流程，看日志
   - `Saved/Logs/CultivationOpenRealm.log`
   - 关键前缀：`[CORUI]`、`[CORUI][Owner]`、`[PmgDesigner]`、`[Game]`、`[UIManager]`
4. **框架自检**（可选）：
   - `CORUI.VerifyOwnership` → `RESULT=PASS/FAIL/SKIP`
   - `CORUI.Travel /Game/Map/TestMap` → 验证换图后 UI 会话重建
   - `TestUIManager.AutoDemo = true` + 打开 `UI_TestHub` → 观澜自检

## 7. 不要做的事

- 不要手改 `.openfeel/flow.json`、`.openfeel/plan/**/status.md`（流程状态必须走 OpenFeel 工具）
- 不要直接编辑 `Content/**/*.uasset`（二进制）
- 不要删除或提交 `Plugins/Puerts/ThirdParty/v8_*/Lib|Bin`（本地库，已 gitignore）
- 不要把调试命令做成 Shipping 可用（会带进发行包）
- 不要在 UI 里直接 new 另一个 Manager 实例（全部走单例 `Instance`）

## 8. 相关文档

- [README.md](./README.md)：项目概况、V8 库下载、构建启动
- [docs/ai/framework-usage.md](./docs/ai/framework-usage.md)：框架 API 详解、生命周期、排障
- [docs/ai/port-to-new-project.md](./docs/ai/port-to-new-project.md)：**把框架移植到别的项目**（含可直接发给 AI 的任务书）
- [开发文档_CORUI插件分析.md](./开发文档_CORUI插件分析.md)：C++ 内部结构分析、问题清单与现状
- `docs/design/`：UI 设计稿；`docs/reviews/`：评审记录；`docs/issues/`：问题清单
- `.openfeel/`：需求/阶段/审查/归档（OpenFeel 流程治理）
