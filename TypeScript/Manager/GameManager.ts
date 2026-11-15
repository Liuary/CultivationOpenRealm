import * as UE from 'ue';
import CorStart from "../CorStart";
import Log from "../Utils/Log/Log";
import LogType from "../Utils/Log/LogType";
import GameManagerBase from "./GameManagerBase";
import UIManager from "./UIManager";
import GameStartManager from "./GameStartManager";
import DataManager from "./DataManager";
import { GameEvents } from "../Game/Events/GameEvents";

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

    /** 是否已初始化（每个 UI 会话恰好初始化一次） */
    private bInited: boolean = false;
    /** C++ 事件是否已绑定（换图后无需重复绑定） */
    private bBound: boolean = false;
    /** UI 会话计数（每次初始化 +1） */
    private Session: number = 0;
    /** 诊断监听器的取消订阅句柄，Deinit 时释放，避免换图重入累积 */
    private TableCachedUnsub: (() => void) = null;

    /** 初始化 */
    Init(): void {

        // 幂等：晚订阅补偿与正常 OnUIManagerInit 可能都会触发
        if (this.bInited) {
            Log.Log(LogType.Game, `[GameManager][Init] already initialized, skip.`);
            return;
        }

        this.bInited = true;

        try {
            super.Init();

            // 防御：若上一次会话异常中断，先释放旧诊断监听，避免重复订阅
            if (this.TableCachedUnsub) {
                this.TableCachedUnsub();
                this.TableCachedUnsub = null;
            }

            // 诊断监听必须先于 InitManagers：GameStart 初始化时会立刻缓存数据表并派发事件
            this.TableCachedUnsub = GameEvents.On('data:table-cached', (Payload) => {
                Log.Log(LogType.Game, `[GameManager][Event] data:table-cached key=${Payload.key} rows=${Payload.rows}`);
            });

            this.InitManagers();

            this.Session++;

            GameEvents.Emit('ui:ready', { session: this.Session });
        }
        catch (Error) {
            // 初始化抛错时回滚标志，避免管理器被永久卡在“已初始化”但无监听的状态
            this.bInited = false;
            throw Error;
        }
    }

    /** 反初始化 */
    Deinit(): void {
        if (!this.bInited) {
            return;
        }

        this.bInited = false;

        this.DeinitManagers();

        // 先释放诊断监听，再广播停止：避免 ui:stopped handler 重入 Init 时重复订阅
        if (this.TableCachedUnsub) {
            this.TableCachedUnsub();
            this.TableCachedUnsub = null;
        }

        GameEvents.Emit('ui:stopped', { session: this.Session });

        // 注意：这里不解除 C++ 事件绑定。
        // 换图/重建 UI 会话时 C++ 会再次广播 OnUIManagerInit，解绑会导致新会话永远收不到初始化。
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

        if (!this.bBound) {
            CppSubSystem.OnUIManagerInit.Add(this.Init);
            CppSubSystem.OnUIManagerDeinit.Add(this.Deinit);
            this.bBound = true;
        }

        // 晚订阅补偿：UI 可能已经就绪（错过唯一一次 OnUIManagerInit），这里立即初始化
        if (CppSubSystem.IsUIInitialized()) {
            Log.Log(LogType.Game, `[GameManger][BindCppEvent] UI already ready, init now.`);
            this.Init();
        }
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