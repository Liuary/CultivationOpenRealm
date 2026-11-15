# v1.1.0-stage-02

## 目标与范围

让 CORUI 窗口在 Pop/关闭后真正进入析构路径（`SObjectWidget` 销毁 → `UUserWidget::NativeDestruct` → puerts JS `Destruct()`），并保证池化复用与 `ClearUI`/世界结束时的完整回收。

改（4 个文件）：`Plugins/CORUI/Source/CORUI/Public/UI/Container/CORCanvasContainer.h`、`Private/UI/Container/CORCanvasContainer.cpp`、`Public/CORUIManager.h`（仅在需要时）、`Private/CORUIManager.cpp`；临时验证日志可加在 `Private/UI/Widget/CORUserWidget.cpp`，验证后降为 `Verbose` 或移除。不改 Puerts 插件、窗口业务行为与布局、TS 业务逻辑。

依赖：无强依赖；建议在 stage-01 编译通过后做（改的是不同文件，但都要重编）。

## 关键现状与根因

- 调用链：`UIManager.PopUI` → `UCORUIManager::NativePopUI`（持 `DataLock`）→ `UCORCanvasContainer::PopUI`（持 `ContainerLock`）→ `NativeRemoveWindow`。
- 根因 1：`NativeRemoveWindow`（`CORCanvasContainer.cpp:126-141`）只做 `RemoveFromParent()`，窗口实例仍在 `WidgetPool` 的 `FUserWidgetPool::ActiveWidgets`（UPROPERTY 强引用）里；`CachedSlateByWidgetObject` 又持有 `TSharedPtr<SWidget>`，Slate 不销毁 → `NativeDestruct`/JS `Destruct` 都不触发，对象永不回收。
- 根因 2：`NativePushUI`（`CORUIManager.cpp:246-256`）发现同名窗口 → `NativePopUI` → 仍复用旧 `WindowInstance` 指针，未重新 `GetOrCreateInstance`；Pop 一旦改为 Release，该指针必须重新取。
- 根因 3：`ClearUI`（`CORUIManager.cpp:177-192`）① `InitUI` 用 `GetGameViewport()->AddViewportWidgetContent(TakeWidget())` 添加，却用 `UGameViewportSubsystem::RemoveWidget` 移除，添加/移除 API 不配对且 `TSharedPtr<SWidget>` 未保存；② `ReleaseAll(true)` 前没有逐窗 `NativeClose()`，也没清 `LayerPanels`、各容器 `Windows/WindowsStack/ChildMaxZOrder`；③ 未 `ResetPool()`。
- 锁与重入：`Release(..., true)` 会立即销毁 Slate 并触发 `NativeDestruct`，其中 JS 回调可能重入 `PopUI`；锁内不得调用 Release，也不得边遍历边释放。

## 具体实施步骤

1. **注入池指针**（`CORCanvasContainer.h/.cpp`）：前置声明 `class FUserWidgetPool;`；新增 `void SetWidgetPool(FUserWidgetPool*)`、`void ClearAllWindows()`、私有 `FUserWidgetPool* WidgetPool = nullptr`（非拥有）、`void DetachWindow(UCORWindow*)`、`void ReleaseWindows(const TArray<UCORWindow*>&)`。
2. **拆「摘除 / 释放」**（`CORCanvasContainer.cpp`）：
   - `DetachWindow`：`WindowsStack.Remove` → `Windows.Remove(name)` → `Hide()` → `NativeClose()`（必须在 Release 前）→ `RemoveFromParent()`。
   - `ReleaseWindows`：锁外调用，`if (!WidgetPool) { 告警; return; }`，`TSet` 去重后逐个 `WidgetPool->Release(W, true)`。
   - `NativeRemoveWindow`：`FScopeLock` 内 `DetachWindow`，锁作用域结束后 `ReleaseWindows({W})`，再 `UpdateWindowState(EWindowState::None)`。
   - `PopUI`：先倒序快照待移除窗口成数组（避免边遍历边改 `WindowsStack` 索引错位），逐个 `DetachWindow`，锁外统一 `ReleaseWindows`；FullScreen 的 `TargetIndex`/`bPopSelf` 逻辑保持不变。
   - `ClearAllWindows`：锁内倒序收集全部有效窗口 → `DetachWindow` → `Windows.Empty()`、`WindowsStack.Empty()`、`ChildMaxZOrder = 0`；锁外 `ReleaseWindows`。
