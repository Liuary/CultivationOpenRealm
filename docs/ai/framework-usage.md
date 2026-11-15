# CORUI 框架使用手册（AI / 开发者通用）

> 面向需要**修改或使用**这套框架的 AI 与开发者。入口速查见 [AGENTS.md](../../AGENTS.md)，项目概况见 [README.md](../../README.md)。
> 本文所有签名均来自当前代码/target 生成的 `.d.ts`，改动代码后请同步本文件。

---

## 1. 分层与职责

```
UMG 蓝图 (.uasset)      ← 视觉、控件树、动画
      ↑ blueprint.mixin()
TypeScript (TypeScript/) ← 业务逻辑：Manager / Mixin / EventBus / ViewModel / DataManager
      ↑ UFUNCTION 自动绑定 + AutoRegister.cpp
C++ (Plugins/CORUI)      ← 层级、窗口栈、对象池、异步加载、数据表、编辑器工具
```

**修改原则**：

- 加 UI 逻辑 → 改 TS（`TypeScript/Game/UI/**`），不要动 C++
- 加框架能力（层级/窗口/资源/数据）→ 改 C++
- 加 UMG 视觉 → 只能通过编辑器改 `.uasset`
- 加导出给 TS 的类型 → `AutoRegister.cpp`，不要手改 `Typing/ue/ue.d.ts`

---

## 2. 生命周期时序

### 2.1 打开窗口（异步）

```
UIManager.Instance.PushUI(类路径, UISingle, 窗口名, 层级, true)
  └─ UCORUIManager::PushUI
      ├─ StreamableManager 异步加载类
      ├─ WidgetPool 取/建窗口实例
      ├─ CanvasContainer.PushUI → 注册 + 入栈 + 设 ZOrder
      ├─ 回调 TS：OnScriptInit(UISingle)        ← 数据初始化在这里
      │    └─ 抛异常 → UIManager 自动 CloseUI 回滚
      ├─ NativeOpen() → OnOpen()
      └─ NativeUpdateUI() → 逐窗口算状态 → OnShow()/OnHide()
```

### 2.2 关闭窗口

```
UIManager.Instance.PopUI(窗口名, 层级, bPopSelf)
  └─ UCORCanvasContainer::PopUI
      ├─ 锁内快照 + DetachWindow（栈/映射同时移除）
      ├─ 锁外 ReleaseWindows → 归还池 → NativeDestruct → Destruct()
      └─ UpdateWindowState → 下层窗口 OnShow() 恢复
```

### 2.3 状态机（UCORWindow 位标志）

| 状态 | 含义 |
|---|---|
| `Shown` | 正常显示 |
| `Hidden` | 被上层要求隐藏 |
| `Covered` | 被上层覆盖 |
| `HiddenUnder` | 需要隐藏底部窗口 |

---

## 3. C++ API 参考

### 3.1 `UCORUIManager`（GameInstance 子系统）

| API | 说明 |
|---|---|
| `PushUI(FString WindowClassPath, FJsObject Callback, FName WindowName, EUILayerType PushLayer, bool bIsAsync)` | 加窗口；回调 `(bool Success, FName Name, UCORWindow*)`，**exactly-once** |
| `PopUI(FName WindowName, EUILayerType PushLayer, bool bPopSelf = true)` | 移窗口；`NAME_None` 弹栈顶；FullScreen 支持"弹自己+上方" |
| `GetLayer(EUILayerType)` | 取层级容器 |
| `GetLayerTypes()` | 全部层级枚举 |
| `GetLayerZOrder(EUILayerType)` | 层级 ZOrder |
| `IsUIInitialized()` | UI 是否已就绪（JS 晚订阅时用） |
| `OnUIManagerInit` / `OnUIManagerDeinit` | 蓝图/TS 可绑的委托 |

> TS 侧直接 `UE.CORUIManager` 调用以上 `UFUNCTION`（自动绑定），不需要手动导出。

### 3.2 `UCORWindow` / `UCORUserWidget`

| 成员 | 说明 |
|---|---|
| `GetWindowName()` / `SetWindowName()` | 窗口名（默认取 CDO） |
| `SetLayer()` / `SetWindowZOrder()` | 层与 ZOrder |
| `CloseUI(bool bCloseSelf = true)` | 关闭自己（或自己及以上） |
| `OnCover` | 被覆盖时的蓝图事件 |
| 生命周期钩子 | `OnOpen/OnClose/OnShow/OnHide`（UCORUserWidget）、`OnScriptInit`（UCORWindow） |

