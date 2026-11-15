# 问题报告：完全重启后 `guard.stale=true`（host-started-at）且无法自愈

> 状态：**已解决**（2026-11-05 OpenFeel 更新后验证，`guard.stale=false`，写入恢复）
>
> 提交方：CultivationOpenRealm / Feel 会话
> 日期：2026-11-05
> 组件：`openfeel-dsh` 适配器（版本 1.5.11）
> 期望：完全重启 DSH 宿主后 `feel_status.guard.stale === false`
> 实际：重启后仍 `stale=true`，`staleReason="host-started-at"`，所有 `feel_*` 写操作被拒
> 诉求：请 OpenFeel 给出**不依赖手动删除状态文件**的解决方案（自愈 / 修复 / 受支持的修复命令均可）

---

## 1. 现象

用户已**完全重启** DSH 宿主（进程 PID 44056，启动于本地 2026-11-05 01:08:26 / UTC 2026-11-04T17:08:26Z），
但会话内 `feel_status` 仍返回：

```json
{
  "guard": {
    "registered": true,
    "stale": true,
    "staleReason": "host-started-at",
    "pid": 44056,
    "hostStartedAt": "2026-11-04T15:10:19.028Z",
    "loaded_hash": "9aecb20f4624723610915b1b6091c48fa2146b949ae246bc61fb103b576dbe1f"
  },
  "workflowGate": {
    "required": true,
    "reason": "guard-stale",
    "actions": ["完全重启 DSH 宿主（不是重载会话）", "..."]
  }
}
```

影响：`feel_stage` / `feel_op` / `feel_req` / `feel_flow advance` 等写操作全部被拒，
工作已实际完成（代码、构建、验证），但 OpenFeel 的任务勾选、op 记录、review 与阶段推进无法写入。
用户已重启至少一次，问题依旧；手动删除 `.openfeel/dev/guard-state.json` 可临时恢复，但用户认为这不合理。

---

## 2. 证据

### 2.1 宿主标记（新）

`~/.openfeel/adapter-host-dsh.json`（本地 01:08:28 写入）：

```json
{
  "adapter": "dsh",
  "version": "1.5.11",
  "loaded_hash": "9aecb20f4624723610915b1b6091c48fa2146b949ae246bc61fb103b576dbe1f",
  "started_at": "2026-11-04T17:08:28.342Z"
}
```

### 2.2 全局门禁状态（新，正确）

`%TEMP%\openfeel-guard\guard-state.json`：

```json
{
  "schemaVersion": 1,
  "registered": true,
  "hostStartedAt": "2026-11-04T17:08:28.342Z",
  "loaded_hash": "9aecb20f4624723610915b1b6091c48fa2146b949ae246bc61fb103b576dbe1f",
  "pid": 44056,
  "cwd": null,
  "cwdSource": "unresolved",
  "degraded": true,
  "updatedAt": "2026-11-04T17:08:28.345Z"
}
```

### 2.3 工作区门禁状态（旧，问题所在）

`<project>/.openfeel/dev/guard-state.json`：

```json
{
  "schemaVersion": 1,
  "registered": true,
  "hostStartedAt": "2026-11-04T15:10:19.028Z",   // ← 旧宿主启动时间（本地 11-04 23:10）
  "loaded_hash": "9aecb20f4624723610915b1b6091c48fa2146b949ae246bc61fb103b576dbe1f",
  "pid": 44056,                                   // ← 已是新进程
  "resolvedAt": "2026-11-04T17:21:44.835Z",
  "updatedAt": "2026-11-04T17:21:44.836Z"
}
```

关键矛盾：

- `pid` 已是当前进程 44056（说明文件确实被当前进程写过）；
- `loaded_hash` 与宿主标记一致（不是 `loaded-hash` 失配）；
- 但 `hostStartedAt` 停留在 **15:10:19Z**，早于宿主标记的 **17:08:28Z**，于是被判 `host-started-at`。

全局状态是对的，工作区状态是旧的——重启并没有把工作区的 `hostStartedAt` 刷新到当前宿主。

---

## 3. 根因分析（代码级）

### 3.1 判定规则

`lib/adapters/guard-state.js`：

