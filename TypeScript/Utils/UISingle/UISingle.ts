import UISingleType from "./UISingleType";

/**
 * UI通信数据结构 
 */
class UISingle {
    private Single: Map<UISingleType, any> = new Map();
    
    /**
     * 设置数据
     * @param Type 数据类型
     * @param Value 数据值
     * @constructor
     */
    public SetSingle(Type: UISingleType, Value: any) {
        this.Single.set(Type, Value);
    }

    /**
     * 获取数据
     * @param Type 数据类型
     * @constructor
     */
    public GetSingle<T>(Type: UISingleType): T {
        return this.Single.get(Type) as T;
    }
}

export default UISingle;