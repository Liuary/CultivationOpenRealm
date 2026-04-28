// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from "../../../Utils/UISingle/UISingle";
import Log from '../../../Utils/Log/Log';
import LogType from '../../../Utils/Log/LogType';
import TimeManager from '../../../Manager/TimeManager';
import UISingleType from '../../../Utils/UISingle/UISingleType';

const BP_UI_GameLoading_Class = UE.Class.Load('/Game/UI/GameLoading/UI_GameLoading.UI_GameLoading_C');
const BP_UI_GameLoading = blueprint.tojs<typeof UE.Game.UI.GameLoading.UI_GameLoading.UI_GameLoading_C>(BP_UI_GameLoading_Class);

interface UI_GameLoading extends UE.Game.UI.GameLoading.UI_GameLoading.UI_GameLoading_C {}

class UI_GameLoading {

	//#region 属性

	/** 检查时间 */
	private _CheckTime: number = 0;
	/** 检查间隔时间（毫秒） */
	private _CheckIntervalTime: number = 100;
	
	/** 当前进度 */
	private _CurrentProgress: number = 0;
	/** 目标进度 */
	private _TargetProgress: number = 100;

	/** 是否已经完成 */
	private _IsCompleted: boolean = false;

	//#region 定时器

	/** 模拟Tick */
	private _TickInterval: number = 0;
	/** 模拟Tick的间隔时间（毫秒） */
	private _TickIntervalTime: number = 25;

	//#endregion 定时器


	//#endregion 属性

	//#region 生命周期
	Construct() {
		this._CheckTime = 0;
		this._CheckIntervalTime = 100;
		this._TickInterval = 0;
		this._TickIntervalTime = 25;
		this._CurrentProgress = 0;
		this._TargetProgress = 100;
	}
	Destruct() {

	}
	OnOpen() {
		Log.Log(LogType.UI, `[UI_GameLoading][OnOpen]`);

		this._TickInterval = setInterval(()=>{
			this.OnTick();
		}, this._TickIntervalTime);
		this._IsCompleted = false;

	}
	OnClose() {
		Log.Log(LogType.UI, `[UI_GameLoading][OnClose]`);

		if (this._TickInterval) {
			clearInterval(this._TickInterval);
			this._TickInterval = 0;
		}

	}
	OnShow() {

	}
	OnHide() {

	}
	OnScriptInit(Single: UISingle) {
	}

	//#endregion 生命周期

	//#region 方法
	OnTick(): void{

		if (this._IsCompleted) {
			return;
		}

		let CurrentTime = TimeManager.Instance.GetCurrentTime();
		if (CurrentTime - this._CheckTime > this._CheckIntervalTime) {
			this._CheckTime = CurrentTime;
			this.CheckLoading();
		}
	}

	private CheckLoading(): void {
		Log.Log(LogType.UI, `[UI_GameLoading][CheckLoading] CurrentProgress: ${this._CurrentProgress}, TargetProgress: ${this._TargetProgress}`);

		// 生成一个随机的[1, 3]的进度
		let RandomProgress = Math.floor(Math.random() * 3) + 1;
		this._CurrentProgress += RandomProgress;
		this._CurrentProgress = Math.min(this._CurrentProgress, this._TargetProgress);

		let Progress = this._CurrentProgress / this._TargetProgress;

		// 设置进度条
		this.Progress_Loading.SetPercent(Progress);

		// 设置进度值
		this.Txt_Progress.SetText(`${this._CurrentProgress}/${this._TargetProgress}`);

		if (this._CurrentProgress >= this._TargetProgress) {

			this.Switcher_Load.SetActiveWidget(this.Txt_Loaded);

			this.OnLoadingComplete();
		} else {
			this.Switcher_Load.SetActiveWidget(this.Txt_Loading);
		}
	}

	private OnLoadingComplete(): void {
		Log.Log(LogType.UI, `[UI_GameLoading][OnLoadingComplete] Loading Complete. CurrentProgress: ${this._CurrentProgress}, TargetProgress: ${this._TargetProgress}`);
		this._IsCompleted = true;
	}

	//#endregion 方法
}
blueprint.mixin(BP_UI_GameLoading, UI_GameLoading);
