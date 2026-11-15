# OpenFeel 体验反馈与问题清单

> 项目：CultivationOpenRealm（UE 5.8 + CORUI 插件）
> 记录时间：2026-11-06 ~ 2026-11-07
> 工作流：`fast`；阶段 `v1.6.1-stage-05`（已归档）→ `v1.6.2-stage-06`（exec_running）
> 证据文件：`.openfeel/flow.json`（revision 176）、`.openfeel/flow-archive/v1.6.1.json`、`.openfeel/dev/guard-state.json`、`.openfeel/config.yaml`（meta.version 1.4.0）

## 0. 背景：这段会话做了什么

- 按审查计划完成 CORUI F1–F14 修复（6 个波次），每波建 op、构建、attempt pass；
- 需求级 review（独立复审）/verify，阶段 review→archiving→done，版本归档 v1.6.1；
- 顺手收口遗留阶段并归档 v1.5.0，开启 v1.6.2-stage-06；
- 中途穿插 UI 资产重建（UE MCP）、PIE 自检、Typecript 声明生成等长耗时任务。

整体评价：**门禁/归档/需求模型是可靠的**，主要痛点在「工具调用的可靠性与状态一致性」和「长任务下的流程打断」。

## 0.1 复核结果（2026-11-08，OpenFeel 1.5.15）

> 复核方式：逐项跑可观测行为，不以「升级了」为准。结论标记：✅ 已修 / ⚠️ 未复现或未测 / ❌ 仍存在。

| 编号 | 结论 | 复核证据 |
|---|---|---|
| OF-01 auto-review 阻断 | ⚠️ 未复现，未能证实已修 | 本轮约 20 次工具调用（`feel_*` / `pwsh`）无一次被 auto-review 拒绝；但无法按需触发故障，故不能断言服务端已修 |
| OF-02 op 状态不落盘 | ✅ 已修 | 新建临时 op-003 → `attempt pass` → `flow.json` 中 `state=done`、`attempts=1`、`attempted_at/completed_at` 均写入；`feel_status.opsSummary` 显示 `done:3` |
| OF-03 preflight 长任务过期 | ✅ 基本修复 | 本轮开始时 preflight 已过期 15.9 小时（`ok:false`, `ageMs≈57,259,035`），随后 `feel_req add` / `claim` / `feel_op create` 等写命令均直接成功，无需手动补 `feel_status` |
| OF-04 done/无阶段禁止 git commit | ⚠️ 未测 | 当前阶段为 `exec_running`，无法安全验证；门禁提示文本仍存在（该门禁疑似在 DSH 适配器侧，不在 openfeel npm 包内） |
| OF-05 构建命令恒 warn | ❌ 仍存在 | 一条复杂只读 `pwsh` 后 `guard.lastDecision = {tool:"pwsh", decision:"warn", reason:"shell-undecidable"}` |
| OF-06 advance 建议不一致 | ⚠️ 部分验证 | `exec_running` 下建议 `review_pending` 正确；`review_pending`→`review_failed` 与 `archiving`→`done` 的建议未复测（需真实推进相位） |
| OF-07 归档后悬挂阶段 | ✅ 已修 | `feel_status.focus.guards` 新增：「关联需求已全部完成（1 条，均处于终态）：可考虑阶段收口 / 归档（… --to done，或归档该版本）」 |
| OF-08 op 模板空填充警告 | ❌ 仍存在 | 所有 op 仍报 `fill:"empty"` + 「模板未填充（存在「- [ ] 待补充」）」，包括已 done 的 op-001/op-002/op-003 |
| 附加：claim 需显式 owner | ✅ 已修 | 不传 owner 直接 `feel_req claim(id=REQ-012)` 成功，解析为 `Liuary`（`.info.json` 的 user 仍为 `unknown`，但已回退 git user.name） |

**1.5.15 的新观察（需产品确认是否有意为之）**

1. `claim` 只写 owner/claimed_at，不再把 `draft` 推进到 `in_progress`：REQ-012 认领后仍为 `draft`，需手动 `advance scheduled → in_progress`。旧版本（REQ-010 记录）认领后即为 `in_progress`，语义变化建议在 CHANGELOG 说明。
2. `feel_op remove` 对 `done` 状态的 op 直接拒绝，需 `--force`；保护合理，但临时/复核用 op 无法清理，会留下 `done` 噪声（本轮临时 op-003 即如此）。
3. `checkpoints.exec.attempts` 仍为 0（与顶层 `attempts=1` 不一致），若下游按 checkpoints 统计会失真。

## 1. 问题清单（按严重度）

### OF-01（P0）自动审查服务不稳定，直接阻断工具调用

**现象**：`feel_*`、`pwsh`、`grep` 等工具在调用前被 auto-review 拦截，返回「body was not executed」，表现为随机失败；同一命令重试有时成功、有时连续失败。

**采样证据**（2026-11-07）：

