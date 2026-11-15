# 把 CORUI 移植到新项目（含给 AI 的任务书）

> 目的：在新 UE 项目里复用这套 UI 框架。
> 适用对象：开发者本人，或**另一个项目的 AI 编码助手**（把第 6 节的任务书整段发给它即可）。

---

## 1. 先理解移植边界（最容易漏的一点）

**C++ 插件只覆盖"能力层"，TypeScript 基础层不在插件里。**

| 类别 | 位置 | 是否随插件走 | 说明 |
|---|---|---|---|
| C++ 插件 | `Plugins/CORUI` | ✅ 整目录复制 | 3 模块 + `Config/DefaultCORUI.ini` + `Content/Config/DA_UILayerTypeZOrderConfigData.uasset` + `Tools/watch-ts` |
| 桥接插件 | `Plugins/Puerts` | ✅ 整目录复制 | **V8 库不在仓库里**，需按 README 单独下载 |
| TS 基础层 | `TypeScript/Utils/**`、`Game/Events/**`、`Game/UI/ViewModel/**`、`Manager/{GameManager,GameManagerBase,UIManager,DataManager,TimeManager}.ts`、`CorStart.ts` | ❌ **要手动复制** | 框架的一半逻辑在这里 |
| TS 示例层 | `Game/UI/{GameStart,GameLoading,Test}/**`、`Manager/{GameStartManager,GameLoadingManager,TestUIManager}.ts` | 可选 | 可当参考，进新项目后可删 |
| 工程配置 | `.uproject`、`tsconfig.json`、`package.json`、`.gitignore`、`Typing/` | ❌ 需按新项目调整 | `Typing/` 建议重新生成 |
| 项目专属 | `Source/<Project>/`、`Config/`、`Content/` 业务资产 | 不迁移 | 新项目自己的 GameInstance 等 |

依赖关系：`CORUI.uplugin` 声明了 `Puerts` 与 `JsEnv` 两个依赖 → **Puerts 必须先到位**，否则插件加载失败。

---

## 2. 移植清单

### 2.1 必须复制

| # | 源 | 目标 | 注意 |
|---|---|---|---|
| 1 | `Plugins/CORUI/` | `<NewProject>/Plugins/CORUI/` | 整目录 |
| 2 | `Plugins/Puerts/` | `<NewProject>/Plugins/Puerts/` | 整目录（含 `Source`、`ThirdParty/Inc`） |
| 3 | `TypeScript/Utils/` | `<NewProject>/TypeScript/Utils/` | 含 `Event/`、`Log/`、`Mixin/`、`UISingle/`、`CommonUtil.ts`、`TypeUitls.ts` |
| 4 | `TypeScript/Game/Events/` | `<NewProject>/TypeScript/Game/Events/` | 事件契约，必须先建 |
| 5 | `TypeScript/Game/UI/ViewModel/` | `<NewProject>/TypeScript/Game/UI/ViewModel/` | ViewModel 基类 |
| 6 | `TypeScript/Manager/` 的 5 个核心 | `<NewProject>/TypeScript/Manager/` | `GameManager`、`GameManagerBase`、`UIManager`、`DataManager`、`TimeManager` |
| 7 | `TypeScript/CorStart.ts` | `<NewProject>/TypeScript/CorStart.ts` | 入口，需改 import |
| 8 | `Plugins/Puerts/ThirdParty/v8_9.4.146.24/{Inc,Lib,Bin}` | 同路径 | 本地下载，**不入库** |

### 2.2 可选（示例 / 测试套件）

| 源 | 说明 |
|---|---|
| `TypeScript/Game/UI/{GameStart,GameLoading,Test}/**` | 观澜测试套件与业务流程示例；想快速验证可整组搬 |
| `TypeScript/Manager/{GameStartManager,GameLoadingManager,TestUIManager}.ts` | 与上面配套 |
| `Content/UI/` 下的示例蓝图 | 仅作参考，新项目应自建 |

