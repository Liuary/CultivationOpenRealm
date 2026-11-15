# CORUI 审查修复计划（按优先级）

> 来源：[corui-review-2026-11-05.md](corui-review-2026-11-05.md)（Codex `gpt-6.1-sol` 只读审查，245 行）
> 结论：无 P0；**6 个 P1、7 个 P2、1 个 P3**。
> 目标：全部处理；按波次推进；每波独立构建/验证；OpenFeel 自动推进；版本归档仍需用户确认。

---

## 0. 排序原则

1. **P1 先于 P2/P3**。
2. 同级别按「会放大其他问题」的程度排序：
   - 所有权/GC（F1）是基础，先定；
   - 栈/激活事务（F3）影响所有 Push/Pop/覆层；
   - 启动/关闭顺序（F4/F5）影响所有生命周期事件；
   - 异步与会话（F2/F6）影响加载与旧请求；
   - 编辑器工具（F11/F12）与 TS 契约（F13/F14）收尾。
3. 每波尽量**独立可构建、可回滚**，避免一次性大改。

---

## 1. 波次总览

| 波次 | 内容 | 发现 | 风险 | 预计 |
|---|---|---|---|---|
| W1 | 根/池的 GC 所有权 | F1 | P1 | 小 |
| W2 | 栈/激活事务与重入 | F3、F7、F8 | P1 | 中 |
| W3 | 启动/关闭与 JS 生命周期边界 | F4、F5 | P1 | 中 |
| W4 | 异步 exactly-once 与会话取消 | F2、F6、F9、F10 | P1/P2 | 中大 |
| W5 | 编辑器扩展与 Mixin 生成健壮性 | F11、F12 | P2 | 中 |
| W6 | TS 初始化契约与调试面板 | F13、F14 | P2/P3 | 小 |

---

## 2. 逐项说明

### F1（P1）根 UI 没有 GC 可见的所有权

- **位置**：`Plugins/CORUI/Source/CORUI/Public/CORUIManager.h:42`、`Private/CORUIManager.cpp:126`
- **问题**：动态创建的根 `UCanvasPanel` 只由 `TWeakObjectPtr` 持有，层容器也是弱引用；`FUserWidgetPool` 没有显式引用收集。viewport 持有 Slate 控件不等于 UObject GC 引用。
- **修复**：
  - 用 transient `UPROPERTY(TObjectPtr<UCanvasPanel>) RootCanvas` 强持有；
  - 为 `FUserWidgetPool` 实现/接入 `AddReferencedObjects`（或把活跃/池内 widget 暴露为 UPROPERTY 数组）；
  - 明确 `ClearUI` 的所有权释放顺序。
- **测试**：打开窗口 → 强制多次 GC → Push/Pop/Clear → 根/容器/活跃/池内 widget 均有效，直到显式 Teardown。
- **验收**：GC 后无失效根；`UCORUIManager` 可安全重初始化。

### F2（P1）异步 Push 把 pending 当失败 / 终止通知不一致

- **位置**：`Private/CORUIManager.cpp:334`、`:327`、`:347`
- **问题**：`RequestAsyncLoad` 后立刻用 `!HasLoadCompleted()` 判失败并回调；随后正常完成又成功回调一次。`ResolveClass` 失败与 `SyncPushUI` 失败均静默返回。
- **修复**：
  - 定义 exactly-once 终止契约：请求被拒绝（失败）与 pending（继续）分开；
  - 类解析/类型错误走统一失败回调；
  - sync/async 共用同一完成路径。
- **测试**：冷加载、缓存加载、缺资产、错类型、坏路径、空路径；每个请求**恰好一次**终止回调。
- **验收**：无“pending 即失败”，无重复回调。

### F3（P1）生命周期回调重入破坏栈遍历/推入事务

- **位置**：`Private/UI/Container/CORCanvasContainer.cpp:296`、`Private/CORUIManager.cpp:303`
- **问题**：`UpdateWindowState` 边索引活 `WindowsStack` 边调用可重写的 `OnCover/OnHide/OnShow`；回调里同步 Push/Pop 会导致索引失效、重复释放。`NativePushUI` 在回调已关闭/替换窗口后仍报成功。`DetachWindow` 在 Slate 脱钩完成前就发 `NativeClose`。
- **修复**：
  - 遍历稳定快照（复制/代际）；回调后重新校验成员；
  - 回调期间把变更入队，分发给出口统一执行；
  - Push 事务完成后再对外报告成功；若回调已关闭该激活，返回失败/已关闭；
  - 先完成结构脱钩，再发关闭通知（或明确两阶段语义）。
- **测试**：`OnOpen/OnShow/OnHide/OnCover/OnClose` 各自关闭自身、移除下方窗口、推入替代窗口；断言无非法索引、无重复释放、已关闭激活不再成功回调。

