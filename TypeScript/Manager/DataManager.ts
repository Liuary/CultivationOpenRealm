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
    
    /** 缓存数据：CacheKey -> (行名 -> 行数据)，保留行名以便 GetRow 按名取值 */
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
        // 形态：{ "RowName": { "Field": value } }；必须保留行名-行数据的映射，
        // 之前压成数组会丢掉行名，导致 GetRow 按行名永远取不到。
        const JsonRecord: Record<string, any> = JSON.parse(JsonStr);
        this.Cache.set(CacheKey, JsonRecord);

        GameEvents.Emit('data:table-cached', { key: CacheKey, rows: Object.keys(JsonRecord).length });
    }

    /** 从缓存中获取数据（按行名） */
    public GetRow<T>(CacheKey: string, RowName: string): PureData<T> | undefined {
        const TableRecord = this.Cache.get(CacheKey);
        if (TableRecord && TableRecord[RowName]) {
            return TableRecord[RowName] as PureData<T>;
        }
        return undefined;
    }

    /** 从缓存中获取所有数据（行顺序不保证，与 UE DataTable 的遍历顺序一致） */
    public GetAllRows<T>(CacheKey: string): PureData<T>[] {
        const TableRecord = this.Cache.get(CacheKey);
        return TableRecord ? (Object.values(TableRecord) as PureData<T>[]) : [];
    }

    /** 获取某张表的行名列表 */
    public GetRowNames(CacheKey: string): string[] {
        const TableRecord = this.Cache.get(CacheKey);
        return TableRecord ? Object.keys(TableRecord) : [];
    }
    
}

export default DataManager;