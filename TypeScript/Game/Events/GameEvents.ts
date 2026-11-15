import { EventBus } from "../../Utils/Event/EventBus";

/** 游戏事件表：事件名 -> 负载类型 */
export interface GameEventMap {
    /** UI 会话就绪 */
    'ui:ready': { session: number };
    /** UI 会话停止 */
    'ui:stopped': { session: number };
    /** 数据表已写入缓存 */
    'data:table-cached': { key: string; rows: number };
    /** Toast 已显示 */
    'toast:shown': { name: string; slot: number };
}

/** 全局游戏事件总线 */
export const GameEvents = new EventBus<GameEventMap>();