### F4（P1）UI 与 JS 启动依赖竞争的 next-tick

- **位置**：`Private/CORUIManager.cpp:103`、`Plugins/CORUI/Source/JsEnvManager/Private/JsEnvManagerSubsystem.cpp:74`
- **问题**：两者各自 `SetTimerForNextTick`；UI 先初始化时，JS 在 `CorStart.Start()`/`BindCppEvent()` 之后才订阅，错过唯一一次 `OnUIManagerInit`。
- **修复**：
  - 显式编排：先启动 JS、绑定监听，再初始化 UI；
  - 或提供幂等 readiness 查询/订阅，已就绪时立即回调；
  - `OnUIManagerInit` 支持“晚订阅立即触发”。
- **测试**：强制两种启动顺序；每个 TS manager 恰好初始化一次。

### F5（P1）关闭/换世界缺少统一生命周期边界

- **位置**：`JsEnvManagerSubsystem.cpp:25`、`Private/CORUIManager.cpp:107`
- **问题**：JsEnvManager 自行 reset；CORUI 另行广播 deinit/销毁 widget，顺序无保证；重复 GameMode 初始化会重复 InitUI/StartJsEnv；`UIEndPlayHandle` 声明但未注册；`InitUI` 不清旧 root。
- **修复**：
  - 定义幂等 session 状态机（Created/Starting/Ready/Stopping/Stopped）；
  - 关闭顺序：拒绝新工作 → 失效在途请求 → 在 JS 存活时关窗口 → 释放 UI → 销毁 JS；
  - 处理重复初始化与 world travel；注册 world end。
- **测试**：PIE 反复启停、单 GameInstance 切图；一个 root/env、生命周期事件平衡、无旧 viewport 残留、无退役会话回调。

### F6（P1）在途加载不绑定当前 UI 会话

- **位置**：`Private/CORUIManager.cpp:318`、`:321`
- **问题**：异步句柄本地持有，`ClearUI` 不跟踪/取消；旧会话请求可能在新 root 上弹旧窗口；`IsValid(this)` 分支为空。
- **修复**：跟踪 pending handle + session/request generation；Teardown 时取消/失效；完成前校验 generation 与 UI session。
- **测试**：延迟加载 → 销毁/重建 UI → 旧请求完成；断言不会在新会话创建窗口。

### F7（P2）重复 Push 保留型 FullScreen 会重复入栈；bPopSelf=false 偏移可疑

- **位置**：`CORUIManager.cpp:268`、`CORCanvasContainer.cpp:38`、`:98`
- **问题**：重复 Push 对 FullScreen 用 `bPopSelf=false` 弹出，容器保留目标；管理器复用后无条件 `Push`，栈出现重复条目。FullScreen 弹出用 `WindowIndex + 2`，保留紧邻上方窗口，与公开语义不符。
- **修复**：定义重复 Push 语义（移动已有条目 vs 重新入栈）；每次激活只保留一个栈条目；把 `+2` 改为显式契约（列出并测试）。
- **测试**：FullScreen A → B/C → 反复重推 A；唯一栈成员；两种 `bPopSelf` 的存活集合符合契约。

### F8（P2）插入失败后池内 widget 未归还

- **位置**：`CORUIManager.cpp:281`、`:296`
- **问题**：从池取出 widget 后，容器插入失败直接返回，未 Release；失败激活残留。
- **修复**：先校验标识再取池；所有插入失败路径归还；失败不得对外表现为可用的激活。
- **测试**：空有效名、slot 插入失败；池计数/生命周期回到请求前。

### F9（P2）Streamable 句柄释放未暴露到 JS；无效句柄返回 ID

- **位置**：`Core/AutoRegister/AutoRegister.cpp:55`、`Public/Core/Streamable/CORStreamableManager.h:46`、`Private/.../CORStreamableManager.cpp:73`
- **问题**：JS 只有 `RequestAsyncLoad/List`，没有 `ReleaseHandle`；无效句柄也返回 ID；只有句柄有效时才移除 map 条目。
- **修复**：暴露 Release/Cancel；拒绝失败请求；无论句柄是否有效都移除条目；返回结构化完成状态。
- **测试**：TS 反复请求/释放（含无效路径、空列表）；句柄 map 有界；区分成功/失败/取消。

### F10（P2）TS 延迟回调活过激活

- **位置**：`TypeScript/Game/UI/GameLoading/UI_GameLoading.ts:132`、`TypeScript/Game/UI/Test/UI_TestData.ts:92`
- **问题**：未跟踪的 timeout 在 Close/Destruct 不取消；池复用后 UObject 仍有效但代表新激活，旧回调会误操作新激活。
- **修复**：Close/Destruct 取消所有激活私有定时器；延迟回调校验 activation generation。
- **测试**：调度回调 → 关闭并立刻重开同类窗口 → 推进时间；旧回调不得关闭/修改新激活。

