# 交接：CORUI 审查全量修复（v1.6.1-stage-05）

> 用途：本会话上下文已过长，后续在**新会话**继续。先读这份 + 修复计划，再调 `feel_status`。
> 日期：2026-11-05 / 项目：CultivationOpenRealm

## 1. 当前状态

| 项 | 值 |
|---|---|
| 已归档版本 | `v1.6.0`（stage-04 + REQ-007/008） |
| 当前阶段 | `v1.6.1-stage-05`，`exec_running`，auto_advance enabled |
| 已完成需求 | `REQ-009`（CORUI 只读审查，Codex gpt-6.1-sol），done |
| 待建/待领需求 | ~~REQ-010~~ → **已全量处理**（见 §8；待 review/verify 与用户确认归档） |
| op | `op-001`（审查）pass；`op-002`~`op-007`（W1–W6）均 attempt pass |
| 门禁 | `feel_status.guard.stale=false`，`signature=valid`；不要再删状态文件 |
| 版本归档 | 需要用户明确确认，不自动归档 |

## 2. 必读文档

1. **审查报告**：`docs/reviews/corui-review-2026-11-05.md`
2. **修复计划（按优先级/波次）**：`docs/reviews/corui-review-fix-plan.md`
3. 崩溃复盘（两处已修 UAF）：`docs/design/crash-2026-11-04.md`
4. 观澜测试套件设计：`docs/design/ui-test-suite.md`
5. 门禁问题归档（已解决）：`docs/issues/openfeel-guard-stale-2026-11-05.md`

## 3. 本会话已完成（不要重做）

- 《观澜》测试套件：10 个 UMG 窗口 + TS Mixin + TestUIManager + 9 步自检；资产在 `/Game/UI/TestSuite`，构建器 `Tools/UIBuilder`。
- 视觉整改：文案改普通术语、字体恢复霞鹜文楷、按钮描边加粗到 5（低分辨率可辨）；截图在 `docs/design/screenshots/`。
- 插件崩溃修复（已构建验证）：
  - `PmgDesignerExtension`：`OnAssetClosed` 委托在 Uninitialize/析构注销；
  - `Initialize` 延迟 lambda 改为 LifeToken + 弱 designer/blueprint 守卫。
- `v1.6.0` 已归档，`REQ-007`/`REQ-008` 已收口。

## 4. 下一步（按修复计划执行）

顺序：**W1 → W2 → W3 → W4 → W5 → W6**（P1 优先；详见 fix-plan）。

- **W1** F1：根/池 GC 所有权（`UCORUIManager` 强持有 RootCanvas + WidgetPool 引用收集）
- **W2** F3/F7/F8：生命周期重入、栈事务、FullScreen 重复 Push、插入失败归还
- **W3** F4/F5：UI/JS 启动顺序、关闭/换世界统一边界
- **W4** F2/F6/F9/F10：异步 exactly-once、会话取消、Streamable 释放、TS 延迟回调
- **W5** F11/F12：编辑器扩展清理、Mixin 生成健壮性
- **W6** F13/F14：TS 初始化契约、调试面板 FPS

每波流程：

1. `feel_req` 确认 REQ-010 在 `in_progress`；
2. 建 `feel_op`（如 `W1 所有权`），实现 + 构建/测试；
3. `feel_op attempt pass`；
4. 记录证据（构建输出、测试结果、截图/日志路径）；
5. 阶段保持 `exec_running`；全部波次完成后走 review → 归档（需用户确认）。

## 5. 环境与命令

- 编辑器：UE 5.8，项目 `CultivationOpenRealm.uproject`；MCP 在 `127.0.0.1:8000`。
- C++ 构建（**必须先关编辑器**）：
  ```powershell
  & 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
    CultivationOpenRealmEditor Win64 Development `
    -Project="C:\Users\Liuary\Dev\Mine\Unreal\CultivationOpenRealm\CultivationOpenRealm.uproject" `
    -WaitMutex
  ```
  - If link fails with DLL locked：关闭 `CrashReportClientEditor.exe` 后重试。
- TS 编译：
  ```powershell
  node C:\Users\Liuary\AppData\Local\Temp\corui-tsc\node_modules\typescript\lib\tsc.js -p tsconfig.json
  ```
- PIE 验证：MCP `StartPIE`（`PlayMode_InViewPort`，warmup 1）+ `CaptureEditorImage`；测试台自检 9/9。
- Codex（如需复审）：`codex exec -m gpt-6.1-sol --sandbox read-only -C <project> "<prompt>"`。

## 6. 已知坑

- 修改 C++ 前关闭编辑器；`CrashReportClientEditor.exe` 会锁插件 DLL。
- 不要在 PIE 运行时用 MCP `SlateInspector.Windows close` 关闭 WBP 编辑器（已修复，但谨慎）。
- 不要手改 `flow.json` / `status.md` / `guard-state.json`；状态写只用 `feel_*`。
- 归档/发布需用户确认；不要 `--force` 绕过门禁。
- 仓库当前有未提交改动（UI 套件、崩溃修复、文档、审查报告）；提交策略由新会话按用户偏好决定。

## 7. 新会话启动提示（复制即用）

> 继续 OpenFeel 阶段 `v1.6.1-stage-05`。先读 `docs/handoff/continue-corui-fixes.md` 与 `docs/reviews/corui-review-fix-plan.md`，调 `feel_status` 确认门禁正常；W1–W6 已全部完成，下一步是 REQ-010 review/verify 与归档确认。

## 8. 2026-11-05 修复回合结果（W1–W6 已完成）

- 逐项处理见 [corui-fix-record-2026-11-05.md](../reviews/corui-fix-record-2026-11-05.md)（F1→F14 对照 + 复审二次修复 + 证据）。
- 每波独立构建 Succeeded；TS `tsc -p tsconfig.json` exit 0；PIE 冒烟三次会话通过（GameStart→GameLoading→观澜台，生命周期配对，无崩溃）。
- 运行时新发现并修复：① Puerts mixin 不执行 constructor → 激活代际字段必须在 `Construct()` 显式初始化；② TS `WindowName=undefined` 被转成 FName `"undefined"` → 现在显式传 `"None"` 以走 CDO 命名回退。
- 独立复审：无 P1；10 项 P2/P3 发现，8 项已修代码、2 项文档化/提交卫生提醒。
- OpenFeel：REQ-010 done；`v1.6.1-stage-05` = `review_passed`；op-001~op-007 全部 attempt pass。
- 未覆盖：观澜 9 步自检未程序化触发（SlateInspector 读不到 PIE 内 UMG 运行时控件）；同一 GameInstance 换图路径仅静态保障。
- 提交卫生：TS 改动引用了上一会话未跟踪的 `TypeScript/Manager/TestUIManager.ts` 与 `TypeScript/Game/UI/Test/`，提交须一并纳入；`Typing/ue/ue.d.ts` 为生成物，本轮手工同步了新增声明。
- **下一步：等用户确认后再 `feel_archive create version=v1.6.1`（归档前先 `feel_req release-check`）。**
