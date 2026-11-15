import * as UE from 'ue';
import * as cpp from 'cpp';
import GameManagerBase from "./GameManagerBase";
import {PureData} from "../Utils/TypeUitls";
import Log from '../Utils/Log/Log';
import LogType from '../Utils/Log/LogType';
import { GameEvents } from "../Game/Events/GameEvents";

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
    
    /** 通过资源路径加载数据表并缓存（成功返回 true） */
    public LoadTableByPath(DataPath: string, CacheKey: string = null): boolean {
        if (!DataPath) {
            Log.Error(LogType.UI, `LoadTableByPath failed: empty path.`);
            return false;
        }

        if (!CacheKey) {
            CacheKey = this.MakeCacheKey(DataPath);
        }

        // Puerts 的类型断言只是编译期行为：先按类校验，再按 DataTable 使用
        const Loaded = cpp.CORSoftObjectPath.MakeSoftObjectPath(DataPath).Load();
        if (!Loaded || !Loaded.IsA(UE.DataTable.StaticClass())) {
            Log.Error(LogType.UI, `LoadTableByPath failed (not a DataTable): ${DataPath}`);
            return false;
        }
        const Table = Loaded as UE.DataTable;

        // CacheTable 内部发出 data:table-cached 事件
        this.CacheTable(CacheKey, Table);
        return true;
    }

    /** 从路径推导缓存键：取末段并去掉对象后缀/引号（/Game/X/DT_Foo.DT_Foo' -> DT_Foo） */
    private MakeCacheKey(DataPath: string): string {
        const Segments = DataPath.split('/');
        let Last = Segments[Segments.length - 1];
        Last = Last.replace(/'/g, '');
        const DotIndex = Last.indexOf('.');
        if (DotIndex > 0) {
            Last = Last.substring(0, DotIndex);
        }
        return Last;
    }

    /** @deprecated 请使用 LoadTableByPath（保留布尔返回值，失败可感知） */
    public LoadAndCacheTable(DataPath: string, CacheKey: string = null): boolean {
        return this.LoadTableByPath(DataPath, CacheKey);
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

        GameEvents.Emit('data:table-cached', { key: CacheKey, rows: RowList.length });
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