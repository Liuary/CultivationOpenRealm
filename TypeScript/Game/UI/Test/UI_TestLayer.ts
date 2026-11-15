// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import Log from '../../../Utils/Log/Log';
import LogType from '../../../Utils/Log/LogType';
import TestUIManager from '../../../Manager/TestUIManager';
import { TestLayer } from './TestPaths';

const BP_UI_TestLayer = UE.Class.Load('/Game/UI/TestSuite/UI_TestLayer2.UI_TestLayer2_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestLayer);

interface UI_TestLayer extends UE.UserWidget {
    Btn_Push_BackGround: UE.Button;
    Btn_Push_Normal: UE.Button;
    Btn_Push_Top: UE.Button;
    Btn_Push_Message: UE.Button;
    Btn_Push_Debug: UE.Button;
    Btn_AllClose: UE.Button;
    Btn_Back: UE.Button;
    Txt_Count_BackGround: UE.TextBlock;
    Txt_Count_Normal: UE.TextBlock;
    Txt_Count_Top: UE.TextBlock;
    Txt_Count_Message: UE.TextBlock;
    Txt_Count_Debug: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestLayer {
    private TimerId: number = 0;

    Construct() { TestUIManager.Instance.TrackConstruct(this); }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.Bind();
        this.TimerId = setInterval(() => this.Refresh(), 300) as unknown as number;
        this.Refresh();
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        this.Unbind();
        if (this.TimerId) {
            clearInterval(this.TimerId);
            this.TimerId = 0;
        }
    }

    OnScriptInit(Single: UISingle) {
    }

    private Bind() {
        [this.Btn_Push_BackGround, this.Btn_Push_Normal, this.Btn_Push_Top,
            this.Btn_Push_Message, this.Btn_Push_Debug, this.Btn_AllClose, this.Btn_Back]
            .forEach((B) => B.OnClicked.Clear());

        this.Btn_Push_BackGround.OnClicked.Add(() => TestUIManager.Instance.OpenBackdrop());
        this.Btn_Push_Normal.OnClicked.Add(() => TestUIManager.Instance.OpenStack());
        this.Btn_Push_Top.OnClicked.Add(() => {
            TestUIManager.Instance.PushConfirm('顶层示例：确认后关闭。', TestLayer.Top);
        });
        this.Btn_Push_Message.OnClicked.Add(() => {
            TestUIManager.Instance.PushToast('通知', '消息层示例 · 3.2 秒后自动收起。');
        });
        this.Btn_Push_Debug.OnClicked.Add(() => TestUIManager.Instance.OpenDebug());
        this.Btn_AllClose.OnClicked.Add(() => {
            TestUIManager.Instance.CloseAllExcept([this.GetWindowName(), 'UI_TestHub']);
            this.Refresh();
        });
        this.Btn_Back.OnClicked.Add(() => this.CloseUI(true));
    }

    private Unbind() {
        [this.Btn_Push_BackGround, this.Btn_Push_Normal, this.Btn_Push_Top,
            this.Btn_Push_Message, this.Btn_Push_Debug, this.Btn_AllClose, this.Btn_Back]
            .forEach((B) => B.OnClicked.Clear());
    }

    private Refresh() {
        const M = TestUIManager.Instance;
        this.Txt_Count_BackGround.SetText(`${M.GetLayerCount(TestLayer.BackGround)} 个窗口`);
        this.Txt_Count_Normal.SetText(`${M.GetLayerCount(TestLayer.Normal)} 个窗口`);
        this.Txt_Count_Top.SetText(`${M.GetLayerCount(TestLayer.Top)} 个窗口`);
        this.Txt_Count_Message.SetText(`${M.GetLayerCount(TestLayer.Message)} 个窗口`);
        this.Txt_Count_Debug.SetText(`${M.GetLayerCount(TestLayer.Debug)} 个窗口`);
    }
}

blueprint.mixin(BPToJs, UI_TestLayer);
export default UI_TestLayer;
