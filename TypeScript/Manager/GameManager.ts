import * as UE from 'ue';
import CorStart from "../CorStart";
import Log from "../Utils/Log/Log";
import LogType from "../Utils/Log/LogType";
import GameManagerBase from "./GameManagerBase";
import UIManager from "./UIManager";
import GameStartManager from "./GameStartManager";
import DataManager from "./DataManager";

/** 
 * Ts 侧游戏管理器 
 */
class GameManager extends GameManagerBase {

    private static _Instance: GameManager = null;
    public static get Instance(): GameManager {
        if (this._Instance == null) {
            this._Instance = new GameManager();
        }
        return this._Instance;
    }

    constructor() {
        super();
        this.Init = this.Init.bind(this);
        this.Deinit = this.Deinit.bind(this);
    }

    /** 初始化 */
    Init(): void {

        super.Init();

        this.InitManagers();
    }

    /** 反初始化 */
    Deinit(): void {
        this.DeinitManagers();
        this.UnBindCppEvent();

        super.Deinit();
    }

    /** Manager们的初始化 */
    private InitManagers(): void {
        UIManager.Instance.Init();
        GameStartManager.Instance.Init();
        DataManager.Instance.Init();
    }

    /** Manager们的反初始化 */
    private DeinitManagers(): void {

        DataManager.Instance.Deinit();
        GameStartManager.Instance.Deinit();
        UIManager.Instance.Deinit();
    }

    /** 绑定C++事件 */
    BindCppEvent(): void {
        let CppSubSystem = this.GetCppSubsystem<UE.CORUIManager>(UE.CORUIManager.StaticClass());
        if (!CppSubSystem) {
            Log.Error(LogType.Game, "[GameManger][BindCppEvent] CppSubSystem is null.")
            return;
        }

        CppSubSystem.OnUIManagerInit.Add(this.Init);
        CppSubSystem.OnUIManagerDeinit.Add(this.Deinit);
    }

    /** 解除C++事件绑定 */
    UnBindCppEvent(): void {
        let CppSubSystem = this.GetCppSubsystem<UE.CORUIManager>(UE.CORUIManager.StaticClass());
        if (!CppSubSystem) {
            Log.Error(LogType.Game, `[GameManger][UnBindCppEvent] CppSubSystem is null.`);
            return;
        }

        CppSubSystem.OnUIManagerInit.Remove(this.Init);
        CppSubSystem.OnUIManagerDeinit.Remove(this.Deinit);
    }

    /** 获取C++的UI管理器子系统 */
    public GetCppSubsystem<T extends UE.GameInstanceSubsystem>(ClassRef: UE.Class): T {
        // 获取当前世界的实例
        let Context = CorStart.Instance.GetGameInstance();
        if (!Context) {
            Log.Error(LogType.Game, `[GameManger][GetCppSubsystem] Context is null.`);
            return null;
        }

        // 获取子系统
        let Subsystem = UE.SubsystemBlueprintLibrary.GetGameInstanceSubsystem(Context, ClassRef);
        if (!Subsystem) {
            Log.Error(LogType.Game, `[GameManger][GetCppSubsystem] Subsystem is null.`);
            return null;
        }

        return Subsystem as T;
    }
}

export default GameManager;