import * as UE from 'ue';
import GameManagerBase from "./GameManagerBase";
import {PureData} from "../Utils/TypeUitls";
import Log from '../Utils/Log/Log';
import LogType from '../Utils/Log/LogType';

class DataManager extends GameManagerBase{

    private static _Instance: DataManager = null;
    public static get Instance(): DataManager {
        if (this._Instance == null)
        {
            this._Instance = new DataManager();
        }
        return this._Instance;
    }
    
    /** 缓存数据 */
    private Cache: Map<string, Record<string, any>> = new Map<string, Record<string, any>>();
    
    /** 加载并缓存整张表 */
    public LoadAndCacheTable(DataPath: string, CacheKey: string = null): void {
        if (!CacheKey) {
            // 读取DataPath的最后一段作为CacheKey
            let PathArray = DataPath.split('/');
            CacheKey = PathArray[PathArray.length - 1];
        }
        
        //TODO 加载表
        let DataTable = null;
        const JsonStr = UE.CORDataLibrary.GetEntireTableAsJson(DataTable);
        
        // 解析Json
        let JsonRecord = JSON.parse(JsonStr);
        this.Cache.set(CacheKey, JsonRecord);
    }

    /** 将已经加载的表缓存到内存中 */
    public CacheTable(CacheKey: string, Table: UE.DataTable): void {
        const JsonStr = UE.CORDataLibrary.GetEntireTableAsJson(Table);
        // Log.Log(LogType.UI, `CacheTable: ${CacheKey}, JsonStr:${JsonStr}`);
        let JsonRecord = JSON.parse(JsonStr);
        // Log.Log(LogType.UI, `CacheTable: ${CacheKey}, JsonRecord length:${JsonRecord.length}`);
        let RowList: any[] = [];
        for (let Key in JsonRecord) {
            RowList.push(JsonRecord[Key]);
            // Log.Log(LogType.UI, `CacheTable: ${Key}, ${JsonRecord[Key]}`);
        }
        this.Cache.set(CacheKey, RowList);
    }
    
    /** 从缓存中获取数据 */
    public GetRow<T>(CacheKey: string, RowName: string): PureData<T> | undefined {
        const TableRecord = this.Cache.get(CacheKey);
        if (TableRecord && TableRecord[RowName]) {
            return TableRecord[RowName] as PureData<T>;
        }
        return undefined;
    }

    /** 从缓存中获取所有数据 */
    public GetAllRows<T>(CacheKey: string): PureData<T>[] {

        const TableRecord = this.Cache.get(CacheKey);
        if (TableRecord) {
            return TableRecord as PureData<T>[];
        }
        return [];

    }
    
}

export default DataManager;