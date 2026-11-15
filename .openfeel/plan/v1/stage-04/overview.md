# v1.6.0-stage-04

## 目标

实现《观澜》CORUI 测试套件：10 个同风格 UMG 窗口 + TS Mixin + TestUIManager + 一键自检，覆盖层级、栈、状态机、对象池、同步/异步加载、UISingle 传参与四个 C++ 控件。

设计稿：`docs/design/ui-test-suite.md`（含概念图 `docs/design/mockups/`）。

## 依赖

无（基于现有 CORUI 与 Puerts 管线；不改 CORUI C++）。

## 操作方案

1. 技术验证：MCP 建 WBP → 加控件 → 设属性/字体 → 编译保存；确认 tsc 可用。
2. 资产：按 `一叶 → 传音 → 幕布 → 问对 → 层境 → 叠嶂 → 鹤信 → 器谱 → 勘验 → 观澜台` 顺序建 10 个 WBP。
3. TS：10 个 Mixin + `TestUIManager` / `TestEventBus` / `UI_TestSingleType` / `TestPaths`。
4. 接线：`UI_GameLoading` 完成回调 + `GameManager` 初始化，受 `TestUIManager.Enabled` 开关保护。
5. 验证：tsc、PIE 逐窗截图、自检 8/8、日志无 CORUI 错误。

## 验收

- 10 个窗口均能通过 CORUI 原生 PushUI/PopUI 打开与关闭；五层计数正确。
- 自检脚本 8/8 PASS。
- 视觉 token 一致（深墨 / 暖白 / 锈红 / 发丝线 / 无圆角无阴影）。
- `tsc` 通过，无新增编译错误；现有启动流程仅有受开关保护的入口改动。
