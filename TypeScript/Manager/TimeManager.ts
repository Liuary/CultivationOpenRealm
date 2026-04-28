import GameManagerBase from "./GameManagerBase";

class TimeManager extends GameManagerBase {

    //#region Instance

    private static _Instance: TimeManager = null;

    public static get Instance(): TimeManager {

        if (this._Instance == null) {

            this._Instance = new TimeManager();

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

    /** 
     * 获取当前时间 
     * @returns 当前时间（毫秒）
     * */
    public GetCurrentTime(): number {
        return Date.now();
    }

    //#endregion 方法
}

export default TimeManager;