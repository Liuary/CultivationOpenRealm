// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';
import UI_TestSingleType from './UI_TestSingleType';

const BP_UI_TestToast = UE.Class.Load('/Game/UI/TestSuite/UI_TestToast.UI_TestToast_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestToast);

interface UI_TestToast extends UE.UserWidget {
    Txt_Toast_Title: UE.TextBlock;
    Txt_Toast_Body: UE.TextBlock;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestToast {
    private CloseTimer: number = 0;

    Construct() { TestUIManager.Instance.TrackConstruct(this); }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); if (this.CloseTimer) { clearTimeout(this.CloseTimer); this.CloseTimer = 0; } }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        if (this.CloseTimer) {
            clearTimeout(this.CloseTimer);
        }
        // 消息层：不提供关闭按钮，3.2 秒后自行收起。
        this.CloseTimer = setTimeout(() => this.CloseUI(true), 3200) as unknown as number;
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        if (this.CloseTimer) {
            clearTimeout(this.CloseTimer);
            this.CloseTimer = 0;
        }
    }

    OnScriptInit(Single: UISingle) {
        if (!Single) {
            return;
        }
        const Title = Single.GetSingle<string>(Number(UI_TestSingleType.Title));
        const Body = Single.GetSingle<string>(Number(UI_TestSingleType.Body));
        if (Title) {
            this.Txt_Toast_Title.SetText(Title);
        }
        if (Body) {
            this.Txt_Toast_Body.SetText(Body);
        }

        // 消息层纵向排队：按槽位下移，避免多条 Toast 重叠
        const Slot = Number(Single.GetSingle<number>(Number(UI_TestSingleType.Index)));
        if (!isNaN(Slot) && Slot > 0) {
            this.SetRenderTranslation(new UE.Vector2D(0, Slot * TestUIManager.ToastSlotHeight));
        }
    }
}

blueprint.mixin(BPToJs, UI_TestToast);
export default UI_TestToast;
