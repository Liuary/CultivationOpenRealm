# CORUI F1–F14 修复记录（v1.6.1-stage-05）

> 对应需求：REQ-010；来源审查：[corui-review-2026-11-05.md](corui-review-2026-11-05.md)；执行计划：[corui-review-fix-plan.md](corui-review-fix-plan.md)
> 日期：2026-11-05

## 0. 验证基线

| 项 | 命令/方式 | 结果 |
|---|---|---|
| C++ 构建 | `Build.bat CultivationOpenRealmEditor Win64 Development -Project=...\CultivationOpenRealm.uproject` | 6 波每波 Succeeded，末次全量 Succeeded |
| TS 编译 | `node .../typescript/lib/tsc.js -p tsconfig.json` | exit 0（无错误） |
| PIE 冒烟 | MCP `StartPIE`（PlayMode_InViewPort，warmup 15/22s）× 2 次会话 | GameStart→GameLoading→观澜台全链路通，无崩溃 |
| 生命周期平衡 | 日志核对 | 每个 Push 均有配对的 OnClose/Destruct，无重复释放 |
| 形态 | `git diff`（CORUI 插件 + TypeScript + Typing 声明同步） | 见下逐项 |

## 1. 逐项处理

| 发现 | 波次 | 处理 |
|---|---|---|
| **F1** 根/池无 GC 可见所有权 | W1 | `RootCanvas` 改 `UPROPERTY(Transient) TObjectPtr`；`LayerPanels` 改 `UPROPERTY(Transient) TMap<EUILayerType, TObjectPtr<...>>`；`WidgetPool` 改 `UPROPERTY(Transient)`（FUserWidgetPool 内部数组对 GC 可见）；`ClearUI` 明确释放顺序并显式置空根引用。UE 5.8 的 `UObject::AddReferencedObjects` 已改为静态接口，不能 override，故走 UPROPERTY 反射收集。 |
| **F2** pending 当失败/终止不一致 | W4 | 引入 `FPendingPushRequest` + `bCompleted` exactly-once 终止；`RequestAsyncLoad` 返回有效句柄即视为已受理（不再判 `!HasLoadCompleted`）；`ResolveClass` 失败、同步加载失败、空路径统一走 `FailPushRequest`。 |
| **F3** 回调重入破坏栈遍历/事务 | W2 | 容器加 `DispatchDepth`：派发期间 `PopUI`/`NativeRemoveWindow` 入队，`UpdateWindowState` 用栈快照 + 每次回调前 `IsWindowRegistered` 复验，出口 `DrainPendingMutations` 统一执行；管理器 `NativeUpdateUI` 加同层重入合并；`NativePushUI` 事务结束前复验激活仍注册；`DetachWindow` 改为先账本+Slate 脱钩、再发 `OnClose`。 |
| **F4** UI/JS 启动 next-tick 竞争 | W3 | `JsEnvManager::OnJsEnvBeginPlay` 改为同步 `StartJsEnv`（`CorStart.Start`→`BindCppEvent` 必然早于 UI 的 next-tick InitUI）；`StartJsEnv` 幂等；新增 `UCORUIManager::IsUIInitialized()`，TS `BindCppEvent` 绑定后若 UI 已就绪立即 `Init()`（晚订阅补偿），`GameManager.Init/Deinit` 幂等。 |
| **F5** 关闭/换世界无统一边界 | W3 | `UCORUIManager` 增加 `ESessionPhase`（Created/Starting/Ready/Stopping/Stopped）幂等状态机；注册 `FWorldDelegates::OnWorldCleanup`；关闭顺序=拒绝新工作→广播 Deinit→清窗口（JS 存活）→释放 UI；`PushUI` 在未就绪时拒绝；`InitUI` 幂等并回收半初始化；`JsEnvManager::Deinitialize` 先 `ShutdownUI()` 再 `JsEnv.Reset()`；会话级关闭不清空 JS 订阅，换图后可重建。 |
| **F6** 在途请求不绑会话 | W4 | 增加 `UISessionGeneration`（ClearUI 递增）与 `InFlightPushes` 跟踪；完成时校验 generation，不匹配则失败终止；`ClearUI` 调 `CancelPendingPushes()` 取消句柄并对每个在途请求恰好一次失败回调；移除空的 `IsValid(this)` 分支。 |
| **F7** FullScreen 重复 Push 双条目 / +2 语义 | W2 | 容器 `PushUI` 对已注册实例复用 Slot 并把栈条目移动到栈顶（`WindowsStack.Remove` 后 `Push`），保证同一激活唯一；`bPopSelf=false` 的 FullScreen 弹出偏移由 `WindowIndex+2` 改为 `+1`，与头文件契约（只弹上方、保留目标）一致。 |
| **F8** 插入失败不归还池 | W2 | 取池前完成 `WindowName` 有效性与层级校验；插入失败时把本次取得的实例 `WidgetPool.Release(..., true)` 归还；失败不再对外表现为可用激活。 |
| **F9** Streamable 未向 JS 暴露释放/无效句柄返回 ID | W4 | `AutoRegister` 增加 `ReleaseHandle`/`CancelHandle`/`ReleaseAll` 绑定；无效/空路径返回 0 且不登记；完成回调改为结构化 `(bool 成功, int32 句柄ID)`；失败请求移除 map 条目；`ReleaseHandle` 无论句柄有效与否都移除条目。 |
| **F10** TS 延迟回调活过激活 | W4 | GameLoading 增加 `_Activation` 代际 + `_ExitTimer` 跟踪，`OnClose`/`Destruct` 统一 `ClearActivationTimers()`，延时回调校验代际；TestData 增加 `ReuseTimerId` + `Activation`，同样在关闭/析构清理并校验。 |
| **F11** 编辑器模块关闭不触达扩展 / Cleanup 不完整 | W5 | 工厂注入 `ActiveExtensions` 登记表，`CreateDesignerExtension` 登记存活扩展；`CleanupActiveExtensions` 先快照再清空；`FPmgDesignerExtension::Cleanup` 改为终态幂等：失效 `LifeToken`、移除 ticker、注销委托、清空并隐藏 VBox。 |
| **F12** Mixin 生成空 GeneratedClass 仍解引用 / 保存静默失败 | W5 | 生成前强制校验 `Blueprint->GeneratedClass`，缺失时弹窗报错并返回；类相关判断统一改用该指针；生成目录/Import 目录用 `DirectoryExists`+`CreateDirectoryTree` 检查；两处 `SaveStringToFile`、`LoadFileToString` 检查返回值并报错。 |
| **F13** OnScriptInit 在窗口打开后接受 null/无类型数据 | W6 | `NativePushUI` 改为「插入→脚本回调（数据初始化）→复验激活→NativeOpen/刷新」，初始化失败/自关时不再打开；TS `UIManager.PushUI` 校验钩子必须为函数、null 载荷用空 `UISingle` 兜底、抛异常时 `CloseUI` 回滚；`UI_GameStart.OnScriptInit` 空载荷兜底。 |
| **F14** 调试面板 FPS 实为定时器频率 | W6 | `UI_TestDebug` 改读 `GameplayStatics.GetWorldDeltaSeconds`，显示真实 `FPS · 帧 ms`，并单独标注刷新间隔 `刷新 ms`。 |

