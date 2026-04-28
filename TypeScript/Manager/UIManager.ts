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

        CppSubSystem.PushUI(WindowClassPath,  (Success: boolean, WindowName: string, Window: UE.CORWindow)=>{
            Log.Log(LogType.UI, `[UIManager][PushUI] PushUI ${WindowName} Success: ${Success}`);
            if (Success && Window) {
                const FuncName = `OnScriptInit`;
                let InitFunc = Window[FuncName];
                if (InitFunc) {
                    // @ts-ignore
                    InitFunc.call(Window, Single);
                } else {
                    Log.Error(LogType.UI, `[UIManager][PushUI] ${WindowName} not has init function ${FuncName}.`);
                }
            }
        }, WindowName, PushLayer, bIsAsync);
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