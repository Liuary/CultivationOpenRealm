#pragma once
#include "CoreMinimal.h"
#include "CORUITypes.h"
#include "JsObject.h"
#include "Blueprint/UserWidgetPool.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/StreamableManager.h"
#include "Runtime/Engine/Public/Subsystems/GameInstanceSubsystem.h"
#include "CORUIManager.generated.h"

// 声明一个无参动态多播委托，用来通知UI管理器的创建和销毁
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCORUIManagerStateNotify);

// 一次异步 Push 请求的完整状态（exactly-once 终止契约的载体）
struct FPendingPushRequest
{
	int32 RequestId = 0;
	// 发起请求时的 UI 会话代际
	int32 Generation = 0;
	FSoftClassPath WindowClassPath;
	FName WindowName = NAME_None;
	EUILayerType PushLayer = EUILayerType::None;
	FJsObject Callback;
	// 是否已经发出过终止回调（成功/失败/取消都只允许一次）
	bool bCompleted = false;
};

// 在途请求：句柄用于 Teardown 时取消
struct FInFlightPushRequest
{
	TSharedPtr<FPendingPushRequest> Request;
	TSharedPtr<FStreamableHandle> Handle;
};

/**
 * UI管理器
 */
UCLASS(Blueprintable)
class CORUI_API UCORUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 获取全部UI层级枚举
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<EUILayerType> GetLayerTypes();

	// UI 是否已就绪（窗口可安全 Push、脚本生命周期可安全回调）
	// 供 JS 侧在晚订阅时判定：已就绪则立即初始化，避免错过唯一一次 OnUIManagerInit
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsUIInitialized() const;

#if !UE_BUILD_SHIPPING
	/**
	 * F1 验证：触发一次全量 GC，随后校验 UI 所有权链（根面板 / 层容器 / 对象池）是否仍然有效。
	 *
	 * 存在的意义：F1（根 UI 所有权对 GC 不可见）的修复只是把裸/弱引用改成 UPROPERTY，
	 * "GC 看得见"不等于"运行期扛得住"，所以补一条可回归的运行期存活断言。
	 * 注意：UPanelSlot::Parent 是 UPROPERTY(Instanced) 反向强引用根面板，即使把 RootCanvas
	 * 的 UPROPERTY 回退，root 断言仍会 PASS——它是存活冒烟探针，不是对 F1 缺口的独立证明。
	 *
	 * @param OutReport 分项断言结果，'|' 连接；INFO 项无判别力、不计入判定；未就绪时给出 SKIP 原因
	 * @return true=全部判定项通过
	 */
	bool VerifyOwnershipAfterGC(FString& OutReport);
#endif

	// 协调关闭：销毁 JS 环境之前先由 JsEnvManager 调用，保证窗口在 JS 存活时关闭
	void ShutdownUI();

	// 获取层级枚举对应的ZOrder
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetLayerZOrder(EUILayerType Layer);

	// 获取指定层级
	UFUNCTION(BlueprintCallable, BlueprintPure)
	class UCORCanvasContainer* GetLayer(EUILayerType Layer);

private:
	// 幂等的 UI 会话相位：Created -> Starting -> Ready -> Stopping -> Stopped
	enum class ESessionPhase : uint8
	{
		Created,
		Starting,
		Ready,
		Stopping,
		Stopped,
	};

	// 当前会话相位
	ESessionPhase SessionPhase = ESessionPhase::Created;

	// 世界清理回调：换图/结束 PIE 时统一走关闭边界
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	FDelegateHandle UIWorldCleanupHandle;

	// UI层级ZOrder配置
	TMap<EUILayerType, int32> LayersZOrder;
	// UI层级枚举
	TArray<EUILayerType> LayerTypes;

	// 根节点：transient 强持有（UPROPERTY 让 GC 可见），生命周期结束由 ClearUI 显式置空。
	// 注意：viewport 持有 Slate 控件并不构成 UObject GC 引用，因此这里必须是强引用。
	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas;

	// 分层容器：transient 强持有，保证根 canvas 与层容器在显式销毁前始终有效
	UPROPERTY(Transient)
	TMap<EUILayerType, TObjectPtr<class UCORCanvasContainer>> LayerPanels;

	// Widget对象池（UPROPERTY 让 FUserWidgetPool 内部的 ActiveWidgets/InactiveWidgets 数组对 GC 可见）
	UPROPERTY(Transient)
	FUserWidgetPool WidgetPool;

	// 原子锁
	FCriticalSection DataLock;

	// 界面状态刷新重入保护（回调里再次请求刷新时合并到外层）
	bool bIsUpdatingUI = false;
	bool bStateUpdateRequested = false;

	// UI 会话代际：每次 ClearUI（Teardown）递增，用于失效旧会话的在途请求
	int32 UISessionGeneration = 0;
	// Push 请求自增 ID
	int32 NextPushRequestId = 0;
	// 在途异步 Push 请求
	TArray<FInFlightPushRequest> InFlightPushes;
	// 最终销毁期间抑制在途请求的 JS 回调（此时 JS 环境可能正在销毁，且重入 Push 只会再失败一次）
	bool bSuppressPendingCallbacks = false;