## 2. 附带修正（运行时发现）

| 问题 | 处理 |
|---|---|
| Puerts mixin 不执行 constructor，类字段初始值不生效，导致 `_Activation++` 得到 NaN → 延时退出回调恒判为过期（观澜台打不开） | `Construct()` 内显式初始化 `_Activation`/`_ExitTimer`、`Activation`/`ReuseTimerId`、`LastTime`/`TimerId` |
| TS 传 `undefined` 作 `WindowName` 被 Puerts 转成字符串 FName `"undefined"`，CDO 命名回退失效 | `UIManager.PushUI` 显式传 `"None"`；验证窗口名恢复为 `UI_GameStart_C` / `UI_GameLoading_C` |

## 3. 独立复审与二次修复

审查方式：独立子代理只读复审 `git diff`（Codex gpt-6.1-sol 因模型容量不可用，改由独立 agent 复审）。复审结论：无 P1；正常路径未发现双 `NativeOpen`、零次/双次终止回调；容器重入、会话代际、GC 所有权、扩展清理判定为成立。

据复审意见修复的项：

| 复审发现 | 严重度 | 处理 |
|---|---|---|
| `RequestAsyncLoad` 同步完成时仍登记在途请求，导致永久残留（钉住 FJsObject/句柄） | P2 | 仅当 `!Request->bCompleted` 才登记 |
| GameLoading 先开观澜台再 `CloseUI(true)`，可能把刚开的窗口一起弹掉 | P2 | 改为先 `CloseUI(true)` 再 `OpenHub()` |
| 生命周期回调内重复 Push：去重弹出被入队，随后 Push 成功、队列弹出又把新激活关掉 | P2 | 容器 `PushUI` 丢弃同名待执行弹出/该实例待移除 |
| `UpdateWindowState` 返回值仍是旧快照结果 | P3 | `DrainPendingMutations` 返回重算状态 |
| 最终销毁期间对在途请求回调，可能重入 Push 二次失败 | P3 | `bSuppressPendingCallbacks` 在 final teardown 抑制 JS 回调 |
| Streamable：Release/Cancel 后完成回调仍补发一次终态；含无效路径的列表静默加载子集 | P3 | 条目已移除则跳过回调；含无效路径整体拒绝 |
| Mixin 生成缺 GeneratedClass 时提前 return，连 Import 列表都不刷新 | P3 | 只跳过该文件，继续刷新 Import 列表 |
| 扩展登记表不清理失效弱引用 | P3 | 创建时压缩失效条目；工厂析构断开登记表指针 |
| 成功回调先于 Open，脚本初始化里自关时返回值 None 但已报成功 | P3 | 在 `PushUI` 头注释中明确 exactly-once 契约与自关语义 |

复审另指出提交卫生问题（非代码缺陷）：本改动集引用了上一会话未跟踪的 `TypeScript/Manager/TestUIManager.ts` 与 `TypeScript/Game/UI/Test/`，提交时须一并纳入，否则 TS 解析失败。

二次修复后再次：构建 Succeeded、`tsc` exit 0、PIE 冒烟通过（GameLoading 先关、观澜台随后正常打开，无报错）。

## 4. 未覆盖/后续

- 观澜 9 步自检未在本轮以程序化方式触发（SlateInspector 不暴露 PIE 内 UMG 运行时控件，无法点击「自检」按钮）；改为手动流程冒烟 + 日志配对核对。若要回归自检，需在 PIE 内人工点击一次。
- 换图（同一 GameInstance 内 Travel）路径未实测，仅按状态机与 world cleanup 委托做了静态保障。
- `Typing/ue/ue.d.ts` 为生成物，本轮手工同步了新增的 `IsUIInitialized`/`ReleaseHandle`/`CancelHandle`/`ReleaseAll` 声明；下次在编辑器内重新生成 typings 时应与之一致。
