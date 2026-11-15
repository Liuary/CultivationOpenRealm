// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';
import Log from '../../../Utils/Log/Log';
import LogType from '../../../Utils/Log/LogType';
import UI_TestSingleType from './UI_TestSingleType';
import { TestDataViewModel } from './TestDataViewModel';
import { TestClassPaths, TestLayer, TestWindowNames } from './TestPaths';

const BP_UI_TestData = UE.Class.Load('/Game/UI/TestSuite/UI_TestData.UI_TestData_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestData);

interface UI_TestData extends UE.UserWidget {
    Btn_Sync: UE.Button;
    Btn_Async: UE.Button;
    Btn_Back: UE.Button;
    Txt_Data_Name: UE.TextBlock;
    Txt_Data_Serial: UE.TextBlock;
    Txt_Data_Tone: UE.TextBlock;
    Txt_Data_Mode: UE.TextBlock;
    Txt_Count_Construct: UE.TextBlock;
    Txt_Count_Open: UE.TextBlock;
    Txt_Count_Close: UE.TextBlock;
    Txt_Count_Destruct: UE.TextBlock;
    Txt_Reuse: UE.TextBlock;
    Txt_Instance: UE.TextBlock;
    Txt_Payload: UE.TextBlock;
    Txt_Log_0: UE.TextBlock;
    Txt_Log_1: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestData {
    private TimerId: number = 0;
    /** 复用判定延时（激活私有，关闭/析构必须取消） */
    private ReuseTimerId: number = 0;
    /** 激活代际：池复用后旧回调不得写新激活 */
    private Activation: number = 0;
    private PushSeq: number = 0;
    /** 视图模型：状态与渲染的唯一数据来源 */
    private ViewModel: TestDataViewModel = null;
    /** 状态变化订阅的取消函数 */
    private Unsubscribe: (() => void) = null;
    /** 渲染次数，供运行时验证观察通知是否到达 */
    private ChangeSeq: number = 0;

    Construct() {
        TestUIManager.Instance.TrackConstruct(this);
        // Puerts mixin 不会执行 constructor，类字段初始值不会自动生效
        this.TimerId = 0;
        this.ReuseTimerId = 0;
        this.Activation = 0;
        this.PushSeq = 0;
        this.ChangeSeq = 0;
        this.Unsubscribe = null;
        // 视图模型只创建一次，跨激活复用
        this.ViewModel = new TestDataViewModel();
    }
    Destruct() {
        TestUIManager.Instance.TrackDestruct(this);
        this.ClearTimers();
        if (this.Unsubscribe) {
            this.Unsubscribe();
            this.Unsubscribe = null;
        }
        if (this.ViewModel) {
            this.ViewModel.Dispose();
            this.ViewModel = null;
        }
    }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.ClearTimers();
        this.Activation++;
        this.PushSeq = 0;
        // 防御：池复用的边界情况下 Construct 可能未走到，补建一次视图模型
        if (!this.ViewModel) {
            this.ViewModel = new TestDataViewModel();
        }
        this.ViewModel.SetReuseText('复用 —');
        // 每次打开重新订阅，避免重复订阅导致一次变化渲染多次
        if (this.Unsubscribe) {
            this.Unsubscribe();
            this.Unsubscribe = null;
        }
        this.Unsubscribe = this.ViewModel.Subscribe(() => this.Render());
        this.Btn_Sync.OnClicked.Clear();
        this.Btn_Async.OnClicked.Clear();
        this.Btn_Back.OnClicked.Clear();
        this.Btn_Sync.OnClicked.Add(() => this.PushCard(false));
        this.Btn_Async.OnClicked.Add(() => this.PushCard(true));
        this.Btn_Back.OnClicked.Add(() => this.CloseUI(true));

        // 池复用时清掉上一次激活的临时载荷文案，计数/复用/事件由 ViewModel 渲染
        this.Txt_Data_Name.SetText('—');
        this.Txt_Data_Serial.SetText('—');
        this.Txt_Data_Tone.SetText('—');
        this.Txt_Data_Mode.SetText('—');
        this.Txt_Payload.SetText('—');
        this.Txt_Instance.SetText('—');
        this.TimerId = setInterval(() => this.Refresh(), 300) as unknown as number;
        // 首帧只渲染一次：Sync 有变化会经订阅触发 Render，否则显式补一次
        if (!this.ViewModel.SyncFromManager()) {
            this.Render();
        }
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        if (this.Unsubscribe) {
            this.Unsubscribe();
            this.Unsubscribe = null;
        }
        this.Btn_Sync.OnClicked.Clear();
        this.Btn_Async.OnClicked.Clear();
        this.Btn_Back.OnClicked.Clear();
        this.ClearTimers();
    }

    private ClearTimers(): void {
        if (this.TimerId) {
            clearInterval(this.TimerId);
            this.TimerId = 0;
        }
        if (this.ReuseTimerId) {
            clearTimeout(this.ReuseTimerId);
            this.ReuseTimerId = 0;
        }
    }

    OnScriptInit(Single: UISingle) {
    }

    private PushCard(Async: boolean) {
        this.PushSeq++;
        const Name = `${TestWindowNames.CardPrefix}_Data_${this.PushSeq}`;
        const Single = new UISingle();
        Single.SetSingle(Number(UI_TestSingleType.Title), '数据 · 示例');
        Single.SetSingle(Number(UI_TestSingleType.Serial), `${this.PushSeq}`.padStart(2, '0'));
        Single.SetSingle(Number(UI_TestSingleType.Body), Async ? '异步推入的载荷。' : '同步推入的载荷。');
        Single.SetSingle(Number(UI_TestSingleType.Tone), '平');
        Single.SetSingle(Number(UI_TestSingleType.Mode), Async ? '异步' : '同步');

        this.Txt_Data_Name.SetText('示例');
        this.Txt_Data_Serial.SetText(`${this.PushSeq}`.padStart(2, '0'));
        this.Txt_Data_Tone.SetText('普通');
        this.Txt_Data_Mode.SetText(Async ? '异步' : '同步');
        this.Txt_Payload.SetText(`名称 示例 · 序号 ${`${this.PushSeq}`.padStart(2, '0')} · 备注 普通 · ${Async ? '异步' : '同步'}`);
        this.Txt_Instance.SetText(`入栈 ${Name}`);

        const Before = TestUIManager.Instance.GetLifecycle().Construct;
        TestUIManager.Instance.Push(TestClassPaths.Card, Name, TestLayer.Normal, Single, Async);

        // 记录本次激活代际：窗口关闭/池复用后旧回调必须放弃
        const Activation = this.Activation;
        if (this.ReuseTimerId) {
            clearTimeout(this.ReuseTimerId);
            this.ReuseTimerId = 0;
        }
        this.ReuseTimerId = setTimeout(() => {
            this.ReuseTimerId = 0;
            if (Activation !== this.Activation) {
                Log.Log(LogType.UI, `[UI_TestData] stale reuse callback ignored.`);
                return;
            }
            const After = TestUIManager.Instance.GetLifecycle().Construct;
            this.ViewModel.SetReuseText(After > Before ? '复用 否（新建实例）' : '复用 是（池内实例）');
        }, 500) as unknown as number;
    }

    private Refresh() {
        // 只同步状态，渲染由订阅回调驱动
        this.ViewModel.SyncFromManager();
    }

    /** 按当前状态刷新文本，并记录一次变化序号 */
    private Render(): void {
        const S = this.ViewModel.GetState();
        this.Txt_Count_Construct.SetText(`${S.Construct}`);
        this.Txt_Count_Open.SetText(`${S.OnOpen}`);
        this.Txt_Count_Close.SetText(`${S.OnClose}`);
        this.Txt_Count_Destruct.SetText(`${S.Destruct}`);
        this.Txt_Reuse.SetText(S.ReuseText);

        const Lines = [this.Txt_Log_0, this.Txt_Log_1];
        Lines.forEach((Line, I) => {
            const E = S.Events[I];
            Line.SetText(E ? `${E.time}  ${E.window}  ${E.action}` : '—');
        });

        this.ChangeSeq++;
        Log.Log(LogType.UI, `[TestDataVM] change #${this.ChangeSeq} c=${S.Construct} o=${S.OnOpen} cl=${S.OnClose} d=${S.Destruct}`);
    }
}

blueprint.mixin(BPToJs, UI_TestData);
export default UI_TestData;