protected:
	FDelegateHandle UIBeginPlayHandle;
	void OnUIBeginPlay(AGameModeBase* GameMode);
	
	FDelegateHandle UIEndPlayHandle;
	/**
	 * 统一关闭边界
	 * @param bFinalTeardown true=子系统销毁（清空对外委托）；false=会话级关闭（换图后可重建，保留 JS 订阅）
	 */
	void OnUIEndPlay(bool bFinalTeardown = false);

	// 初始化UI
	virtual void InitUI();
	// 清理UI
	virtual void ClearUI();
	
	/**
	 * 添加窗口到层级
	 * @param WindowClass 窗口蓝图类
	 * @param WindowName FName 窗口命名
	 * @param PushLayer EUILayerType 要Push的目标Layer
	 * @param Callback 操作完成后的回调
	 * @return 窗口命名
	 */
	FName NativePushUI(UClass* WindowClass, FName WindowName = NAME_None, EUILayerType PushLayer = EUILayerType::None, FJsObject Callback = FJsObject());

	/**
	 * 异步加载窗口资产，然后添加窗口到层级 
	 * @param WindowClassPath 窗口类路径
	 * @param WindowName 窗口命名
	 * @param PushLayer 要Push的目标层
	 * @param Callback 操作完成后的回调
	 * @return 窗口命名
	 */
	void AsyncPushUI(FSoftClassPath WindowClassPath, FName WindowName = NAME_None, EUILayerType PushLayer = EUILayerType::None, FJsObject Callback = FJsObject());

	/**
	 * 同步加载窗口资产，然后添加窗口到层级 
	 * @param WindowClassPath 窗口类路径
	 * @param WindowName 窗口命名
	 * @param PushLayer 要Push的目标层
	 * @param Callback 操作完成后的回调
	 * @return 窗口命名
	 */
	void SyncPushUI(FSoftClassPath WindowClassPath, FName WindowName = NAME_None, EUILayerType PushLayer = EUILayerType::None, FJsObject Callback = FJsObject());

	// 异步请求完成入口（只会产生一次终止回调）
	void CompleteAsyncPush(const TSharedPtr<FPendingPushRequest>& Request);
	// 以失败/取消终止一个尚未完成的请求
	void FailPushRequest(const TSharedPtr<FPendingPushRequest>& Request, const TCHAR* Reason);
	// 取消全部在途请求（Teardown 时调用）
	void CancelPendingPushes();
	// 从在途表移除
	void RemoveInFlightPush(const TSharedPtr<FPendingPushRequest>& Request);

	/**
	 * 移除某个窗口
	 * @param WindowName FName 窗口命名
	 * @param PushLayer EUILayerType 要从哪个层移除
	 * @param bPopSelf bool 是否移除自身，如果为false，则只会弹出目标窗口上层的窗口；如果为true，则会移除目标窗口及其顶层的窗口
	 */
	void NativePopUI(FName WindowName, EUILayerType PushLayer, bool bPopSelf = true);

	/**
	 * 界面刷新操作
	 * - 检查当前界面层级，并更新层内界面状态
	 */
	void NativeUpdateUI();

public:
	/**
	 * 加载窗口资产，然后添加窗口到层级 
	 * 完成契约（exactly-once）：
	 * - 成功：目标窗口已插入容器，脚本回调先于 OnOpen/OnShow 执行（用于数据初始化）；
	 *   若脚本初始化期间主动 CloseUI 回滚，本函数返回 None，但成功后不再补发失败回调。
	 * - 失败/取消：空路径、类解析失败、层级未就绪、插入失败、会话切换，均恰好一次失败回调。
	 * @param WindowClassPath 窗口类路径
	 * @param Callback 操作完成后的回调
	 * @param WindowName 窗口命名
	 * @param PushLayer 要Push的目标层
	 * @param bIsAsync 是否异步加载
	 * @return 窗口命名
	 */
	UFUNCTION()
	void PushUI(FString WindowClassPath, FJsObject Callback, FName WindowName = NAME_None, EUILayerType PushLayer = EUILayerType::None, bool bIsAsync = true);

	/**
	 * 移除某个窗口
	 * @param WindowName FName 窗口命名
	 * @param PushLayer EUILayerType 要从哪个层移除
	 * @param bPopSelf bool 是否移除自身，如果为false，则只会弹出目标窗口上层的窗口；如果为true，则会移除目标窗口及其顶层的窗口
	 */
	UFUNCTION()
	void PopUI(FName WindowName, EUILayerType PushLayer, bool bPopSelf = true);

public:

	// UI管理器初始化事件
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
	FOnCORUIManagerStateNotify OnUIManagerInit;

	// UI管理器结束事件
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
	FOnCORUIManagerStateNotify OnUIManagerDeinit;
};