### 2.3 不要复制

- `Content/JavaScript/`（编译产物，重建即可）
- `Plugins/Puerts/ThirdParty/v8_*/Lib|Bin`（仓库里没有，本地库单独放）
- `.openfeel/`（本项目流程治理，与框架无关）
- `Output/`、`Saved/`、`Intermediate/`、`Binaries/`

---

## 3. 工程配置（新项目侧）

### 3.1 `.uproject` 启用插件

```json
{
  "Plugins": [
    { "Name": "CORUI", "Enabled": true },
    { "Name": "Puerts", "Enabled": true }
  ]
}
```

`ModelContextProtocol` 等本项目自用的编辑器插件可不带。

### 3.2 `tsconfig.json`

```json
{
    "compilerOptions": {
        "target": "esnext",
        "module": "commonjs",
        "experimentalDecorators": true,
        "useDefineForClassFields": false,
        "jsx": "react",
        "sourceMap": true,
        "typeRoots": ["Typing", "./node_modules/@types"],
        "outDir": "Content/JavaScript"
    },
    "include": ["TypeScript/**/*"]
}
```

### 3.3 `package.json`

```json
{
  "private": true,
  "scripts": {
    "build": "tsc -p tsconfig.json",
    "watch": "tsc -p tsconfig.json --watch",
    "typecheck": "tsc -p tsconfig.json --noEmit"
  },
  "devDependencies": { "typescript": "^5.6.3" }
}
```

### 3.4 `.gitignore` 必加

```
node_modules/
Content/JavaScript/
Plugins/Puerts/ThirdParty/v8_*/Lib/
Plugins/Puerts/ThirdParty/v8_*/Bin/
*.lib
*.dll
```

### 3.5 `Typing/`

两种方式：

1. 用 Puerts 的 "Generate Typing" 功能在新项目生成（**推荐**，类型与新版插件一致）
2. 从本项目复制 `Typing/{cpp,ffi,puerts,ue}`，再重新生成一次以对齐

### 3.6 MiXin 生成器设置

`Project Settings → Puerts Mixin Generator`：

- `PuertsMixinPath` = `TypeScript`（生成根目录）
- `PuertsImportListPathScope` = `Game`（Import 列表收集范围）

生成器产出的 Mixin 必须落在 `TypeScript/<PuertsImportListPathScope>/...` 下，否则不会被 `PuertsImportList.ts` 收集。

### 3.7 启动接线

- **不需要**改新项目的 GameInstance：JS 环境由 `UJsEnvManagerSubsystem`（CORUI 插件内）创建，入口模块名固定 `CorStart`
- `CorStart.ts` 末尾会 `GameManager.Instance.BindCppEvent()`，确保 `TypeScript/CorStart.ts` 被复制且 `PuertsImportList.ts` 存在（生成器会自动创建）

---

## 4. 最小验证（先跑通再谈业务）

1. 编译：`Build.bat <Project>Editor Win64 Development -Project=... -WaitMutex` → `Result: Succeeded`
2. TS：`npm install && npm run build` → exit 0
3. PIE 启动，日志应出现：
   - `JsEnv created` 或 `[CorStart][Start]`
   - `[UIManager]` 初始化日志
4. 建一个最简单的 `UCORWindow` 蓝图（`Content/UI/Demo/UI_Hello`），生成 Mixin，在 PIE 里：
   ```ts
   UIManager.Instance.PushUI("/Game/UI/Demo/UI_Hello.UI_Hello_C", new UISingle(), "Hello", UE.EUILayerType.Normal, true);
   ```
   日志出现 `[UIManager][PushUI] PushUI Hello Success: true` 即链路通
5. 框架自检：PIE 控制台 `CORUI.VerifyOwnership` → `RESULT=PASS`

---

## 5. 常见移植坑

