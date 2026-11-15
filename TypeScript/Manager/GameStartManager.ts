import * as UE from 'ue';
import {CORCommonUtil} from 'ue';
import {$ref, $unref} from "puerts"
import GameManagerBase from "./GameManagerBase";
import UIManager from "./UIManager";
import UISingle from "../Utils/UISingle/UISingle";
import UI_GameStartSingleType from "../Game/UI/GameStart/UI_GameStartSingleType";
import GameManager from "./GameManager";
import Log from "../Utils/Log/Log";
import LogType from "../Utils/Log/LogType";
import DataManager from './DataManager';
import { PureData } from '../Utils/TypeUitls';
import UISingleType from '../Utils/UISingle/UISingleType';

/** 游戏开始的资产表格路径 */
const GameStartAssetTablePath = "/Script/Engine.DataTable'/Game/Data/GameStart/DT_GameStart.DT_GameStart'";
const GameStartAssetTableCacheKey = 'CacheKey_DT_GameStart';

/** 游戏启动的UI路径 */
const Asset_GameStartPath = `/Script/UMGEditor.WidgetBlueprint'/Game/UI/GameStart/UI_GameStart.UI_GameStart_C'`;


/** 游戏启动的管理器 */
class GameStartManager extends GameManagerBase {

    //#region Instance
    private static _Instance: GameStartManager = null;
    public static get Instance(): GameStartManager {
        if (this._Instance == null)
        {
            this._Instance = new GameStartManager();
        }
        return this._Instance;
    }
    //#endregion Instance

    //#region 属性

    /** 图片路径 */
    private ImageList: any[] = [];
    /** 文本路径 */
    private TextList: string[] = [];
    //#endregion 属性


    //#region 生命周期
    
    public override Init() {

        super.Init();

        this.LoadImageList();
    }
    
    public override Deinit(): void {
        super.Deinit();
    }

    //#endregion 生命周期

    //#region 方法
    
    StartGame(): void {
        let Single: UISingle = new UISingle();
        // Single.SetSingle(Number(UI_GameStartSingleType.ImageList), this.ImageList);
        Single.SetSingle(Number(UI_GameStartSingleType.ImageList), []);
        UIManager.Instance.PushUI(Asset_GameStartPath, Single);
    }
    
    LoadImageList(): void {
        // Log.Log(LogType.UI, 'LoadImageList');

        // 走 DataManager 的统一加载路径（加载 + 缓存 + data:table-cached 事件）
        if (!DataManager.Instance.LoadTableByPath(GameStartAssetTablePath, GameStartAssetTableCacheKey)) {
            Log.Error(LogType.UI, 'LoadImageList failed: table load error.');
            return;
        }

        this.ImageList = [];
        this.TextList = [];

        let RowList: PureData<UE.RowStructGameStart>[] = DataManager.Instance.GetAllRows<UE.RowStructGameStart>(GameStartAssetTableCacheKey);
        // Log.Log(LogType.UI, `RowList count: ${RowList.length}`);
        for (let Row of RowList) {
            this.ImageList.push(Row.ImageSoftRef);
            this.TextList.push(Row.Desc);
        }

        this.PlayImageList();
        
    }
    
    PlayImageList(): void {
        this.StartGame();
    }
    //#endregion 方法
}

export default GameStartManager;