| 时间 | 工具 | 错误 |
|---|---|---|
| 15:5x | `feel_op attempt` | `SERVER: Internal Server Error (request_id: f43e2c68-c313-479a-9006-fa38bede787a)` |
| 15:5x | `feel_verify health` | `SERVER: Internal Server Error (request_id: f7202334-1650-4655-8151-e22816c6860a)` |
| 15:5x | `feel_op attempt` | `SERVER: Internal Server Error (request_id: f565ce0c-2d9e-4da9-bc44-ff094253bb1e)` |
| 15:5x | `feel_op attempt` | `SERVER: Internal Server Error (request_id: 6c073b7c-b1f1-40d2-a469-0b13ebf27b8e)` |
| 16:0x | `grep` | `SERVER: Internal Server Error (request_id: d5888973-6561-407e-a138-7b54a1c2233e)` |
| 16:0x | `pwsh` | `Service is too busy. We advise users to temporarily switch to alternative LLM API service providers. (request_id: 789b34c3-6aae-427b-a48d-0916328dbe71)` |
| 16:0x | `pwsh` | `auto-review: reviewer must emit zero or more reasoning blocks followed by exactly one text block` |

**影响**：长任务里平均每 5–10 次调用就有 1 次失败；重试消耗上下文与时间；最严重时无法记录 op 结果（见 OF-02）。

**期望**：
1. auto-review 故障时应 **fail-open**（允许执行并记录审计），而不是 fail-closed 卡死流程；至少对只读工具（`grep`/`read`/`feel_status`）直接放行。
2. 区分「审查拒绝」与「服务不可用」，返回可重试标记，适配器可自动重试 1–2 次。
3. 暴露 auto-review 的健康检查接口。

**临时绕过**：重试；或换等价工具（如 `read` 代替 `grep`）。

---

### OF-02（P1）op 的 attempt 结果没有落到 flow.json

**现象**：`feel_op attempt pass` 返回成功（例：`[OK] attempt: 已记录 op-002 执行结果=pass`），但归档后 `flow-archive/v1.6.1.json` 中 `op-001`~`op-007` 全部仍是：

```json
{ "state": "pending", "attempts": 0, "checkpoints": { "exec": { "attempts": 0, "self": "pending" } } }
```

当前阶段 `v1.6.2-stage-06.op-001` 同样为 `state=pending / attempts=0`。

**影响**：审查/归档时无法从状态区分「已完成」与「未开始」；`feel_status.opsSummary` 一直是 `pending`，会误导调度（建议推进 vs 继续执行）。

**期望**：`attempt` 更新 `state`（pass→done/executed、fail→failed）与 `attempts/checkpoints.exec`；或提供 `feel_op complete <id>` 显式收口；`flow archive` 前对未完成 op 给出提示。

---

### OF-03（P1）preflight 10 分钟过期，长任务被反复打断

**现象**：构建 UE 工程、生成 15 MB 声明文件等操作动辄 10–20 分钟；操作结束后执行写命令（`feel_op`/`feel_archive`/`git commit`）会报：

```
请先运行 `openfeel flow next`（或 `flow status`）读取状态，再执行写操作（preflight 缺失或已过期）
```

采样：`feel_status.preflight.ageMs = 64612468`（约 17.9 小时）后，`feel_archive create` 直接报 `{"error":"preflight-required","errorKind":"refusal"}`。

**影响**：每次长任务后必须机械地先 `feel_status` 再写；写命令多时反复踩。

**期望**：
1. 允许写命令自身携带「读前状态」，或在 preflight 过期时**自动刷新一次**再判定；
2. 提供 `feel_status` 的轻量 `--touch` 语义（刷新 preflight 不产生大输出）。

---

### OF-04（P1）无活跃阶段 / done 阶段时禁止 `git commit`，归档收尾无处提交

**现象**：版本归档完成后（阶段 done、pipeline 无活跃阶段），对归档产物（`.openfeel/flow-archive`、`docs`、最终代码）执行 `git commit` 被门禁拒绝：

```
OpenFeel 写入门禁：禁止在当前阶段用 shell 做 git 工作区写（git commit）——
git commit 只允许活跃阶段处于 exec_running / review_passed / test_passed / archiving 之一；
plan_* / scheme_* / review_pending / review_failed / test_pending / test_failed / done 一律拒绝
（done 表示工作已收口，请走归档提交）。
```

**影响**：为了提交归档结果，只能**新建一个空阶段**（本次即为此开了 `v1.6.2-stage-06`），污染流程树；否则仓库留下未提交状态。

**期望**：
1. 归档动作本身可触发/允许一次「归档收尾提交」（例如 `archiving` 阶段内允许，或提供 `openfeel commit` 包装）；
2. 或者明确文档化「归档=先提交再归档」的顺序并在 `feel_archive` 前置检查未提交改动。

---

### OF-05（P2）构建命令恒被判 `warn`，每个命令多一轮审查

