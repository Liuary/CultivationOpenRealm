"""在编辑器内执行 Puerts.Gen 生成 TypeScript 声明，并退出编辑器。

由 Tools/TypeScript/gen-typings.ps1 通过 -ExecutePythonScript 调用；
也可手动：UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="Tools/TypeScript/gen_typings.py" -unattended -nosplash -nopause

说明：Puerts.Gen 是编辑器控制台命令，注册发生在编辑器模块加载之后，
因此这里用 slate tick 回调在编辑器就绪后再执行，避免 -ExecCmds 阶段命令尚不存在。
"""

import unreal

_started = {"done": False}


def _generate_and_quit(_delta_seconds):
    if _started["done"]:
        return False
    _started["done"] = True

    unreal.log("[gen-typings] running Puerts.Gen")
    # 编辑器上下文里没有 World，传 None 由引擎使用当前编辑器世界
    unreal.SystemLibrary.execute_console_command(None, "Puerts.Gen")
    unreal.log("[gen-typings] Puerts.Gen returned, quitting")
    unreal.SystemLibrary.execute_console_command(None, "Quit")
    return False


unreal.register_slate_post_tick_callback(_generate_and_quit)
unreal.log("[gen-typings] python started, waiting for first tick")
