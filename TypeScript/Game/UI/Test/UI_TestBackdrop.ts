// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from '../../../Utils/UISingle/UISingle';
import TestUIManager from '../../../Manager/TestUIManager';

const BP_UI_TestBackdrop = UE.Class.Load('/Game/UI/TestSuite/UI_TestBackdrop.UI_TestBackdrop_C');
const BPToJs = blueprint.tojs<any>(BP_UI_TestBackdrop);

interface UI_TestBackdrop extends UE.UserWidget {
    GetWindowName(): string;
    GetLayer(): UE.EUILayerType;
}

class UI_TestBackdrop {
    Construct() { TestUIManager.Instance.TrackConstruct(this); }
    Destruct() { TestUIManager.Instance.TrackDestruct(this); }
    OnOpen() { TestUIManager.Instance.TrackOpen(this); }
    OnClose() { TestUIManager.Instance.TrackClose(this); }
    OnShow() { TestUIManager.Instance.TrackShow(this); }
    OnHide() { TestUIManager.Instance.TrackHide(this); }
    OnCover() { TestUIManager.Instance.TrackCover(this); }
    OnScriptInit(Single: UISingle) {
    }
}

blueprint.mixin(BPToJs, UI_TestBackdrop);
export default UI_TestBackdrop;