| 现象 | 原因 | 处理 |
|---|---|---|
| 插件加载失败 `JsEnv could not be found` | Puerts 没放 / 版本不匹配 | 先放 Puerts，且 `UseV8Version` 与本地库一致 |
| 编译报找不到 v8 库 | 缺 `ThirdParty/v8_9.4.146.24` | 按 README 下载 |
| 日志有 `[CorStart]` 但 UI 不出现 | 没复制 `TypeScript/Manager/*` 或 `UIManager.ts` import 路径断 | 检查 `CorStart` 的 import 链 |
| 生成 Mixin 后 Import 列表为空 | 生成路径不在 `PuertsImportListPathScope` 下 | 改生成器设置 |
| `UE.CORUIManager` 类型不存在 | `Typing/ue` 未生成 | 重新 Generate Typing |
| 层级 ZOrder 全是 0 | 缺 `DA_UILayerTypeZOrderConfigData` | 该资产随插件 Content 走，确认已复制 |

---

## 6. 给新项目 AI 的任务书（整段复制）

> 以下内容可直接发给另一项目的 AI 编码助手。把 `<本仓库路径>` 替换成实际路径或 GitHub 地址。

```text
我要在新 UE 项目里接入 CORUI UI 框架。参考仓库：<本仓库路径>（GitHub: https://github.com/Liuary/CultivationOpenRealm ）。

请严格按仓库里的 docs/ai/port-to-new-project.md 执行，不要自己发挥目录结构。核心要求：

1. 复制 Plugins/CORUI 与 Plugins/Puerts 到本项目的 Plugins/ 下（整目录）。
2. 提醒我本地补 Puerts V8 库：Plugins/Puerts/ThirdParty/v8_9.4.146.24/{Inc,Lib,Bin}，
   下载地址见参考仓库 README「首次克隆必读」；库文件不进 git。
3. 复制 TypeScript 基础层（不要只复制插件就以为完事）：
   Utils/**、Game/Events/**、Game/UI/ViewModel/**、
   Manager/{GameManager,GameManagerBase,UIManager,DataManager,TimeManager}.ts、CorStart.ts。
   修正这些文件的相对 import 路径，保证能编译。
4. 给本项目加 tsconfig.json（outDir=Content/JavaScript，include=TypeScript/**/*）、
   package.json（typescript 依赖 + build/watch/typecheck 脚本）、
   .gitignore（node_modules、Content/JavaScript、V8 库）。
5. 在 .uproject 里启用 CORUI 与 Puerts 插件。
6. 用 Puerts 生成 Typing/（或复制参考仓库的 Typing 后重新生成）。
7. 设置 PuertsMixinGenerator：PuertsMixinPath=TypeScript，PuertsImportListPathScope=Game。
8. 先只做「最小可跑」：建一个继承 UCORWindow 的测试蓝图 UI_Hello，生成 Mixin，
   在 PIE 里通过 UIManager.Instance.PushUI 打开它并关掉，确认日志出现
   [UIManager][PushUI] Success: true。
9. 验收：Build 成功 + npm run build exit 0 + PIE 里 PushUI/PopUI 正常 + CORUI.VerifyOwnership 输出 RESULT=PASS。

约束：
- 不要手改 .uasset / .umap（二进制，用编辑器或脚本生成）。
- 不要提交 V8 库二进制。
- 不要复制参考仓库的 .openfeel/、Output/、Saved/、Intermediate/。
- 遇到 API 疑问先读参考仓库的 AGENTS.md 与 docs/ai/framework-usage.md，不要猜。
```

---

## 7. 移植后建议做的三件事

1. 改 `Config/DefaultCORUI.ini` 的 `LayerConfig → UILayerConfig` 重定向（新项目没有旧资产，可删）
2. 把 `DA_UILayerTypeZOrderConfigData` 的 ZOrder 调成新项目的视觉分层
3. 把 `TypeScript/Game/UI/Test/**`（观澜套件）删掉或保留作回归用例
