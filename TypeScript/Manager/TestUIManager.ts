import * as UE from 'ue';
import UIManager from './UIManager';
import UISingle from '../Utils/UISingle/UISingle';
import Log from '../Utils/Log/Log';
import LogType from '../Utils/Log/LogType';
import UI_TestSingleType from '../Game/UI/Test/UI_TestSingleType';
import { GameEvents } from '../Game/Events/GameEvents';
import {
    TestClassPaths, TestWindowNames, PushPath, TestLayer, LayerKey,
} from '../Game/UI/Test/TestPaths';

export interface TestEvent {
    time: string;
    window: string;
    action: string;
}

export interface TestSelfCheckResult {
    name: string;
    pass: boolean;
}

/**
 * 《观澜》测试套件管理器。
 * - 统一推入/弹出，并在窗口生命周期回调里登记事件与层级计数；
 * - 只统计本套件推入的窗口，不反射框架内部状态；
 * - 提供一键自检脚本。
 */
class TestUIManager {
    /** 开发期总开关：关闭后加载完成不会自动弹出观澜台。 */
    public static Enabled: boolean = true;
    /** 仅用于自动截图验证：打开观澜台后自动巡游各窗口。 */
    public static AutoDemo: boolean = false;
    /** 开发期调试：观澜台打开后自动切图到该关卡（空 = 关闭），用于验证同 GameInstance 换图。 */
    public static AutoTravelMap: string = "";

    private static _Instance: TestUIManager = null;
    public static get Instance(): TestUIManager {
        if (this._Instance == null) {
            this._Instance = new TestUIManager();
        }
        return this._Instance;
    }

    private Events: TestEvent[] = [];
    private OpenNames: Map<number, Set<string>> = new Map();
    private NameLayers: Map<string, number> = new Map();
    private Lifecycle = { Construct: 0, OnOpen: 0, OnClose: 0, Destruct: 0 };
    private ConstructByClass: Map<string, number> = new Map();
    private CardSeq: number = 0;
    private ToastSeq: number = 0;
    private SelfCheckRunning: boolean = false;

    /** 消息层 Toast 的可见槽位数（超出后排队） */
    public static readonly ToastSlotCount: number = 3;
    /** 每个槽位的纵向偏移（UMG 像素，约等于一张 Toast 卡片高度 + 间距） */
    public static readonly ToastSlotHeight: number = 120;

    /** 槽位占用表 */
    private ToastSlotOccupied: boolean[] = [];
    /** Toast 名 -> 槽位 */
    private ToastSlots: Map<string, number> = new Map();
    /** 满槽时排队的 Toast */
    private ToastPending: Array<{ Title: string; Body: string }> = [];

    //#region 事件与计数