### 3.3 `UCORStreamableManager`

| API | 说明 |
|---|---|
| `RequestAsyncLoad(CORSoftObjectPath, FJsObject Callback)` | 单资源异步加载，返回句柄 ID |
| `RequestAsyncLoadList(TArray<CORSoftObjectPath>, FJsObject)` | 批量加载 |
| `ReleaseHandle(int32)` / `CancelHandle(int32)` / `ReleaseAll()` | 释放 / 取消 / 全释放 |

### 3.4 `UCORDataLibrary`（配置表工具）

| API | 说明 |
|---|---|
| `GetTableRowAsJson(Table, RowName)` | 单行转 JSON |
| `GetEntireTableAsJson(Table)` | 整表转 JSON |
| `ImportTableFromJson(Table, Json)` | 导入（两阶段校验 + 失败回滚） |
| `ImportTableFromCsv(Table, Csv)` | 导入 CSV |
| `ExportTableToJson(Table)` / `ExportTableToCsv(Table)` | 导出（CSV 仅编辑器有效） |
| `CreateTableFromTemplate(Template, Name)` | 建 transient 表（往返校验用） |

---

## 4. TypeScript API 参考

### 4.1 `UIManager`（单例）

```ts
PushUI(WindowClassPath: string, Single?: UISingle, WindowName?: string,
       PushLayer?: UE.EUILayerType, bIsAsync?: boolean): void
PopUI(WindowName: string, PushLayer: UE.EUILayerType, bPopSelf?: boolean): void
```

约定：

- `WindowClassPath` 是**类路径**（`..._C` 结尾）
- 省略窗口名时内部传 `"None"`（避免 Puerts 把 `undefined` 变字符串）
- `OnScriptInit` 抛异常会回滚关闭

### 4.2 `DataManager`（单例）

```ts
LoadTableByPath(DataPath: string, CacheKey?: string): boolean
CacheTable(CacheKey: string, Table: UE.DataTable): void
GetRow<T>(CacheKey: string, RowName: string): PureData<T> | undefined
GetAllRows<T>(CacheKey: string): PureData<T>[]
GetRowNames(CacheKey: string): string[]
```

- 缓存形态：`{ 行名: 行数据 }`
- 缓存后广播 `data:table-cached { key, rows }`

### 4.3 `EventBus` / `GameEvents`

```ts
GameEvents.On(event, handler): () => void   // 返回取消函数
GameEvents.Once(event, handler): () => void
GameEvents.Off(event, handler): void
GameEvents.Emit(event, payload): void
GameEvents.Clear(event?): void
```

内置事件（`GameEventMap`）：`ui:ready`、`ui:stopped`、`data:table-cached`、`toast:shown`。

加事件 = 改 `GameEvents.ts` 的 `GameEventMap`。

### 4.4 `ViewModelBase`

```ts
GetState(): Readonly<TState>
SetState(Patch: Partial<TState>): boolean   // 有变化返回 true，undefined 跳过
Subscribe(handler): () => void
Dispose(): void
```

### 4.5 `UISingle`

```ts
SetSingle(Type: UISingleType, Value: any): void
GetSingle<T>(Type: UISingleType): T
```

`UISingleType`：`None / Id / Name / List / Flag_0..Flag_9`。

### 4.6 `Log`

```ts
Log.Log(LogType.UI, "...");
Log.Error(LogType.Game, "...");
```

日志进入 UE 输出日志，可用 `[CORUI]`、`[UIManager]` 过滤。

---

## 5. Mixin 机制

生成器（`PuertsMixinGenerator`）为蓝图产出：

```ts
// 《生成标志，勿删》
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from "../../../Utils/UISingle/UISingle";

const BP_UI_TestData_Class = UE.Class.Load('/Game/UI/TestSuite/UI_TestData.UI_TestData');
const BP_UI_TestData = blueprint.tojs<typeof UE.Game.UI.TestSuite.UI_TestData>(BP_UI_TestData_Class);
interface UI_TestData extends UE.Game.UI.TestSuite.UI_TestData {}

class UI_TestData {
    OnScriptInit(Single: UISingle) {}
    OnOpen() {}
    OnClose() {}
    OnShow() {}
    OnHide() {}
}

blueprint.mixin(BP_UI_TestData, UI_TestData);
```

规则：

