# 生成 Puerts TypeScript 声明文件（Typing/**.d.ts）
#
# 背景：Typing/ue/*.d.ts、Typing/ue/ue_bp.d.ts、Typing/cpp/index.d.ts 都是生成物，
#       不要手工编辑；C++ 的 UFUNCTION / UCLASS 变化后重跑本脚本即可。
#
# 用法（仓库根目录）：
#   pwsh Tools/TypeScript/gen-typings.ps1
#   pwsh Tools/TypeScript/gen-typings.ps1 -EditorCmd "D:\UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
#   pwsh Tools/TypeScript/gen-typings.ps1 -SkipTsc
#
# 实现说明：
#   Puerts 的生成入口是编辑器控制台命令 `Puerts.Gen`（DeclarationGenerator 模块）。
#   该命令在编辑器模块加载后才注册，`-ExecCmds`（引擎初始化早期执行）拿不到它；
#   因此用 -ExecutePythonScript 跑 gen_typings.py，在 Slate tick 回调里执行命令并退出。
#
# 注意：运行前请关闭正在使用的 UE 编辑器（项目锁会阻止 Cmd 实例启动）。

param(
    [string]$EditorCmd = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    [int]$TimeoutSec = 900,
    [switch]$SkipTsc
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$ProjectFile = Join-Path $RepoRoot "CultivationOpenRealm.uproject"
$PythonScript = Join-Path $PSScriptRoot "gen_typings.py"
$UeDts = Join-Path $RepoRoot "Typing\ue\ue.d.ts"

if (-not (Test-Path $EditorCmd)) { throw "找不到 UnrealEditor-Cmd.exe：$EditorCmd（可用 -EditorCmd 指定）" }
if (-not (Test-Path $ProjectFile)) { throw "找不到项目文件：$ProjectFile" }
if (-not (Test-Path $PythonScript)) { throw "找不到生成脚本：$PythonScript" }
if (-not (Test-Path $UeDts)) { throw "找不到声明文件：$UeDts" }

if (Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue) {
    throw "检测到 UE 编辑器正在运行，请先关闭（项目锁会阻止生成实例启动）"
}

$Before = (Get-Item $UeDts).LastWriteTimeUtc
Write-Host "[gen-typings] $UeDts"

$EditorArgs = @(
    "`"$ProjectFile`"",
    "-ExecutePythonScript=`"$PythonScript`"",
    "-unattended", "-nosplash", "-nopause", "-noP4"
)

$Proc = Start-Process -FilePath $EditorCmd -ArgumentList $EditorArgs -PassThru `
    -RedirectStandardOutput (Join-Path $env:TEMP "gen-typings.out") `
    -RedirectStandardError (Join-Path $env:TEMP "gen-typings.err")

if (-not $Proc.WaitForExit($TimeoutSec * 1000)) {
    $Proc.Kill()
    throw "生成超时（$TimeoutSec 秒），已终止编辑器"
}
if ($Proc.ExitCode -ne 0) {
    throw "生成进程退出码 $($Proc.ExitCode)，详见 $env:TEMP\gen-typings.out"
}

$After = (Get-Item $UeDts).LastWriteTimeUtc
if ($After -le $Before) {
    throw "Puerts.Gen 未更新 $UeDts，请检查 $env:TEMP\gen-typings.out"
}

Write-Host "[gen-typings] $UeDts ($((Get-Item $UeDts).Length) bytes) updated"

if (-not $SkipTsc) {
    $Tsc = Join-Path $env:TEMP "corui-tsc\node_modules\typescript\lib\tsc.js"
    if (Test-Path $Tsc) {
        & node $Tsc -p (Join-Path $RepoRoot "tsconfig.json")
        if ($LASTEXITCODE -ne 0) { throw "tsc 校验失败（exit=$LASTEXITCODE）" }
        Write-Host "[gen-typings] tsc ok"
    }
    else {
        Write-Host "[gen-typings] 未找到本地 tsc，跳过编译校验"
    }
}

Write-Host "[gen-typings] done"

