// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';
import UI_TestSingleType from './UI_TestSingleType';

const BP_UI_TestConfirm = UE.Class.Load('/Game/UI/TestSuite/UI_TestConfirm.UI_TestConfirm_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestConfirm);

interface UI_TestConfirm extends UE.UserWidget {
    Txt_Confirm_Body: UE.TextBlock;
    Btn_Cancel: UE.Button;
    Btn_Confirm: UE.Button;
    UnderWindowHide: boolean;
    CloseUI(bCloseSelf?: boolean): void;
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestConfirm {
    Construct() {
        // 模态：让下方窗口进入 HiddenUnder。Construct 早于首次状态更新。
        this.UnderWindowHide = true;
        TestUIManager.Instance.TrackConstruct(this);
    }

    Destruct() { TestUIManager.Instance.TrackDestruct(this); }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }

    OnOpen() {
        TestUIManager.Instance.TrackOpen(this);
        this.Btn_Cancel.OnClicked.Clear();
        this.Btn_Confirm.OnClicked.Clear();
        this.Btn_Cancel.OnClicked.Add(() => {
            TestUIManager.Instance.LogEvent(this.GetWindowName(), '取消');
            this.CloseUI(true);
        });
        this.Btn_Confirm.OnClicked.Add(() => {
            TestUIManager.Instance.LogEvent(this.GetWindowName(), '确认');
            this.CloseUI(true);
        });
    }

    OnClose() {
        TestUIManager.Instance.TrackClose(this);
        this.Btn_Cancel.OnClicked.Clear();
        this.Btn_Confirm.OnClicked.Clear();
    }

    OnScriptInit(Single: UISingle) {
        const Body = Single ? Single.GetSingle<string>(Number(UI_TestSingleType.Body)) : null;
        if (Body) {
            this.Txt_Confirm_Body.SetText(Body);
        }
    }
}

blueprint.mixin(BPToJs, UI_TestConfirm);
export default UI_TestConfirm;
