import { EventBus } from '../../../Utils/Event/EventBus';

/** 状态变化负载：新状态 + 本次增量 */
export type ViewModelChange<TState> = { state: Readonly<TState>; patch: Partial<TState> };

/**
 * ViewModel 基类。
 * - 内部用 EventBus 承载 changed 事件，视图通过 Subscribe 被动刷新；
 * - SetState 做浅比较：无实际变化不派发，避免无意义重绘；
 * - 状态在内部合并为新对象，外部拿到的 GetState 是只读视图。
 */
export abstract class ViewModelBase<TState extends object> {

    /** 当前状态（每次变化都换新对象） */
    protected State: TState;

    /** 状态变化事件总线 */
    private Bus: EventBus<{ changed: ViewModelChange<TState> }> = new EventBus<{ changed: ViewModelChange<TState> }>();

    constructor(Initial: TState) {
        // 浅拷贝一份，防止调用方持有引用后误改内部状态
        this.State = { ...Initial };
    }

    /** 读取只读状态 */
    public GetState(): Readonly<TState> {
        return this.State;
    }

    /**
     * 合并增量状态；有实际变化时通知订阅者并返回 true，否则返回 false。
     * 显式 `undefined` 的键会被跳过：本项目未开 strictNullChecks，避免用一个空值清掉必需字段。
     */
    public SetState(Patch: Partial<TState>): boolean {
        if (!Patch) {
            return false;
        }

        const Keys = (Object.keys(Patch) as Array<keyof TState>).filter(
            (Key) => Patch[Key] !== undefined);

        const Merged = { ...this.State } as TState;
        let Changed = false;
        for (const Key of Keys) {
            if (Patch[Key] !== this.State[Key]) {
                (Merged as any)[Key] = Patch[Key];
                Changed = true;
            }
        }
        if (!Changed) {
            return false;
        }

        this.State = Merged;
        this.Bus.Emit('changed', { state: this.State, patch: Patch });
        return true;
    }

    /**
     * 订阅状态变化；返回取消订阅函数
     */
    public Subscribe(Handler: (State: Readonly<TState>) => void): () => void {
        return this.Bus.On('changed', (Change: ViewModelChange<TState>) => Handler(Change.state));
    }

    /** 释放全部订阅 */
    public Dispose(): void {
        this.Bus.Clear();
    }
}
