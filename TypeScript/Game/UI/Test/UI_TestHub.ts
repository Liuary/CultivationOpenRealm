// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import Log from '../../../Utils/Log/Log';
import LogType from '../../../Utils/Log/LogType';
import TestUIManager from '../../../Manager/TestUIManager';
import { TestLayer } from './TestPaths';

const BP_UI_TestHub = UE.Class.Load('/Game/UI/TestSuite/UI_TestHubMain.UI_TestHubMain_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestHub);

interface UI_TestHub extends UE.UserWidget {
    Btn_Nav_Layer: UE.Button;
    Btn_Nav_Stack: UE.Button;
    Btn_Nav_Data: UE.Button;
    Btn_Nav_Widgets: UE.Button;
    Btn_Nav_Debug: UE.Button;
    Btn_SelfCheck: UE.Button;
    Btn_CloseAll: UE.Button;
    Btn_Close: UE.Button;
    Txt_Layer_BackGround_Count: UE.TextBlock;
    Txt_Layer_Normal_Count: UE.TextBlock;
    Txt_Layer_Top_Count: UE.TextBlock;
    Txt_Layer_Message_Count: UE.TextBlock;
    Txt_Layer_Debug_Count: UE.TextBlock;
    Txt_Event_0: UE.TextBlock;
    Txt_Event_1: UE.TextBlock;
    Txt_Event_2: UE.TextBlock;
    Txt_Hint: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestHub {
    private TimerId: number = 0;
    private DemoTimers: number[] = [];
    /** AutoTravel 的延时句柄：关闭/析构必须取消，避免对已销毁/池复用的窗口触发换图 */
    private TravelTimer: number = 0;

    Construct() {
        TestUIManager.Instance.TrackConstruct(this);
        this.TimerId = 0;
        this.TravelTimer = 0;
        this.DemoTimers = [];
    }

    Destruct() {
        TestUIManager.Instance.TrackDestruct(this);
        if (this.TimerId) { clearInterval(this.TimerId); this.TimerId = 0; }
        this.ClearTravelTimer();
        this.DemoTimers.forEach((T) => clearTimeout(T));
        this.DemoTimers = [];
    }

    /** 取消 AutoTravel 延时 */
    private ClearTravelTimer(): void {
        if (this.TravelTimer) {
            clearTimeout(this.TravelTimer);
            this.TravelTimer = 0;
        }
    }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.DemoTimers = [];
        this.BindButtons();
        this.TimerId = setInterval(() => this.Refresh(), 300) as unknown as number;
        this.Refresh();
        Log.Log(LogType.UI, `[UI_TestHub][Opened] normal=${TestUIManager.Instance.GetLayerCount(TestLayer.Normal)}`);
        try {
            const Scale = UE.WidgetLayoutLibrary.GetViewportScale(this);
            const Size = UE.WidgetLayoutLibrary.GetViewportSize(this);
            Log.Log(LogType.UI, `[UI_TestHub] viewport=${Size.X}x${Size.Y} dpi=${Scale}`);
        } catch (e) {
            Log.Error(LogType.UI, `[UI_TestHub] viewport query failed: ${e}`);
        }
        if (TestUIManager.AutoDemo) {
            this.StartDemo();
        }

        // 开发期调试：验证同 GameInstance 换图（只触发一次，换图后不再重复）
        if (TestUIManager.AutoTravelMap) {
            const TravelMap = TestUIManager.AutoTravelMap;
            TestUIManager.AutoTravelMap = "";
            this.ClearTravelTimer();
            this.TravelTimer = setTimeout(() => {
                this.TravelTimer = 0;
                Log.Log(LogType.UI, `[UI_TestHub][AutoTravel] travelling to ${TravelMap}`);
                // 用窗口自身作为 WorldContext，换图后仍能解析到当前世界
                UE.KismetSystemLibrary.ExecuteConsoleCommand(this, `CORUI.Travel ${TravelMap}`);
            }, 2500) as unknown as number;
        }
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        this.UnbindButtons();
        if (this.TimerId) {
            clearInterval(this.TimerId);
            this.TimerId = 0;
        }
        this.ClearTravelTimer();
        this.DemoTimers.forEach((T) => clearTimeout(T));
        this.DemoTimers = [];
    }

    OnShow() {
        TestUIManager.Instance.TrackShow(this);
    }

    OnHide() {
        TestUIManager.Instance.TrackHide(this);
    }

    OnCover() {
        TestUIManager.Instance.TrackCover(this);
    }

    OnScriptInit(Single: UISingle) {
        Log.Log(LogType.UI, `[UI_TestHub][OnScriptInit]`);
    }

    private BindButtons() {
        this.Btn_Nav_Layer.OnClicked.Clear();
        this.Btn_Nav_Stack.OnClicked.Clear();
        this.Btn_Nav_Data.OnClicked.Clear();
        this.Btn_Nav_Widgets.OnClicked.Clear();
        this.Btn_Nav_Debug.OnClicked.Clear();
        this.Btn_SelfCheck.OnClicked.Clear();
        this.Btn_CloseAll.OnClicked.Clear();
        this.Btn_Close.OnClicked.Clear();

        this.Btn_Nav_Layer.OnClicked.Add(() => TestUIManager.Instance.OpenLayer());
        this.Btn_Nav_Stack.OnClicked.Add(() => TestUIManager.Instance.OpenStack());
        this.Btn_Nav_Data.OnClicked.Add(() => TestUIManager.Instance.OpenData());
        this.Btn_Nav_Widgets.OnClicked.Add(() => TestUIManager.Instance.OpenWidgets());
        this.Btn_Nav_Debug.OnClicked.Add(() => TestUIManager.Instance.OpenDebug());
        this.Btn_SelfCheck.OnClicked.Add(() => this.RunSelfCheck());
        this.Btn_CloseAll.OnClicked.Add(() => {
            TestUIManager.Instance.CloseAllExcept([this.GetWindowName()]);
            this.Refresh();
        });
        this.Btn_Close.OnClicked.Add(() => this.CloseUI(true));
    }

    private UnbindButtons() {
        this.Btn_Nav_Layer.OnClicked.Clear();
        this.Btn_Nav_Stack.OnClicked.Clear();
        this.Btn_Nav_Data.OnClicked.Clear();
        this.Btn_Nav_Widgets.OnClicked.Clear();
        this.Btn_Nav_Debug.OnClicked.Clear();
        this.Btn_SelfCheck.OnClicked.Clear();
        this.Btn_CloseAll.OnClicked.Clear();
        this.Btn_Close.OnClicked.Clear();
    }

    private Refresh() {
        const M = TestUIManager.Instance;
        this.Txt_Layer_BackGround_Count.SetText(`${M.GetLayerCount(TestLayer.BackGround)}`);
        this.Txt_Layer_Normal_Count.SetText(`${M.GetLayerCount(TestLayer.Normal)}`);
        this.Txt_Layer_Top_Count.SetText(`${M.GetLayerCount(TestLayer.Top)}`);
        this.Txt_Layer_Message_Count.SetText(`${M.GetLayerCount(TestLayer.Message)}`);
        this.Txt_Layer_Debug_Count.SetText(`${M.GetLayerCount(TestLayer.Debug)}`);

        const Events = M.GetEvents(3);
        const Text = (I: number) => {
            const E = Events[I];
            return E ? `${E.time}  ${E.window}  ${E.action}` : '—';
        };
        this.Txt_Event_0.SetText(Text(0));
        this.Txt_Event_1.SetText(Text(1));
        this.Txt_Event_2.SetText(Text(2));
    }

    private StartDemo() {
        const M = TestUIManager.Instance;
        const Hub = this.GetWindowName();
        const Step = (Ms: number, Fn: () => void) => {
            this.DemoTimers.push(setTimeout(Fn, Ms) as unknown as number);
        };
        const OnlyHub = () => M.CloseAllExcept([Hub]);

        Step(2000, () => { OnlyHub(); M.OpenLayer(); });
        Step(5000, () => { OnlyHub(); M.OpenStack(); });
        Step(8000, () => { OnlyHub(); M.OpenData(); });
        Step(11000, () => { OnlyHub(); M.OpenWidgets(); });
        Step(14000, () => { OnlyHub(); M.OpenWidgets(); M.OpenDebug(); });
        Step(17000, () => {
            OnlyHub();
            // 队列验证：3 条可见（槽位 0/1/2），第 4 条排队，等首条 3.2s 收起后补位
            M.PushToast('通知 1', '第一条 · 槽位 0。');
            M.PushToast('通知 2', '第二条 · 槽位 1。');
            M.PushToast('通知 3', '第三条 · 槽位 2。');
            M.PushToast('通知 4', '第四条 · 排队补位。');
            M.PushConfirm('是否清空本次会话的全部窗口？', TestLayer.Top);
        });
        Step(20000, () => { OnlyHub(); M.OpenBackdrop(); });
        Step(23000, () => { OnlyHub(); });
        Step(24000, () => this.RunSelfCheck());
    }

    private RunSelfCheck() {
        const M = TestUIManager.Instance;
        if (M.IsSelfCheckRunning()) {
            return;
        }
        this.Txt_Hint.SetText('自检中…');
        M.RunSelfCheck((Summary, Results) => {
            const Failed = Results.filter((R) => !R.pass).map((R) => R.name);
            this.Txt_Hint.SetText(Failed.length === 0 ? `自检通过 ${Summary}` : `自检 ${Summary} · 失败：${Failed.join('、')}`);
            this.Refresh();
        });
    }
}

blueprint.mixin(BPToJs, UI_TestHub);
export default UI_TestHub;