```js
const stateAt = Date.parse(state.hostStartedAt);
const markerAt = Date.parse(hostMarker.started_at);
if (Number.isFinite(stateAt) && Number.isFinite(markerAt) && stateAt < markerAt) {
  return { reason: 'host-started-at' };
}
```

### 3.2 状态合并顺序

`lib/adapters/guard-state.js`（`writeGuardState`）：

```js
const globalRaw = readStateFile(globalGuardStatePath());
const workspaceRaw = isGlobal ? null : readStateFile(file);
const seed = globalRaw === null ? {} : /* 只取 REGISTRATION_KEYS */;
const next = {
  ...seed,             // 全局注册段（本轮宿主的 hostStartedAt / loaded_hash / pid）
  ...(workspaceRaw ?? {}),  // ← 旧工作区状态，hostStartedAt=15:10 覆盖 seed
  ...patch,            // 决策 patch：lastDecision / cwd / pid / loaded_hash… 但不含 hostStartedAt
  ...
};
```

- 决策 patch 会更新 `pid`、`lastDecision`、`cwd`、`resolvedAt`，所以工作区里的 `pid=44056`；
- 但 patch **不含 `hostStartedAt`**，于是 `workspaceRaw` 的旧值 15:10 保留下来；
- `seed` 虽然是新的 17:08，但它在展开顺序里位于 `workspaceRaw` **之前**，被覆盖。

### 3.3 启动期注册为什么没写到工作区

`lib/index.js`（`apply()` 启动阶段）：

```js
hostMarker = writeHostMarker({ adapterRoot, version: adapterVersion });
...
writeGuardState(config.cwd ?? null, {
  ...
  hostStartedAt: hostMarker?.started_at ?? new Date().toISOString(),
  loaded_hash: hostMarker?.loaded_hash ?? '',
  pid: process.pid,
});
```

启动时 `config.cwd` 为 `undefined`（profile 未固定 cwd），注册段写入 **全局临时文件**（`cwd:null, cwdSource:"unresolved"`）。
等会话解析出工作区后，第一次判定才写工作区；此时走的是 3.2 的合并路径，旧的 `hostStartedAt` 胜出。

### 3.4 结果

工作区文件的 `hostStartedAt` **永远不会**被刷新到当前宿主启动时间：

- 每次重启，宿主标记的 `started_at` 前进；
- 工作区 `hostStartedAt` 仍停在第一次写入时的值；
- 差距越拉越大，`stale` 永久为 true；
- 重启无法自愈，只有删除工作区状态文件（让 `workspaceRaw=null`，用全局 seed 重建）才能恢复。

---

## 4. 复现步骤

1. profile 不固定 cwd（`config.cwd` 未定义）。
2. 启动 DSH，打开本项目会话；执行任意 `feel_*` 调用，使工作区 `guard-state.json` 生成。
3. 完全退出 DSH 并重新启动。
4. 在同一项目会话执行 `feel_status`：
   - 期望 `stale=false`；
   - 实际 `stale=true`，`staleReason="host-started-at"`；
   - 对比全局状态 `hostStartedAt`（新）与工作区状态（旧）。
5. 后续任何 `advance/stage/req/op` 写入均被 leader 拒绝（`guard-stale`）。

---

## 5. 期望行为

- **完全重启宿主后，同一项目会话的首次 `feel_*` 调用应自愈为 `stale=false`**，无需用户或 agent 手动删除任何文件。
- `loaded-hash` 失配（适配器文件已更新但宿主未重启）仍应保持 `stale=true`——这条规则本身是正确的。
- `host-started-at` 判定不应把“当前进程自己写出、但基于旧种子”的状态误判为旧进程；`pid` 已经能证明状态来自当前进程。
- 不允许通过手改 `guard-state.json`（HMAC）绕过；也不应把“删状态文件”作为官方操作步骤。

---

## 6. 建议的解决方向（供 OpenFeel 评估，非强制实现）

### 方向 A：修正合并顺序（最小）

注册键以当前进程为准，工作区旧值不覆盖：

```js
const next = {
  ...(workspaceRaw ?? {}),
  ...seed,          // hostStartedAt / loaded_hash / pid 用当前注册段
  ...patch,
  ...
};
```

