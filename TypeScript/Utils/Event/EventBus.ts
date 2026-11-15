import Log from "../Log/Log";
import LogType from "../Log/LogType";

/** 事件处理器 */
export type EventHandler<T> = (Payload: T) => void;

/**
 * 泛型事件总线。
 * - 事件名与负载类型由 TEvents 约束，Emit 传错负载会编译报错；
 * - 每个事件维护一组处理器，可注册多个；
 * - Emit 时遍历处理器快照，回调内增删订阅不会打乱本轮迭代。
 */
export class EventBus<TEvents extends object> {

    /** 事件名 -> 处理器集合 */
    private Handlers: Map<keyof TEvents, Set<EventHandler<TEvents[keyof TEvents]>>> =
        new Map<keyof TEvents, Set<EventHandler<TEvents[keyof TEvents]>>>();

    /**
     * 订阅事件。
     * @returns 取消订阅的函数
     */
    public On<K extends keyof TEvents>(Event: K, Handler: EventHandler<TEvents[K]>): () => void {
        let HandlerSet = this.Handlers.get(Event);
        if (!HandlerSet) {
            HandlerSet = new Set<EventHandler<TEvents[keyof TEvents]>>();
            this.Handlers.set(Event, HandlerSet);
        }
        HandlerSet.add(Handler as EventHandler<TEvents[keyof TEvents]>);
        return () => this.Off(Event, Handler);
    }

    /**
     * 只订阅一次，触发后自动取消订阅。
     * @returns 取消订阅的函数
     */
    public Once<K extends keyof TEvents>(Event: K, Handler: EventHandler<TEvents[K]>): () => void {
        // 先注销再回调，避免处理器内部抛错导致重复触发
        const Wrapped: EventHandler<TEvents[K]> = (Payload: TEvents[K]) => {
            this.Off(Event, Wrapped);
            Handler(Payload);
        };
        return this.On(Event, Wrapped);
    }

    /** 取消订阅指定处理器 */
    public Off<K extends keyof TEvents>(Event: K, Handler: EventHandler<TEvents[K]>): void {
        const HandlerSet = this.Handlers.get(Event);
        if (!HandlerSet) {
            return;
        }

        HandlerSet.delete(Handler as EventHandler<TEvents[keyof TEvents]>);
        if (HandlerSet.size === 0) {
            this.Handlers.delete(Event);
        }
    }

    /**
     * 派发事件。
     * 遍历的是处理器快照，回调内订阅/取消订阅不影响本轮迭代；
     * 单个处理器抛错只记录日志，不打断其余处理器。
     */
    public Emit<K extends keyof TEvents>(Event: K, Payload: TEvents[K]): void {
        const HandlerSet = this.Handlers.get(Event);
        if (!HandlerSet || HandlerSet.size === 0) {
            return;
        }

        const Snapshot = Array.from(HandlerSet);
        for (const Handler of Snapshot) {
            try {
                Handler(Payload);
            } catch (Err) {
                Log.Error(LogType.Game, `[EventBus][Emit] handler failed on '${String(Event)}': ${Err}`);
            }
        }
    }

    /** 清空指定事件的处理器；不传则清空全部 */
    public Clear(Event?: keyof TEvents): void {
        if (Event === undefined) {
            this.Handlers.clear();
            return;
        }
        this.Handlers.delete(Event);
    }

    /** 指定事件当前的处理器数量 */
    public Count(Event: keyof TEvents): number {
        const HandlerSet = this.Handlers.get(Event);
        return HandlerSet ? HandlerSet.size : 0;
    }
}
