import TestUIManager, { TestEvent } from '../../../Manager/TestUIManager';
import { ViewModelBase } from '../ViewModel/ViewModelBase';

/** 观澜台·数据 页的视图状态 */
export interface TestDataState {
    Construct: number;
    OnOpen: number;
    OnClose: number;
    Destruct: number;
    ReuseText: string;
    Events: TestEvent[];
}

/**
 * 观澜台·数据 页的 ViewModel。
 * - 从 TestUIManager 拉取生命周期计数与最近事件；
 * - 事件数组按元素比较，内容相同则保留旧引用，避免浅比较误判为变化。
 */
export class TestDataViewModel extends ViewModelBase<TestDataState> {

    constructor() {
        super({ Construct: 0, OnOpen: 0, OnClose: 0, Destruct: 0, ReuseText: '复用 —', Events: [] });
    }

    /** 从管理器同步一次快照；返回状态是否发生变化 */
    public SyncFromManager(): boolean {
        const Manager = TestUIManager.Instance;
        const Lifecycle = Manager.GetLifecycle();
        const Events = Manager.GetEvents(2);
        const Current = this.GetState();

        return this.SetState({
            Construct: Lifecycle.Construct,
            OnOpen: Lifecycle.OnOpen,
            OnClose: Lifecycle.OnClose,
            Destruct: Lifecycle.Destruct,
            // 仅内容变化时才换引用，其余情况沿用旧数组
            Events: this.SameEvents(Events, Current.Events) ? Current.Events : Events,
        });
    }

    /** 更新复用判定文案 */
    public SetReuseText(Text: string): void {
        this.SetState({ ReuseText: Text });
    }

    /** 逐元素比较两个事件数组内容是否一致 */
    private SameEvents(A: TestEvent[], B: TestEvent[]): boolean {
        if (A === B) {
            return true;
        }
        if (!A || !B || A.length !== B.length) {
            return false;
        }
        for (let I = 0; I < A.length; I++) {
            if (A[I].time !== B[I].time || A[I].window !== B[I].window || A[I].action !== B[I].action) {
                return false;
            }
        }
        return true;
    }
}
