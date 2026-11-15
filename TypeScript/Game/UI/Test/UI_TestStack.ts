// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';
import { TestLayer, TestWindowNames } from './TestPaths';

const BP_UI_TestStack = UE.Class.Load('/Game/UI/TestSuite/UI_TestStack.UI_TestStack_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestStack);

interface UI_TestStack extends UE.UserWidget {
    Btn_Push: UE.Button;
    Btn_Repush: UE.Button;
    Btn_PopTop: UE.Button;
    Btn_PopBelow: UE.Button;
    Btn_PopAll: UE.Button;
    Btn_Cover: UE.Button;
    Btn_Back: UE.Button;
    Txt_Rail_0: UE.TextBlock;
    Txt_Rail_1: UE.TextBlock;
    Txt_Rail_2: UE.TextBlock;
    Txt_Rail_3: UE.TextBlock;
    Txt_Rail_4: UE.TextBlock;
    Txt_Log_0: UE.TextBlock;
    Txt_Log_1: UE.TextBlock;
    Txt_Log_2: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestStack {
    private TimerId: number = 0;
    private Seq: number = 0;

    Construct() { TestUIManager.Instance.TrackConstruct(this); }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); if (this.TimerId) { clearInterval(this.TimerId); this.TimerId = 0; } }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.Seq = 0;
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

    private Cards(): string[] {
        return TestUIManager.Instance.GetLayerNames(TestLayer.Normal)
            .filter((N) => N.indexOf(TestWindowNames.CardPrefix) === 0);
    }

    private Bind() {
        [this.Btn_Push, this.Btn_Repush, this.Btn_PopTop, this.Btn_PopBelow,
            this.Btn_PopAll, this.Btn_Cover, this.Btn_Back].forEach((B) => B.OnClicked.Clear());

        this.Btn_Push.OnClicked.Add(() => {
            this.Seq++;
            TestUIManager.Instance.PushCard(`卡片 ${this.Seq}`, `${this.Seq}`, '推入一张卡片，观察栈深度。');
            this.Refresh();
        });
        this.Btn_Repush.OnClicked.Add(() => {
            const Cards = this.Cards();
            if (Cards.length === 0) {
                TestUIManager.Instance.LogEvent('窗口栈', '同名重推失败：栈为空');
                return;
            }
            const Last = Cards[Cards.length - 1];
            TestUIManager.Instance.PushCardNamed(Last, '同名 · 卡片', '—', '同名窗口先被弹出，再回到栈顶。');
            this.Refresh();
        });
        this.Btn_PopTop.OnClicked.Add(() => {
            TestUIManager.Instance.Pop('', TestLayer.Normal, true);
            this.Refresh();
        });
        this.Btn_PopBelow.OnClicked.Add(() => {
            const Cards = this.Cards();
            if (Cards.length === 0) {
                TestUIManager.Instance.LogEvent('窗口栈', '弹出失败：栈为空');
                return;
            }
            TestUIManager.Instance.LogEvent('窗口栈', 'bPopSelf=false：普通窗口只移除自身');
            TestUIManager.Instance.Pop(Cards[0], TestLayer.Normal, false);
            this.Refresh();
        });
        this.Btn_PopAll.OnClicked.Add(() => {
            this.Cards().forEach((N) => TestUIManager.Instance.Pop(N, TestLayer.Normal, true));
            this.Refresh();
        });
        this.Btn_Cover.OnClicked.Add(() => {
            TestUIManager.Instance.PushConfirm('覆层测试：下方卡片应进入 HiddenUnder；关闭后恢复。', TestLayer.Normal);
            this.Refresh();
        });
        this.Btn_Back.OnClicked.Add(() => this.CloseUI(true));
    }

    private Unbind() {
        [this.Btn_Push, this.Btn_Repush, this.Btn_PopTop, this.Btn_PopBelow,
            this.Btn_PopAll, this.Btn_Cover, this.Btn_Back].forEach((B) => B.OnClicked.Clear());
    }

    private Refresh() {
        const Cards = this.Cards();
        const Rails = [this.Txt_Rail_0, this.Txt_Rail_1, this.Txt_Rail_2, this.Txt_Rail_3, this.Txt_Rail_4];
        Rails.forEach((Rail, I) => {
            const Name = Cards[I];
            Rail.SetText(Name ? `${I + 1}  ${Name}` : `${I + 1}  —`);
        });

        const Events = TestUIManager.Instance.GetEvents(3);
        const Lines = [this.Txt_Log_0, this.Txt_Log_1, this.Txt_Log_2];
        Lines.forEach((Line, I) => {
            const E = Events[I];
            Line.SetText(E ? `${E.time}  ${E.window}  ${E.action}` : '—');
        });
    }
}

blueprint.mixin(BPToJs, UI_TestStack);
export default UI_TestStack;