- 文件头生成标志不能删（Import 列表靠它识别）
- 类名/接口名与文件名一致
- 蓝图改了要**删旧文件再重新生成**（工具不覆盖已存在文件）
- Mixin 的 `constructor` 不会执行，初始化放 `OnScriptInit`/`Construct`

---

## 6. 调试

| 命令 | 说明 |
|---|---|
| `CORUI.Travel <LevelPath>` | 同 GameInstance 换图（仅非 Shipping） |
| `CORUI.VerifyOwnership` | GC 后所有权断言，`RESULT=PASS/FAIL/SKIP` |
| `CORUI.DrawCallOverlay 0/1` | 设计器 DrawCall 面板开关（编辑器） |

日志：`Saved/Logs/CultivationOpenRealm.log`

| 关键字 | 含义 |
|---|---|
| `[CORUI][Owner]` | 所有权断言明细 |
| `[PmgDesigner] dc >>` | 面板统计刷新（节流后仅变化时打印） |
| `[GameManager][Event] data:table-cached` | 配置表缓存完成 |
| `[UIManager][PushUI]` | 窗口打开/回滚 |

---

## 7. 完整示例：一个读表并展示的窗口

```ts
// TypeScript/Game/UI/Demo/UI_Demo.ts（由生成器生成后补全）
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from "../../Utils/UISingle/UISingle";
import UISingleType from "../../Utils/UISingle/UISingleType";
import DataManager from "../../Manager/DataManager";
import ViewModelBase from "../ViewModel/ViewModelBase";
import { GameEvents } from "../../Game/Events/GameEvents";

interface DemoState { Title: string; Count: number; }

class UI_DemoVM extends ViewModelBase<DemoState> {
    constructor() { super({ Title: "", Count: 0 }); }
}

class UI_Demo {
    private VM = new UI_DemoVM();
    private ItemId = 0;
    private Off: () => void = null;

    OnScriptInit(Single: UISingle) {
        this.ItemId = Single.GetSingle<number>(UISingleType.Id) ?? 0;

        const Key = "DT_GameStart";
        if (DataManager.Instance.LoadTableByPath("/Game/Data/DT_GameStart.DT_GameStart", Key)) {
            const Row = DataManager.Instance.GetRow<any>(Key, `Row${this.ItemId}`);
            this.VM.SetState({ Title: Row?.Desc ?? "", Count: DataManager.Instance.GetAllRows<any>(Key).length });
        }

        this.Off = GameEvents.On('data:table-cached', ({ key, rows }) => {
            if (key === Key) this.VM.SetState({ Count: rows });
        });

        this.VM.Subscribe((s) => {
            (this as any).Txt_Title?.SetText?.(s.Title);
        });
    }

    OnClose() {
        this.Off?.();
        this.VM.Dispose();
    }
}

blueprint.mixin(BP_UI_Demo, UI_Demo);
```

---

## 8. 故障排查

| 现象 | 可能原因 | 处理 |
|---|---|---|
| 编译报找不到 V8 库 | 缺 `v8_9.4.146.24/{Inc,Lib,Bin}` | 按 README 下载解压 |
| 链接失败/文件被占用 | 编辑器开着 | 关编辑器再 build |
| TS 类型报找不到 `UE.CORUIManager` | `Typing/ue` 未生成 | 用 Puerts 重新生成类型（或看 `.d.ts` 是否旧） |
| 窗口打开了但没脚本逻辑 | Mixin 未生成 / Import 列表没刷新 / 未 `npm run build` | 重新生成 + 刷新 + 编译 |
| `OnScriptInit` 里取不到数据 | 传参时没 `SetSingle`，或 key 不一致 | 检查 `UISingleType` |
| `GetRow` 返回 undefined | 行名写错 / 表没加载成功 | 先 `GetRowNames` 看行名 |
| 面板不显示 | CVar 关了 / 设计器未就绪 | `CORUI.DrawCallOverlay 1`，看 `[PmgDesigner]` 日志 |
| 换图后 UI 不重建 | 未走 `CORUI.Travel` 或会话逻辑改坏 | `CORUI.VerifyOwnership` + 查 `UISessionGeneration` |

---

## 9. 与 OpenFeel 流程的关系

- `.openfeel/flow.json` 是唯一权威状态，**不要手改**
- 需求（REQ）→ 阶段（stage）→ 审查（REV）→ 版本归档
- 改动代码前确认阶段处于 `exec_running`（否则写入门禁会拒绝文件写入）
- 归档判据：版本内全部阶段 done + 无 blocking REV
