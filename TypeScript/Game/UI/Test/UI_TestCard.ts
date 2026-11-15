// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';
import UI_TestSingleType from './UI_TestSingleType';

const BP_UI_TestCard = UE.Class.Load('/Game/UI/TestSuite/UI_TestCard2.UI_TestCard2_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestCard);

interface UI_TestCard extends UE.UserWidget {
    Txt_Card_Title: UE.TextBlock;
    Txt_Card_Serial: UE.TextBlock;
    Txt_Card_Body: UE.TextBlock;
    Txt_Card_Stats: UE.TextBlock;
    Btn_Close: UE.Button;
    Btn_Repush: UE.Button;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
    GetClass(): UE.Class;
    GetName(): string;
}

class UI_TestCard {
    private Title: string = '卡片';
    private Serial: string = '01';
    private Body: string = '';

    Construct() { TestUIManager.Instance.TrackConstruct(this); }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.Title = this.Title || '卡片';
        this.Serial = this.Serial || '01';
        this.Body = this.Body || '';
        this.Btn_Close.OnClicked.Clear();
        this.Btn_Repush.OnClicked.Clear();
        this.Btn_Close.OnClicked.Add(() => this.CloseUI(true));
        this.Btn_Repush.OnClicked.Add(() => {
            TestUIManager.Instance.PushCardNamed(this.GetWindowName(), this.Title, this.Serial, this.Body);
        });
        this.RefreshStats();
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        this.Btn_Close.OnClicked.Clear();
        this.Btn_Repush.OnClicked.Clear();
    }

    OnScriptInit(Single: UISingle) {
        if (!Single) {
            return;
        }
        this.Title = Single.GetSingle<string>(Number(UI_TestSingleType.Title)) || this.Title;
        this.Serial = Single.GetSingle<string>(Number(UI_TestSingleType.Serial)) || this.Serial;
        this.Body = Single.GetSingle<string>(Number(UI_TestSingleType.Body)) || this.Body;

        this.Txt_Card_Title.SetText(this.Title);
        this.Txt_Card_Serial.SetText(this.Serial);
        this.Txt_Card_Body.SetText(this.Body);
        this.RefreshStats();
    }

    private RefreshStats() {
        const ClassName = `${this.GetClass().GetName()}`;
        const Construct = TestUIManager.Instance.GetConstructCount(ClassName);
        this.Txt_Card_Stats.SetText(`构造 ${Construct} · 入栈 ${this.Serial} · 实例 #${this.GetName()}`);
    }
}

blueprint.mixin(BPToJs, UI_TestCard);
export default UI_TestCard;