3. **修 `NativePushUI`**：命中旧窗口先 `NativePopUI(..., false)`，然后**无条件**重新 `WidgetPool.GetOrCreateInstance(TSubclassOf<UCORWindow>(WindowClass))`，并用 `IsA(WindowClass)` 校验；失败走原错误回调。
4. **修 `ClearUI` 与根 Canvas 配对**：`InitUI` 里 `WidgetPool.SetWorld(GetWorld())` 提前，创建每个容器后 `SetWidgetPool(&WidgetPool)`，根 Canvas 改用 `UGameViewportSubsystem::AddWidget(NewRootCanvas)`（与现有 `RemoveWidget` 配对）。`ClearUI` 顺序：逐层 `SetWidgetPool(nullptr)` + `ClearAllWindows()` → `LayerPanels.Empty()` → `RootCanvas->ClearChildren()` → `Subsystem->RemoveWidget(RootCanvas.Get())` → `RootCanvas.Reset()` → `WidgetPool.ReleaseAll(true)` → `WidgetPool.ResetPool()` → `WidgetPool.SetWorld(nullptr)`（`SetWidgetPool(nullptr)` 必须在 `SetWorld(nullptr)` 之前）。
5. **锁与重入**：`DataLock`/`ContainerLock` 内只做摘除与状态变更；Release 与可能触发 JS 的回调全部放锁外；重入的 `PopUI` 因窗口已摘除而直接返回，`IsValid` + `TSet` 双保险防二次释放。
6. **验证日志**：`UCORUserWidget::NativeDestruct` 加 `[CORUI][Destruct]`（Log），`ReleaseWindows` 加 `[CORUI][Release]`；需要 JS 证据时临时在 `UI_GameLoading.ts` 的 `Destruct()` 加一行 Log，验证后删除。

## 验收标准

| # | 判据 | 验证 |
|---|------|------|
| B1 | 编译通过 | `Build.bat CultivationOpenRealmEditor Win64 Development -Project=... -WaitMutex` → `Result: Succeeded`，0 error |
| B2 | Pop 触发原生析构 | PIE Push `UI_GameStart` → Pop：日志出现 `[UI_GameStart][OnClose]` 与 `[CORUI][Destruct] UI_GameStart_C...`（修复前只有前者） |
| B3 | JS 析构同路径 | 临时 TS 日志时出现 `[UI_GameLoading][Destruct]`；无 TS 日志则以 B2 为判据 |
| B4 | 池计数不增长 | 连续 10 次 Push→Pop 同一窗口：`GetActiveWidgetCount()` 回到 0，`GetInactiveWidgetCount()` 稳定在 0/1，不随次数增长 |
| B5 | 复用正确 | 第 2 次 Push 复用同一实例，`OnScriptInit`/`NativeOpen` 正常、状态无残留 |
| B6 | ClearUI 完整回收 | 退出 PIE/切关卡：逐窗 `[CORUI][Destruct]` → `ReleaseAll`/`ResetPool`，计数归零，无 `Ensure`/`Fatal` |
| B7 | 根 Canvas 移除正确 | `AddWidget`/`RemoveWidget` 成对（各打一条日志），退出后视口无残留遮挡 |
| B8 | 稳定性 | 100 次 Push/Pop 不崩溃、内存不线性增长（`stat memory`），无悬挂引用警告 |

## 风险与回滚

- **Release 立即销毁 Slate**：TS/其他对象若仍持窗口引用会崩 → 检查 `UIManager`/`GameManager`/`UISingle` 是否缓存窗口，`OnClose` 后禁止再用；必要时 TS 侧 `UE.System.IsValid` 守卫。
- **`NativeDestruct` 里的 `Hide()`** 会在析构中对已空 Slate 调 `SetVisibility`，可能 ensure → 若复现，改为 `NativeDestruct` 只重置状态、`Hide()` 移到 `NativeClose`。
- **池复用状态残留**（`bIsShowing`/`WindowState`/TS 字段）→ `NativeOpen` 已重置 `WindowState`；复用必须重跑 `OnScriptInit`，由 B5 回归。
- **`GetOrCreateInstance` 返回同类异名旧窗口** → 步骤 3 的 `IsA` 校验 + 复用前 `SetWindowName`/`SetLayer`。
- **`SetWorld(nullptr)` 后仍有 Release** → 顺序保证（先断容器指针再清 World），`ReleaseWindows` 对空池只告警。
- **与 stage-01 混改** → 分开提交，本阶段只碰 4 个文件。
- **回滚**：还原上述 4 个文件（`git -c filter.lfs.process= checkout -- <4 files>` 或阶段前备份），无数据迁移；回滚后回到「不析构」旧现状，不引入新崩溃。
