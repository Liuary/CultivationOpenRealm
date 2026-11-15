// PuertsMixinGenerator_Auto_Generator_Flag
import * as UE from 'ue';
import { blueprint } from 'puerts';
import UISingle from "../../../Utils/UISingle/UISingle";
import Log from '../../../Utils/Log/Log';
import LogType from '../../../Utils/Log/LogType';
import TimeManager from '../../../Manager/TimeManager';
import TestUIManager from '../../../Manager/TestUIManager';
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

	/** 加载完成后的延时退出定时器（激活私有，关闭/析构时必须取消） */
	private _ExitTimer: number = 0;

	/** 激活代际：池复用后对象仍存在但代表新的激活，旧回调不得操作新激活 */
	private _Activation: number = 0;

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
		// Puerts mixin 不会执行 constructor，类字段初始值不会自动生效，必须在这里显式初始化
		this._ExitTimer = 0;
		this._Activation = 0;
	}
	Destruct() {
		// 析构时激活私有定时器必须全部取消，避免池复用后旧回调操作新激活
		this.ClearActivationTimers();
	}
	OnOpen() {
		Log.Log(LogType.UI, `[UI_GameLoading][OnOpen]`);

		// 新的激活代际：先清掉上一次激活遗留的定时器
		this.ClearActivationTimers();
		this._Activation++;

		this._TickInterval = setInterval(()=>{
			this.OnTick();
		}, this._TickIntervalTime);
		this._IsCompleted = false;

	}
	OnClose() {
		Log.Log(LogType.UI, `[UI_GameLoading][OnClose]`);

		this.ClearActivationTimers();
	}
	OnShow() {

	}
	OnHide() {

	}
	OnScriptInit(Single: UISingle) {
	}

	//#endregion 生命周期

	//#region 方法

	/** 取消当前激活持有的全部定时器 */
	private ClearActivationTimers(): void {
		if (this._TickInterval) {
			clearInterval(this._TickInterval);
			this._TickInterval = 0;
		}
		if (this._ExitTimer) {
			clearTimeout(this._ExitTimer);
			this._ExitTimer = 0;
		}
	}

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

		// 开发期入口：加载完成后打开《观澜》测试台，并收起加载界面。
		if (TestUIManager.Enabled) {
			// 记录本次激活代际：窗口被关闭/池复用后打开的可能是新激活，旧回调必须放弃
			const Activation = this._Activation;
			this._ExitTimer = setTimeout(() => {
				this._ExitTimer = 0;

				if (Activation !== this._Activation) {
					Log.Log(LogType.UI, `[UI_GameLoading][OnLoadingComplete] stale callback ignored.`);
					return;
				}

				// 先收起加载界面，再打开观澜台：否则 CloseUI(true) 会把刚打开的窗口一起弹掉
				this.CloseUI(true);
				TestUIManager.Instance.OpenHub();
			}, 400) as unknown as number;
		}
	}

	//#endregion 方法
}
blueprint.mixin(BP_UI_GameLoading, UI_GameLoading);
