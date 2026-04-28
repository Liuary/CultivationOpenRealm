// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import * as cpp from "cpp";
import { blueprint, toDelegate } from 'puerts';
import UISingle from "../../../Utils/UISingle/UISingle";
import UISingleType from '../../../Utils/UISingle/UISingleType';
import Log from '../../../Utils/Log/Log';
import LogType from '../../../Utils/Log/LogType';
import UI_GameStartSingleType from './UI_GameStartSingleType';
import GameLoadingManager from '../../../Manager/GameLoadingManager';

const BP_UI_GameStart_Class = UE.Class.Load('/Game/UI/GameStart/UI_GameStart.UI_GameStart_C');
const BP_UI_GameStart = blueprint.tojs<typeof UE.Game.UI.GameStart.UI_GameStart.UI_GameStart_C>(BP_UI_GameStart_Class);

interface UI_GameStart extends UE.Game.UI.GameStart.UI_GameStart.UI_GameStart_C {}

class UI_GameStart {

	//#region UI 生命周期
	Construct() {
		Log.Log(LogType.UI, `[UI_GameStart][Construct] constructed.`);

		this.OnFadIn = this.OnFadIn.bind(this);
		this.OnFadOut = this.OnFadOut.bind(this);
	}
	Destruct() {
		Log.Log(LogType.UI, `[UI_GameStart][Destruct] destructed.`);
	}

	OnOpen() {
		Log.Log(LogType.UI, `[UI_GameStart][OnOpen]`);

		this.Ani_FadeIn.BindToAnimationFinished(this, toDelegate(this, this.OnFadIn.bind(this)));
		this.Ani_FadeOut.BindToAnimationFinished(this, toDelegate(this, this.OnFadOut.bind(this)));
	}
	OnClose() {
		Log.Log(LogType.UI, `[UI_GameStart][OnClose]`);

		this.Ani_FadeIn.UnbindAllFromAnimationFinished(this);
		this.Ani_FadeOut.UnbindAllFromAnimationFinished(this);
	}

	OnScriptInit(Single: UISingle) {
		this.ImgPathList = Single.GetSingle<string[]>(Number(UI_GameStartSingleType.ImageList));

		this.Start();
	}
	OnShow() {
		Log.Log(LogType.UI, `[UI_GameStart][OnShow] show.`);
	}
	OnHide() {
		Log.Log(LogType.UI, `[UI_GameStart][OnHide] hide.`);
	}
	//#endregion UI 生命周期

	private ImgPathList: string[] = null;
	private TextList: string[] = null;

	private SetStartImg(ImgPath: string) {
		this.Img_Start.SetImagePath(ImgPath);
	}

	private Start() {
		if (!this.ImgPathList || this.ImgPathList.length == 0) {
			Log.Log(LogType.UI, `[UI_GameStart][Start] ImgPathList is null or empty.`);
			this.CloseUI();
			GameLoadingManager.Instance.LoadingGame();
			return;
		}
		
		// 获取首个图片，并移除
		let ImgPath = this.ImgPathList.shift();
		this.SetStartImg(ImgPath);

		this.PlayAnimation(this.Ani_FadeIn, 0, 1, UE.EUMGSequencePlayMode.Forward, 1, false);
	}

	private OnFadIn() {
		Log.Log(LogType.UI, `[UI_GameStart][OnFadIn] fade in.`);
		this.PlayAnimation(this.Ani_FadeOut, 0, 1, UE.EUMGSequencePlayMode.Forward, 1, false);
	}

	private OnFadOut() {
		Log.Log(LogType.UI, `[UI_GameStart][OnFadOut] fade out.`);
		this.Start();
	}

}
blueprint.mixin(BP_UI_GameStart, UI_GameStart);
