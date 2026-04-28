import Log from "../Utils/Log/Log";
import LogType from "../Utils/Log/LogType";
import UISingle from "../Utils/UISingle/UISingle";
import UISingleType from "../Utils/UISingle/UISingleType";
import GameManagerBase from "./GameManagerBase";
import UIManager from "./UIManager";

/** 游戏加载界面路径 */
const Asset_GameLoadingPath = `/Script/UMGEditor.WidgetBlueprint'/Game/UI/GameLoading/UI_GameLoading.UI_GameLoading_C'`;

class GameLoadingManager extends GameManagerBase {
    //#region Instance
    private static _Instance: GameLoadingManager = null;
    public static get Instance(): GameLoadingManager {
        if (this._Instance == null)
        {
            this._Instance = new GameLoadingManager();
        }
        return this._Instance;
    }
    //#endregion Instance

    //#region 属性

    //#endregion 属性

    //#region 生命周期

    public override Init(): void {
        super.Init();

    }

    public override Deinit(): void {
        super.Deinit();
    }

    //#endregion 生命周期

    //#region 方法
    public LoadingGame(): void {
        Log.Log(LogType.UI, `[GameLoadingManager][LoadingGame]`);

        UIManager.Instance.PushUI(Asset_GameLoadingPath);

    }
    //#endregion 方法
}

export default GameLoadingManager;