**现象**：每次 `Build.bat ...` / 复杂 `pwsh` 都命中：

```json
{ "tool": "pwsh", "decision": "warn", "reason": "shell-undecidable" }
```

**影响**：构建一次要额外等一次判定；`warn` 语义不明确（是否会阻断？），日志噪声大。

**期望**：对已知的安全模式（如 `Build.bat`、`node .../tsc.js`、`python Tools/...`）提供可配置白名单或一次性 `allow`；`warn` 在返回中说明「已放行」。

---

### OF-06（P2）`flow advance` 的下一步建议与实际门禁不一致

采样：

| 当前相位 | 建议命令 | 实际 |
|---|---|---|
| `review_pending` | `advance --to review_failed` | 合法下一相位是 `review_failed` / `review_passed`，建议只给了失败分支 |
| `archiving` | `advance --to done` | 需要先 `feel_stage verify`，否则被 done 门拒绝 |

**期望**：`feel_next`/`suggestedCommand` 应给出「通过路径」+ 前置动作（如「先 verify」），而不是第一个合法项。

---

### OF-07（P2）版本归档后仍有同版本历史阶段悬挂

**现象**：本次开始前 `v1.5.0-stage-03` 处于 `exec_running`，其需求 REQ-005/006 均已 done，但阶段从未收口；`feel_status` 一直把它当作活跃阶段调度。需要人工判断后补 verify + 归档 v1.5.0。

**期望**：`feel_status` 对「需求全部完成但阶段仍 exec_running」给出显式提醒/一键收口建议；或在 `feel_req advance done` 时检查关联阶段是否可归档。

---

### OF-08（P3）体验细节

1. `feel_status` 每次输出的 `guard.lastDecision.command` 会截断长命令，排查时信息不足。
2. `opsSummary.byState` 的键是 `pending`，但语义上 op 可能已成功执行（见 OF-02），字段命名易误解。
3. `feel_op create` 生成的模板固定含「- [ ] 待补充」，`fill=empty` 警告持续出现；对「执行型 op」建议允许空模板或自动填充标题/证据占位。

## 2. 非 OpenFeel、但影响整体体验（记录备查）

这些属于 **UE MCP / 外部工具**，不是 OpenFeel 的问题，供环境维护参考：

1. `call_tool` 需要 `toolset_name` + `tool_name`，直接用 `EditorToolset.EditorAppToolset.StartPIE` 作为 tool 名会报 not found（文档未说明）。
2. `LogsToolset.GetLogEntries` 的 `category` 默认值 `LogsToolset` 实际不存在，必须显式传空串。
3. `SlateInspector` 不暴露 PIE 内 UMG 运行时控件（无法点击观澜台按钮），也不暴露编辑器控制台输入浮层，导致「自动触发自检」不可行，只能用 AutoDemo 或人工点击。
4. `-ExecCmds` 在编辑器模块加载前执行，Puerts 的 `Puerts.Gen` 命令此时未注册；必须用 `-ExecutePythonScript` + Slate tick 回调（已在 `Tools/TypeScript/gen-typings.ps1` 落地）。
5. MCP 会话在上层工具超时/编辑器重启后会失效（`Unknown session id`），需要重新 `initialize`——脚本里应内置重连。

## 3. 做得好的地方（建议保持）

- `flow.json` 为权威 + 禁止手改 + 乐观并发 revision，可靠性高；
- 门禁拒绝信息具体（能直接定位到缺哪一步）；
- 需求/阶段/版本三层模型清晰，`release-check` + 归档分片非常实用；
- `feel_status` 的 compact 档位与 `dispatch` 模板对调度很友好；
- 归档判据（阶段全 done + 无 blocking REV）没有 force 后门，这点很好。

## 4. 建议的修复优先级

| 优先级 | 项 | 理由 |
|---|---|---|
| P0 | OF-01 auto-review fail-open + 只读放行 | 直接决定长会话可用性 |
| P1 | OF-02 op 状态落盘 | 影响调度正确性与归档可读性 |
| P1 | OF-03 preflight 自动刷新 | 高频打断，成本低收益高 |
| P1 | OF-04 归档收尾提交通道 | 影响每次发版的仓库卫生 |
| P2 | OF-05 安全命令白名单 | 降低构建期延迟 |
| P2 | OF-06 建议命令一致性 | 减少踩门禁次数 |
| P2 | OF-07 悬挂阶段提醒 | 避免旧阶段长期占用调度 |
| P3 | OF-08 体验细节 | 锦上添花 |

## 5. 复现所需环境

- `fast` 模式；`auto_advance: enabled`；`execution_mode: manual`；
- 单会话跨 2 天、含多次 10–20 分钟构建与 UE 资产/声明生成；
- 期间使用 `feel_flow` / `feel_op` / `feel_req` / `feel_stage` / `feel_archive` / `feel_verify` 全套接口。