    private Now(): string {
        const d = new Date();
        const p = (n: number) => (n < 10 ? `0${n}` : `${n}`);
        return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`;
    }

    public LogEvent(Window: string, Action: string): void {
        this.Events.push({ time: this.Now(), window: Window, action: Action });
        if (this.Events.length > 50) {
            this.Events.shift();
        }
    }

    public GetEvents(Count: number): TestEvent[] {
        return this.Events.slice(Math.max(0, this.Events.length - Count));
    }

    public ClearEvents(): void {
        this.Events = [];
    }

    public GetLifecycle(): { Construct: number; OnOpen: number; OnClose: number; Destruct: number } {
        return this.Lifecycle;
    }

    private WindowName(Window: any): string {
        try {
            return `${Window.GetWindowName()}`;
        } catch {
            return 'Unknown';
        }
    }

    private WindowLayer(Window: any): number {
        const Name = this.WindowName(Window);
        const Layer = this.NameLayers.get(Name);
        return Layer === undefined ? 0 : Layer;
    }

    public TrackConstruct(Window: any): void {
        this.Lifecycle.Construct++;
        const ClassName = `${Window.GetClass().GetName()}`;
        this.ConstructByClass.set(ClassName, (this.ConstructByClass.get(ClassName) || 0) + 1);
        this.LogEvent(this.WindowName(Window), 'Construct');
    }

    public TrackDestruct(Window: any): void {
        this.Lifecycle.Destruct++;
        this.LogEvent(this.WindowName(Window), 'Destruct');
    }

    public TrackOpen(Window: any): void {
        this.Lifecycle.OnOpen++;
        const Layer = this.WindowLayer(Window);
        const Name = this.WindowName(Window);
        if (!this.OpenNames.has(Layer)) {
            this.OpenNames.set(Layer, new Set<string>());
        }
        this.OpenNames.get(Layer).add(Name);
        Log.Log(LogType.UI, `[TestManager][Open] ${Name} L=${Layer} n=${this.GetLayerCount(Layer)}`);
        this.LogEvent(Name, 'OnOpen');

        // Toast 真正 Open 时才广播“已显示”：Push 只是请求，可能被会话拒绝或脚本回滚
        const ToastSlot = this.ToastSlots.get(Name);
        if (ToastSlot !== undefined) {
            GameEvents.Emit('toast:shown', { name: Name, slot: ToastSlot });
        }
    }

    public TrackClose(Window: any): void {
        this.Lifecycle.OnClose++;
        const Layer = this.WindowLayer(Window);
        const Name = this.WindowName(Window);
        const Set = this.OpenNames.get(Layer);
        if (Set) {
            Set.delete(Name);
        }
        Log.Log(LogType.UI, `[TestManager][Close] ${Name} L=${Layer} n=${this.GetLayerCount(Layer)}`);
        this.LogEvent(Name, 'OnClose');

        // Toast 槽位释放 + 队列补位（延迟到下一 tick，避免在容器关闭回调里重入 Push）
        const Slot = this.ToastSlots.get(Name);
        if (Slot !== undefined) {
            this.ToastSlots.delete(Name);
            this.ToastSlotOccupied[Slot] = false;

            const Next = this.ToastPending.shift();
            if (Next) {
                const PendingSlot = Slot;
                const Title = Next.Title;
                const Body = Next.Body;
                setTimeout(() => {
                    this.SpawnToast(Title, Body, PendingSlot);
                }, 0);
            }
        }
    }

    public TrackShow(Window: any): void {
        this.LogEvent(this.WindowName(Window), 'OnShow');
    }

    public TrackHide(Window: any): void {
        this.LogEvent(this.WindowName(Window), 'OnHide');
    }

    public TrackCover(Window: any): void {
        this.LogEvent(this.WindowName(Window), 'OnCover');
    }

    public GetConstructCount(ClassName: string): number {
        return this.ConstructByClass.get(ClassName) || 0;
    }

    public GetLayerCount(Layer: number): number {
        const Set = this.OpenNames.get(Number(Layer));
        return Set ? Set.size : 0;
    }

    public GetLayerNames(Layer: number): string[] {
        const Set = this.OpenNames.get(Number(Layer));
        return Set ? Array.from(Set) : [];
    }

    //#endregion

    //#region 推入 / 弹出

    public Push(ClassPath: string, Name: string, Layer: UE.EUILayerType, Single: UISingle = null, Async: boolean = true): void {
        this.NameLayers.set(Name, Number(Layer));
        Log.Log(LogType.UI, `[TestManager][Push] ${Name} layer=${Number(Layer)}`);
        UIManager.Instance.PushUI(PushPath(ClassPath), Single, Name, Layer, Async);
    }

    public Pop(Name: string, Layer: UE.EUILayerType, PopSelf: boolean = true): void {
        UIManager.Instance.PopUI(Name, Layer, PopSelf);
    }

    public OpenHub(): void {
        this.Push(TestClassPaths.Hub, TestWindowNames.Hub, TestLayer.Normal);
    }

    public OpenLayer(): void {
        this.Push(TestClassPaths.Layer, TestWindowNames.Layer, TestLayer.Normal);
    }

    public OpenStack(): void {
        this.Push(TestClassPaths.Stack, TestWindowNames.Stack, TestLayer.Normal);
    }

    public OpenData(): void {
        this.Push(TestClassPaths.Data, TestWindowNames.Data, TestLayer.Normal);
    }

    public OpenWidgets(): void {
        this.Push(TestClassPaths.Widgets, TestWindowNames.Widgets, TestLayer.Normal);
    }

    public OpenDebug(): void {
        this.Push(TestClassPaths.Debug, TestWindowNames.Debug, TestLayer.Debug);
    }

    public OpenBackdrop(): void {
        this.Push(TestClassPaths.Backdrop, TestWindowNames.Backdrop, TestLayer.BackGround);
    }

    public PushCard(Title: string, Serial: string, Body: string): string {
        this.CardSeq++;
        const Name = `${TestWindowNames.CardPrefix}_${this.CardSeq}`;
        this.Push(TestClassPaths.Card, Name, TestLayer.Normal, this.MakeCardSingle(Title, Serial, Body));
        return Name;
    }

    public PushCardNamed(Name: string, Title: string, Serial: string, Body: string): void {
        this.Push(TestClassPaths.Card, Name, TestLayer.Normal, this.MakeCardSingle(Title, Serial, Body));
    }

    public PushConfirm(Body: string, Layer: UE.EUILayerType = TestLayer.Top): string {
        const Single = new UISingle();
        Single.SetSingle(Number(UI_TestSingleType.Body), Body);
        this.Push(TestClassPaths.Confirm, TestWindowNames.Confirm, Layer, Single);
        return TestWindowNames.Confirm;
    }

    /**
     * 推入一条消息层 Toast。
     * - 最多同时显示 ToastSlotCount 条，按槽位纵向偏移，避免重叠；
     * - 槽位已满时进入队列，等前面某条自动收起后补位。
     * @returns 实际显示的窗口名；排队时返回空串
     */
    public PushToast(Title: string, Body: string): string {
        const Slot = this.AllocToastSlot();
        if (Slot < 0) {
            this.ToastPending.push({ Title, Body });
            Log.Log(LogType.UI, `[TestManager][Toast] queued (pending=${this.ToastPending.length})`);
            return '';
        }

        return this.SpawnToast(Title, Body, Slot);
    }

    /** 找到一个空闲槽位，没有则返回 -1 */
    private AllocToastSlot(): number {
        for (let i = 0; i < TestUIManager.ToastSlotCount; i++) {
            if (!this.ToastSlotOccupied[i]) {
                return i;
            }
        }
        return -1;
    }

    private SpawnToast(Title: string, Body: string, Slot: number): string {
        this.ToastSeq++;
        const Name = `${TestWindowNames.Toast}_${this.ToastSeq}`;

        this.ToastSlotOccupied[Slot] = true;
        this.ToastSlots.set(Name, Slot);

        const Single = new UISingle();
        Single.SetSingle(Number(UI_TestSingleType.Title), Title);
        Single.SetSingle(Number(UI_TestSingleType.Body), Body);
        Single.SetSingle(Number(UI_TestSingleType.Index), Slot);

        this.Push(TestClassPaths.Toast, Name, TestLayer.Message, Single);
        Log.Log(LogType.UI, `[TestManager][Toast] ${Name} slot=${Slot}`);
        return Name;
    }

    /** 关闭所有测试窗口，exclude 里的窗口名保留。 */
    public CloseAllExcept(Exclude: string[]): void {
        const Snapshot: Array<{ Layer: number; Name: string }> = [];
        this.OpenNames.forEach((Set, Layer) => {
            Set.forEach((Name) => {
                if (Exclude.indexOf(Name) < 0) {
                    Snapshot.push({ Layer, Name });
                }
            });
        });
        Snapshot.forEach((Item) => {
            this.Pop(Item.Name, Item.Layer as UE.EUILayerType, true);
        });
    }

    //#endregion

    //#region 自检

    public IsSelfCheckRunning(): boolean {
        return this.SelfCheckRunning;
    }

    public RunSelfCheck(OnDone: (Summary: string, Results: TestSelfCheckResult[]) => void): void {
        if (this.SelfCheckRunning) {
            OnDone('自检进行中', []);
            return;
        }
        this.SelfCheckRunning = true;
        const Results: TestSelfCheckResult[] = [];
        const Check = (Name: string, Pass: boolean) => {
            Results.push({ name: Name, pass: Pass });
            this.LogEvent('自检', `${Pass ? 'PASS' : 'FAIL'} ${Name}`);
            Log.Log(LogType.UI, `[TestSelfCheck] ${Pass ? 'PASS' : 'FAIL'} ${Name}`);
        };
        const Later = (Ms: number, Fn: () => void) => setTimeout(Fn, Ms);
        const CardA = this.PushCard('自检 · A', '01', '首先推入一张卡片。');

        Later(500, () => {
            Check('推入一叶', this.CardCount() >= 1);
            const OpenBefore = this.Lifecycle.OnOpen;
            this.Push(TestClassPaths.Card, CardA, TestLayer.Normal, this.MakeCardSingle('自检 · A', '01', '同名重推同一窗口。'));

            Later(500, () => {
                Check('同名重推', this.CardCount() === 1 && this.Lifecycle.OnOpen >= OpenBefore + 1);
                this.Pop('', TestLayer.Normal, true);

                Later(400, () => {
                    Check('弹出顶部', this.CardCount() === 0);

                    const CardB = this.PushCard('自检 · B', '02', '第二张卡片。');
                    const CardC = this.PushCard('自检 · C', '03', '第三张卡片。');

                    Later(500, () => {
                        Check('叠两张', this.CardCount() === 2);
                        this.Pop(CardC, TestLayer.Normal, true);

                        Later(400, () => {
                            const Names = this.GetLayerNames(TestLayer.Normal);
                            Check('按名弹出', Names.indexOf(CardB) >= 0 && Names.indexOf(CardC) < 0);

                            this.PushConfirm('覆层测试：下方卡片应进入 HiddenUnder', TestLayer.Normal);
                            Later(500, () => {
                                Check('覆层隐藏', this.Events.some((E) => E.action === 'OnHide'));
                                this.Pop(TestWindowNames.Confirm, TestLayer.Normal, true);

                                Later(500, () => {
                                    Check('覆层恢复', this.Events.some((E) => E.action === 'OnShow'));
                                    const CardD = this.PushCard('自检 · D', '04', '异步推入。');

                                    Later(600, () => {
                                        Check('异步推入', this.GetLayerNames(TestLayer.Normal).indexOf(CardD) >= 0);
                                        this.CloseAllExcept([TestWindowNames.Hub]);

                                        Later(600, () => {
                                            Check('清空', this.CardCount() === 0);
                                            const Pass = Results.filter((R) => R.pass).length;
                                            this.SelfCheckRunning = false;
                                            OnDone(`${Pass}/${Results.length}`, Results);
                                        });
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    }

    private CardCount(): number {
        return this.GetLayerNames(TestLayer.Normal)
            .filter((N) => N.indexOf(TestWindowNames.CardPrefix) === 0).length;
    }

    private MakeCardSingle(Title: string, Serial: string, Body: string): UISingle {
        const Single = new UISingle();
        Single.SetSingle(Number(UI_TestSingleType.Title), Title);
        Single.SetSingle(Number(UI_TestSingleType.Serial), Serial);
        Single.SetSingle(Number(UI_TestSingleType.Body), Body);
        return Single;
    }

    //#endregion
}

export default TestUIManager;
