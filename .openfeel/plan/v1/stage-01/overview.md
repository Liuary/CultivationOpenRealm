# v1.1.0-stage-01

## 目标与范围

把 `Plugins/Puerts` 从当前 1.0.8 源码升级到上游 `Unreal_v1.0.9`，保留现有 V8 9.4.146.24，完成 API 适配、Typing 重新生成与编译验证。改：`Source/`、`Typing/`、`Content/JavaScript/`、`PuertsEditor/`、`Resources/`、`Puerts.uplugin`（必要时含项目侧签名适配）；不改 `ThirdParty/`（含 `v8_9.4.146.24`）、`Binaries/`、`Intermediate/`、任何 `.uasset`。

## 关键现状

- v1.0.9 已下载核对：相对当前版本 **29 个源文件实质改动**——UE5.6 兼容、`JsObject` 的 `THREAD_SAFE` 处理、`UEDataBinding` 的 `is_ue_container`、`TypeScriptGeneratedClass` 重定向、d.ts 生成修复；并**移除 `JSAnimGeneratedClass.*`**。
- `Puerts.uplugin` 的 `1.0.5` 是陈旧元数据：源码已有 v1.0.7/1.0.8 特征（`V11_8_172`、`WITH_V8_BYTECODE`、`EditorOnly`、`WithWebsocket`），无 `UE_5_6_OR_LATER`。版本判定看内容特征，升级后顺手改成 1.0.9。
- V8 走 `9.4.146.24`（`JsEnv.Build.cs` 的 `UseV8Version`；`ThirdParty/v8_9.4.146.24/Lib/Win64DLL` 动态库齐全）；v1.0.9 移除的是 10.6.194，本项目不受影响。
- `.gitignore` 排除 `ThirdParty/v8_*/{Bin,Lib}`、`ThirdParty/Library` 与 `*.lib/*.dll`：**这些二进制不在 git 里**，回滚只能靠文件系统备份。
- 本机无 UE 5.5（仅 UE_5.8），历史日志为 5.5.4 —— 编译门禁需先确认引擎，不能用 5.8 结果替代。
- 沙箱内 git-lfs 的 `filter-process` 报 `Win32 error 5`，git 命令需带 `-c filter.lfs.process= -c filter.lfs.clean=cat -c filter.lfs.smudge=cat`。

## 具体实施步骤

1. **备份留痕**：`robocopy Plugins/Puerts Saved/Backup/Puerts-before-v1.0.9 /MIR /XD Binaries Intermediate`；`ThirdParty` 全量 SHA256 清单 → `Saved/Upgrade/thirdparty-before.txt`；`git rev-parse HEAD` 与 `git status -- Plugins/Puerts` → `baseline-git.txt`。
2. **差异清单**：对 `Source/ Typing/ Content/JavaScript/ PuertsEditor/ Resources/ Puerts.uplugin` 逐文件哈希，输出 `Saved/Upgrade/diff.txt`（ADDED/CHANGED/MISSING；MISSING = 1.0.8 残留，必须删除）。确认 `JSAnimGeneratedClass.*` 已从本地移除。
3. **替换**：删除上述旧目录后整目录复制新包（避免旧文件残留）；覆盖 `Puerts.uplugin`，`Version/VersionName/FriendlyVersion` 改 `1.0.9`、`FileVersion` 保持 3；`ThirdParty` 不复制不删；删 `Plugins/Puerts/{Binaries,Intermediate}`；保留本地唯一改动 `Source/CSharpParamDefaultValueMetas/*.ubtplugin.csproj.props`。
4. **API 适配**：核对新 `JsEnv.Build.cs` 的 `v8LibSuffix` 仍产出 `v8_9.4.146.24`；grep `FJsObject`、`JsEnv.h`、`UEDataBinding`、`DefineClass`、`MakeFunction`、`Eval`，按编译错误最小改；重点看 `JsObject` 的 THREAD_SAFE（本项目 `ThreadSafe=false`）、`UEDataBinding::is_ue_container`、`TypeScriptGeneratedClass` 重定向对 d.ts 的影响。每条适配记入 `Saved/Upgrade/api-adapt.md`。
5. **重生成 Typing**：编辑器控制台 `Puerts.Gen FULL`（或 `UnrealEditor-Cmd.exe <proj> -ExecCmds="Puerts.Gen FULL" -Unattended`）；注意 `JsEnv.Build.cs` 会先复制插件 Typing 到项目根，顺序是「先编译再生成」。
6. **编译冒烟**：`Build.bat CultivationOpenRealmEditor Win64 Development -Project="<proj>" -WaitMutex` → `Result: Succeeded`；随后 PIE 打开 `UI_GameLoading`、进度推进、关闭不崩。

## 验收标准

| # | 判据 | 验证 |
|---|------|------|
| A1 | 备份/清单齐备 | 备份目录、`thirdparty-before.txt`、`diff.txt`、`baseline-git.txt` 均存在且非空 |
| A2 | 与 v1.0.9 逐文件一致 | 5 个目录 + uplugin 的 SHA256 差异为 0（本地保留项需登记） |
| A3 | ThirdParty 零改动 | 升级后重算清单与 before 相同；`v8_9.4.146.24/{Inc,Lib}` 均在 |
| A4 | 元数据正确且无残留 | uplugin 三处版本 1.0.9；`JSAnimGeneratedClass.*` 不存在 |
| A5 | 编译通过 | `Result: Succeeded`、0 error、无 v8.dll/wee8.lib 缺失 |
| A6 | Typing 可用 | `Puerts.Gen FULL` 成功；`Typing/ue/ue.d.ts` 含 `CORUIManager`/`CORWindow`；`npx tsc --noEmit -p tsconfig.json` 退出 0 |
| A7 | 运行不回归 | 编辑器/PIE 日志无 `LogJsEnv: Error`/`Fatal error`/`Ensure`；窗口开关正常 |

A5–A7 依赖 UE 5.5.4；环境缺失则阶段标 blocked，不用 5.8 顶替。

## 风险与回滚

- **网络/资产名不符**：按 tag `Unreal_v1.0.9` clone 兜底；仍失败记 blocked，备份不动。
- **新包自带 ThirdParty 或 `v8LibSuffix` 变化**：坚持保留 `9.4.146.24`，必要时回改 Build.cs 路径并记录。
- **覆盖本地魔改**：备份 + `git status -- Plugins/Puerts` 复核；NuGet props 单独还原。
- **旧 OBJ/PDB 残留**：删 `Binaries/Intermediate` 重编（备份已排除）。
- **d.ts 生成差异致 tsc 失败**：保留旧 Typing 对照，只做签名适配，不重构业务；差异交 reviewer。
- **本机缺 UE 5.5**：开工前确认引擎，缺则 blocked。
- **回滚**：`robocopy Saved/Backup/Puerts-before-v1.0.9 Plugins/Puerts /MIR` → 删 `Binaries/Intermediate` → 重编；核对 ThirdParty 指纹与 `git status` 回到基线。
