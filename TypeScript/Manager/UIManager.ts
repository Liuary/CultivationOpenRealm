import * as UE from 'ue';
import GameManager from "./GameManager";
import Log from "../Utils/Log/Log";
import LogType from "../Utils/Log/LogType";
import GameManagerBase from "./GameManagerBase";
import UISingle from "../Utils/UISingle/UISingle";
import UISingleType from "../Utils/UISingle/UISingle";

class UIManager extends GameManagerBase{
    constructor() {
        super();
        
        Log.Log(LogType.UI, `[UIManager][constructor] constructed.`);

        this.Init = this.Init.bind(this);
        this.Deinit = this.Deinit.bind(this);
    }

    private static _Instance: UIManager = null;
    public static get Instance(): UIManager {
        if (this._Instance == null) 
        {
            this._Instance = new UIManager();
        }
        return this._Instance;
    }

    public override Init(): void {
        super.Init();
        
        Log.Log(LogType.UI, `[UIManager][Init] initialized.`);
    }

    public override Deinit(): void {
        Log.Log(LogType.UI, `[UIManager][Deinit] deinitialized.`);
        
        super.Deinit();
    }
    
    /**
     * UI 入栈
     * @param WindowClassPath UI 类路径
     * @param Single UI 通信数据
     * @param WindowName UI 命名
     * @param PushLayer Push 的层
     * @param bIsAsync 是否异步
     * @returns 
     */
    public PushUI(WindowClassPath: string, Single: UISingle = null, WindowName?: string /* = "None" */, PushLayer?: UE.EUILayerType /* = None */, bIsAsync?: boolean /* = true */) {
        let CppSubSystem = GameManager.Instance.GetCppSubsystem<UE.CORUIManager>(UE.CORUIManager.StaticClass());
        if (!CppSubSystem) {
            Log.Error(LogType.Game, `[UIManager][PushUI] CppSubSystem is null.`)
            return;
        }

        // 传入的数据载荷统一为 UISingle 实例：C++ 在 Open/Show 之前回调，
        // 这里完成 OnScriptInit 的数据初始化，避免 null 载荷被窗口直接解引用。
        const Payload: UISingle = Single ?? new UISingle();

        // Puerts 会把 JS undefined 转成字符串 FName：显式传 "None"（等价 NAME_None），
        // 才能让 C++ 回退到 CDO 里的窗口命名。
        const Name = WindowName === undefined || WindowName === null ? "None" : WindowName;

        CppSubSystem.PushUI(WindowClassPath,  (Success: boolean, WindowName: string, Window: UE.CORWindow)=>{
            Log.Log(LogType.UI, `[UIManager][PushUI] PushUI ${WindowName} Success: ${Success}`);
            if (!Success || !Window) {
                return;
            }

            const FuncName = `OnScriptInit`;
            const InitFunc = (Window as any)[FuncName];

            // 没有钩子是合法情况（纯蓝图窗口不需要脚本初始化）
            if (InitFunc === undefined || InitFunc === null) {
                Log.Log(LogType.UI, `[UIManager][PushUI] ${WindowName} has no ${FuncName}, skip.`);
                return;
            }

            // 非函数钩子属于契约错误：回滚，避免留下半初始化的激活
            if (typeof InitFunc !== "function") {
                Log.Error(LogType.UI, `[UIManager][PushUI] ${WindowName}.${FuncName} is not a function.`);
                this.RollbackActivation(Window, WindowName);
                return;
            }

            try {
                // 初始化失败必须回滚：此时 C++ 尚未 Open/Show，窗口不会以半初始化状态出现
                InitFunc.call(Window, Payload);
            } catch (Error) {
                Log.Error(LogType.UI, `[UIManager][PushUI] ${WindowName}.${FuncName} threw: ${Error}`);
                this.RollbackActivation(Window, WindowName);
            }
        }, Name, PushLayer, bIsAsync);
    }

    /** 回滚一次脚本初始化失败的激活 */
    private RollbackActivation(Window: UE.CORWindow, WindowName: string): void {
        try {
            Window.CloseUI(true);
            Log.Log(LogType.UI, `[UIManager][PushUI] ${WindowName} activation rolled back.`);
        } catch (Error) {
            Log.Error(LogType.UI, `[UIManager][PushUI] rollback ${WindowName} failed: ${Error}`);
        }
    }

    /**
     * UI 出栈
     * @param WindowName 
     * @param PushLayer 
     * @param bPopSelf 
     * @returns 
     */
    public PopUI(WindowName: string, PushLayer: UE.EUILayerType, bPopSelf?: boolean /* = true */) {
        let CppSubSystem = GameManager.Instance.GetCppSubsystem<UE.CORUIManager>(UE.CORUIManager.StaticClass());
        if (!CppSubSystem) {
            Log.Error(LogType.Game, `[UIManager][PopUI] CppSubSystem is null.`)
            return;
        }

        CppSubSystem.PopUI(WindowName, PushLayer, bPopSelf);
    }

    /**
     * 获取 UE World
     */
    public GetWorld(): UE.World {
        let CppSubSystem = GameManager.Instance.GetCppSubsystem<UE.CORUIManager>(UE.CORUIManager.StaticClass());
        if (!CppSubSystem) {
            Log.Error(LogType.Game, `[UIManager][GetWorld] CppSubSystem is null.`)
            return null;
        }
        
        return CppSubSystem.GetWorld();
    }
}

export default UIManager;