`REGISTRATION_KEYS` 只包含注册键，不会丢 `lastDecision`/`cwd`。

### 方向 B：决策写入补注册键

在每次 `writeGuardState` 的 patch 中带入当前宿主的
`hostStartedAt`（宿主标记 `started_at`）、`loaded_hash`、`pid`，保证工作区状态与当前进程同源。
需要把 `hostMarker` 传进 guard 判定/写入链路，或缓存到模块级。

### 方向 C：放宽 `host-started-at` 语义

`pid` 与当前进程一致 + `loaded_hash` 与宿主标记一致时，状态已可证明由当前进程写出；
`hostStartedAt < marker.started_at` 只说明“注册段是在标记刷新前写的”，不说明是旧进程。
可考虑：

- 仅在 `pid` 不匹配时才判 stale；或
- 比较宿主身份（pid + started_at）而不是单调时间；或
- 对“同一进程 + 同一 hash”给出 `stale=false`，并把时间差异降级为 warning。

### 方向 D：受支持的自愈/修复入口

提供 `openfeel setup --repair-guard` 或 `feel_verify action=health --fix` 之类的受支持命令：
从当前宿主标记重写注册段（保留 `lastDecision`/`cwd`），记录审计，不要求删文件。
文档明确“重启后应自愈；若未自愈，运行 repair”。

### 方向 E：回归测试

- 启动时 cwd 未解析 → 全局注册 → 之后工作区首次写入：`hostStartedAt` 应为当前宿主；
- 工作区已有旧 `hostStartedAt` + 全局新注册：判定结果应为 `stale=false`；
- 适配器文件变更（`loaded_hash` 不同）：仍应为 `stale=true`；
- 完全重启模拟：`stale` 不因 `host-started-at` 误报；
- 旧状态缺少 `pid`/HMAC 异常：维持现有 fail-closed。

---

## 7. 验收标准（建议）

1. 在“profile 未固定 cwd”的场景下，完全重启 DSH 后，同一项目会话的 `feel_status.guard.stale === false`；
2. 不删除、不手改任何 `guard-state.json`；
3. `loaded-hash` 失配场景保持 `stale=true`；
4. 工作区状态仍保留 `lastDecision` 与当前 cwd；
5. 有自动化回归测试覆盖上述场景。

---

## 8. 当前环境

| 项 | 值 |
|---|---|
| 项目 | `C:\Users\Liuary\Dev\Mine\Unreal\CultivationOpenRealm` |
| 适配器 | `openfeel-dsh` 1.5.11 |
| 适配器源码 | `C:\Users\Liuary\Dev\Mine\AI\OpenFeel\dsh\lib`（安装目录符号链接指向） |
| DSH 进程 | PID 44056，本地 2026-11-05 01:08:26 启动 |
| 宿主标记 | `started_at = 2026-11-04T17:08:28.342Z` |
| 全局状态 | `hostStartedAt = 2026-11-04T17:08:28.342Z`（正确） |
| 工作区状态 | `hostStartedAt = 2026-11-04T15:10:19.028Z`（旧） |
| 现象 | `staleReason = host-started-at`，写入全拒 |

---

## 9. 解决记录（2026-11-05）

OpenFeel 更新后重新启动 DSH，`feel_status` 返回：

```json
{
  "guard": {
    "stale": false,
    "signature": "valid",
    "hostStartedAt": "2026-11-05T14:13:04.022Z",
    "loaded_hash": "a8d3d1221f7195e162b33bf0b336314be27988ea6a3e015d03cf62a478f59407",
    "pid": 7156,
    "source": "workspace"
  }
}
```

工作区状态 `hostStartedAt` 已刷新到当前宿主启动时间，`loaded_hash` 为新适配器指纹；
未删除任何 `guard-state.json`，写入（`task/op/req/advance`）全部恢复。
本报告保留为问题归档。

## 10. 备注

- 本报告只描述 OpenFeel 适配器的门禁状态问题；同会话中的 UI 整改与 CORUI 插件崩溃修复均已完成并验证，与本案无关。
- 在 OpenFeel 给出方案前，我不会删除或手改 `guard-state.json`，也不会通过 `--force` 绕过门禁。
