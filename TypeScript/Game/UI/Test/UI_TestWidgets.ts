// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';

const BP_UI_TestWidgets = UE.Class.Load('/Game/UI/TestSuite/UI_TestWidgets.UI_TestWidgets_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestWidgets);

interface UI_TestWidgets extends UE.UserWidget {
    Btn_Normal: UE.Button;
    Btn_Normal_Label: UE.TextBlock;
    Btn_Hover: UE.Button;
    Btn_Pressed: UE.Button;
    Btn_Disabled: UE.Button;
    Btn_LoadImage: UE.Button;
    Btn_Replay: UE.Button;
    Btn_Back: UE.Button;
    Img_Async: any;
    Progress_Test: any;
    Txt_Progress: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestWidgets {
    private ClickCount: number = 0;
    private AnimTick: number = 0;
    private Progress: number = 0;

    Construct() { TestUIManager.Instance.TrackConstruct(this); }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); if (this.AnimTick) { clearInterval(this.AnimTick); this.AnimTick = 0; } }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.ClickCount = 0;
        this.AnimTick = 0;
        this.Progress = 0;
        const Buttons = [this.Btn_Normal, this.Btn_Hover, this.Btn_Pressed,
            this.Btn_LoadImage, this.Btn_Replay, this.Btn_Back];
        Buttons.forEach((B) => {
            B.OnClicked.Clear();
            B.OnHovered.Clear();
            B.OnPressed.Clear();
        });

        this.Btn_Normal.OnClicked.Add(() => {
            this.ClickCount++;
            this.Btn_Normal_Label.SetText(`常态 ×${this.ClickCount}`);
        });
        this.Btn_Hover.OnHovered.Add(() => {
            TestUIManager.Instance.LogEvent('控件', '悬停：按钮描边转为锈红');
        });
        this.Btn_Pressed.OnPressed.Add(() => {
            TestUIManager.Instance.LogEvent('控件', '按下：填充 12% 纸白');
        });
        this.Btn_LoadImage.OnClicked.Add(() => {
            this.Img_Async.SetImagePath('/Game/Asset/UI/GameStart/T_AU_GameStart_01_World_cn.T_AU_GameStart_01_World_cn', true, true);
            TestUIManager.Instance.LogEvent('控件', 'CORImage 异步加载示例纹理');
        });
        this.Btn_Replay.OnClicked.Add(() => this.StartProgress());
        this.Btn_Back.OnClicked.Add(() => this.CloseUI(true));

        this.StartProgress();
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        [this.Btn_Normal, this.Btn_Hover, this.Btn_Pressed, this.Btn_LoadImage,
            this.Btn_Replay, this.Btn_Back].forEach((B) => {
            B.OnClicked.Clear();
            B.OnHovered.Clear();
            B.OnPressed.Clear();
        });
        if (this.AnimTick) {
            clearInterval(this.AnimTick);
            this.AnimTick = 0;
        }
    }

    OnScriptInit(Single: UISingle) {
    }

    private StartProgress() {
        if (this.AnimTick) {
            clearInterval(this.AnimTick);
        }
        this.Progress = 0;
        this.ApplyProgress();
        this.AnimTick = setInterval(() => {
            this.Progress = Math.min(1, this.Progress + 0.02);
            this.ApplyProgress();
            if (this.Progress >= 1 && this.AnimTick) {
                clearInterval(this.AnimTick);
                this.AnimTick = 0;
            }
        }, 30) as unknown as number;
    }

    private ApplyProgress() {
        this.Progress_Test.SetPercent(this.Progress);
        this.Txt_Progress.SetText(`${Math.round(this.Progress * 100)}%`);
    }
}

blueprint.mixin(BPToJs, UI_TestWidgets);
export default UI_TestWidgets;