### F11（P2）编辑器模块关闭不触达存活扩展；Cleanup 不完整

- **位置**：`Plugins/CORUI/Source/PuertsMixinGenerator/Private/PmgDesignerExtensionFactory.cpp:17`、`Private/PuertsMixinGenerator.cpp:164`、`PmgDesignerExtension.cpp:282`
- **问题**：`CleanupActiveExtensions` 遍历 `ActiveExtensions`，但 factory 只创建、未注册；`Cleanup` 只移除委托/清 VBox，不失效延迟 token、不移除 ticker。
- **修复**：注册存活扩展或由 factory 统一 ownership；Cleanup 幂等且终态：失效延迟工作、注销委托、移除 ticker、释放 UI 状态。
- **测试**：保持 designer 打开时执行模块关闭/重载；无 ticker/委托/延迟回调残留。模块卸载崩溃未复现，需运行时验证。

### F12（P2）Mixin 生成在允许空 GeneratedClass 后仍解引用；保存失败静默

- **位置**：`PuertsMixinGenerator.cpp:285`、`:292`、`:301`、`:311`、`:347`（空类回退在 243–250）
- **问题**：空 `GeneratedClass` 有回退路径，但后续无条件解引用；目录创建/保存返回值被忽略。
- **修复**：生成类相关内容前强制校验；或所有访问统一守卫；检查目录/保存结果并报错。
- **测试**：未编译/无效蓝图、不可写目录；明确报错、不崩溃、不假装成功。

### F13（P2）`OnScriptInit` 在窗口打开后接受 null/无类型数据

- **位置**：`TypeScript/Manager/UIManager.ts:60`、`TypeScript/Game/UI/GameStart/UI_GameStart.ts:44`
- **问题**：只查真值不查可调用；payload 默认 null 但 GameStart 直接解引用；C++ 先打开/更新可见性，再回调 `OnScriptInit`，异常时窗口已插入/打开。
- **修复**：定义类型化脚本契约；校验 hook 可调用与 payload；打开/显示前完成数据初始化；初始化失败回滚激活并返回失败。
- **测试**：null/畸形 payload、缺失/非函数 hook、抛异常初始化；可预期失败且无半初始化窗口。

### F14（P3）调试面板 FPS 实际测量的是定时器频率

- **位置**：`TypeScript/Game/UI/Test/UI_TestDebug.ts:55`、`:77`
- **问题**：250ms 刷新，`1000 / elapsed` 被标为 FPS；稳定时约 4，与渲染帧率无关。
- **修复**：读取真实帧时间，或把标签改为“刷新频率”。
- **测试**：30/60 FPS 受控对比；显示值为渲染帧率或明确标注为定时器频率。

---

## 3. 每波验证协议

| 类型 | 命令/方法 |
|---|---|
| C++ 构建 | 关闭 UE 编辑器；`Build.bat CultivationOpenRealmEditor Win64 Development -Project="...\CultivationOpenRealm.uproject"`；若 DLL 被 `CrashReportClientEditor.exe` 占用，先关闭它 |
| TS 编译 | `node C:\Users\Liuary\AppData\Local\Temp\corui-tsc\node_modules\typescript\lib\tsc.js -p tsconfig.json` |
| PIE | MCP `StartPIE`，截图用 `CaptureEditorImage`；UI 测试套件自检 9/9 |
| 回归 | 每项按上文“测试”执行；GC 用 `gc`/重复开关窗口；重入用测试 UI 按钮 |
| 门禁 | `feel_status.guard.stale=false`；状态写入只走 `feel_*`；不手改 `flow.json`/`status.md` |

> 注意：修改 C++ 前先关闭编辑器（DLL 锁）；PIE 运行中不要用 MCP 关闭 WBP 编辑器窗口（已修复，但保持谨慎）。

---

## 4. 完成定义

- F1–F14 全部有修复 + 回归测试/证据；
- 每波构建/编译通过，PIE 冒烟通过；
- 审查报告中的 P1 全部关闭，P2/P3 有处理记录；
- OpenFeel：每波 `op attempt=pass`，需求 review + verify 通过；
- 版本归档只在用户确认后执行。

---

## 5. 与 OpenFeel 的对应

- 阶段：`v1.6.1-stage-05`（exec_running）
- 需求：`REQ-009`（审查，done）；本计划对应 **REQ-010**（全量修复）
- 每个波次建议单独 `op`，完成即 attempt pass，阶段保持 exec_running 直到全部波次完成。
