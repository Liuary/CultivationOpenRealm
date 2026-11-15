# v1.6.1-stage-05

## 目标

用 Codex `gpt-6.1-sol` 对 CORUI 插件做一次**只读、独立**的代码审查，产出可执行的分级问题报告，并把 P0/P1 拆成后续需求。

## 审查范围

- `Plugins/CORUI/Source/CORUI`：`UCORUIManager`、`UCORWindow`/`UCORUserWidget`、`UCORCanvasContainer`、`UCORStreamableManager`、C++ 控件（CORButton/Image/TextBlock/ProgressBar）、配置与 ZOrder。
- `Plugins/CORUI/Source/JsEnvManager`：JS 虚拟机生命周期。
- `Plugins/CORUI/Source/PuertsMixinGenerator`：Mixin 生成、`FPmgDesignerExtension`（近期修过两处 UAF）、编辑器扩展。
- `Plugins/CORUI/Config`、`Content/Config`。
- TS 桥接面：`TypeScript/Manager/UIManager.ts`、`GameManager`、`Utils/UISingle`、`Game/UI/*`（只读，用于判断 C++/TS 契约）。

## 重点

1. 架构与分层：Manager/Window/Container/Streamable/JS 桥接的职责边界。
2. 生命周期与所有权：委托、Ticker、Timer、对象池、Slate 引用、UObject 弱引用；重点复核已修的两处（`OnAssetClosed` 委托、`Initialize` 延迟 lambda）。
3. 线程与异步：`FStreamableManager`、JS 回调、对象池并发/重入。
4. 错误处理：加载失败、缺层、重复 Push/Pop、空路径、异步回调失效。
5. JS 桥接安全与契约：`FJsObject` 回调、`blueprint.mixin` 注入、类型/空值假设。
6. 性能与可测试性：ZOrder/状态更新遍历、Tick、可注入的测试点。
7. 编辑器工具：Mixin 生成的稳定性与关闭顺序。

## 方法

```powershell
codex exec -m gpt-6.1-sol --sandbox read-only -C <project> "<review prompt>"
```

只读审查，不允许改代码；报告由 Codex 输出，Feel 落盘到 `docs/reviews/`。

## 输出

Markdown 报告，包含：

- 范围与方法、总体结论；
- 发现列表：`严重度 P0/P1/P2/P3`、`文件:行`、问题、证据、建议修复、回归测试；
- 架构/生命周期/异步/桥接四个维度的专项结论；
- Top 5 风险与建议的后续需求拆分；
- 未覆盖/不确定项。

## 验收

- 三个 C++ 模块与 TS 桥接面均有覆盖结论；
- 每条发现带 `文件:行` 与可执行修复建议；
- P0/P1 已整理为后续需求候选；
- 报告落盘到 `docs/reviews/corui-review-2026-11-05.md`。

## 后续（已启动）

- 审查完成：`REQ-009` done；报告 `docs/reviews/corui-review-2026-11-05.md`。
- 全量修复：`REQ-010`（in_progress）；计划 `docs/reviews/corui-review-fix-plan.md`，按 W1–W6 执行 F1–F14。
- 交接：`docs/handoff/continue-corui-fixes.md`（新会话从这里继续）。
