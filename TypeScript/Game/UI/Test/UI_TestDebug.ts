// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';
import { TestLayer } from './TestPaths';

const BP_UI_TestDebug = UE.Class.Load('/Game/UI/TestSuite/UI_TestDebug.UI_TestDebug_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestDebug);

interface UI_TestDebug extends UE.UserWidget {
    Btn_Clear: UE.Button;
    Btn_Hide: UE.Button;
    Txt_Fps: UE.TextBlock;
    Txt_Count_BackGround: UE.TextBlock;
    Txt_Count_Normal: UE.TextBlock;
    Txt_Count_Top: UE.TextBlock;
    Txt_Count_Message: UE.TextBlock;
    Txt_Count_Debug: UE.TextBlock;
    Txt_Log_0: UE.TextBlock;
    Txt_Log_1: UE.TextBlock;
    Txt_Log_2: UE.TextBlock;
    Txt_Log_3: UE.TextBlock;
    Txt_Log_4: UE.TextBlock;
    Txt_Log_5: UE.TextBlock;
    Txt_Log_6: UE.TextBlock;
    Txt_Log_7: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestDebug {
    private TimerId: number = 0;
    private LastTime: number = 0;
    private Fps: number = 0;

    Construct() {
        TestUIManager.Instance.TrackConstruct(this);
        // Puerts mixin 不会执行 constructor，类字段初始值不会自动生效
        this.TimerId = 0;
        this.LastTime = Date.now();
        this.Fps = 0;
    }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); if (this.TimerId) { clearInterval(this.TimerId); this.TimerId = 0; } }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.Fps = 0;
        this.Btn_Clear.OnClicked.Clear();
        this.Btn_Hide.OnClicked.Clear();
        this.Btn_Clear.OnClicked.Add(() => {
            TestUIManager.Instance.ClearEvents();
            this.Refresh();
        });
        this.Btn_Hide.OnClicked.Add(() => this.CloseUI(true));
        this.LastTime = Date.now();
        this.TimerId = setInterval(() => this.Refresh(), 250) as unknown as number;
        this.Refresh();
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        this.Btn_Clear.OnClicked.Clear();
        this.Btn_Hide.OnClicked.Clear();
        if (this.TimerId) {
            clearInterval(this.TimerId);
            this.TimerId = 0;
        }
    }

    OnScriptInit(Single: UISingle) {
    }

    private Refresh() {
        const Now = Date.now();
        const RefreshDelta = Now - this.LastTime;
        this.LastTime = Now;

        // 读取真实帧时间：之前用 1000/刷新间隔 得到的其实是定时器频率（约 4），与渲染帧率无关
        let FrameDelta = 0;
        try {
            FrameDelta = UE.GameplayStatics.GetWorldDeltaSeconds(this);
        } catch {
            FrameDelta = 0;
        }

        if (FrameDelta > 0) {
            const Instant = 1 / FrameDelta;
            this.Fps = this.Fps === 0 ? Instant : this.Fps * 0.8 + Instant * 0.2;
        }

        const FrameMs = FrameDelta > 0 ? FrameDelta * 1000 : 0;
        this.Txt_Fps.SetText(`FPS ${this.Fps.toFixed(0)} · 帧 ${FrameMs.toFixed(1)} ms · 刷新 ${RefreshDelta} ms`);

        const M = TestUIManager.Instance;
        this.Txt_Count_BackGround.SetText(`背景    ${M.GetLayerCount(TestLayer.BackGround)}`);
        this.Txt_Count_Normal.SetText(`常规    ${M.GetLayerCount(TestLayer.Normal)}`);
        this.Txt_Count_Top.SetText(`顶层    ${M.GetLayerCount(TestLayer.Top)}`);
        this.Txt_Count_Message.SetText(`消息    ${M.GetLayerCount(TestLayer.Message)}`);
        this.Txt_Count_Debug.SetText(`调试    ${M.GetLayerCount(TestLayer.Debug)}`);

        const Events = M.GetEvents(8);
        const Lines = [this.Txt_Log_0, this.Txt_Log_1, this.Txt_Log_2, this.Txt_Log_3,
            this.Txt_Log_4, this.Txt_Log_5, this.Txt_Log_6, this.Txt_Log_7];
        Lines.forEach((Line, I) => {
            const E = Events[I];
            Line.SetText(E ? `${E.time}  ${E.window}  ${E.action}` : '—');
        });
    }
}

blueprint.mixin(BPToJs, UI_TestDebug);
export default UI_TestDebug;
