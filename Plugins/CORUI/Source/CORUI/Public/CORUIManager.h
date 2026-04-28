#pragma once
#include "CoreMinimal.h"
#include "CORUITypes.h"
#include "JsObject.h"
#include "Blueprint/UserWidgetPool.h"
#include "Runtime/Engine/Public/Subsystems/GameInstanceSubsystem.h"
#include "CORUIManager.generated.h"

// 声明一个无参动态多播委托，用来通知UI管理器的创建和销毁
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCORUIManagerStateNotify);

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

	// 获取层级枚举对应的ZOrder
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetLayerZOrder(EUILayerType Layer);

	// 获取指定层级
	UFUNCTION(BlueprintCallable, BlueprintPure)
	class UCORCanvasContainer* GetLayer(EUILayerType Layer);

private:
	// UI层级ZOrder配置
	TMap<EUILayerType, int32> LayersZOrder;
	// UI层级枚举
	TArray<EUILayerType> LayerTypes;

	// 根节点
	TWeakObjectPtr<class UCanvasPanel> RootCanvas;

	// 分层容器
	TMap<EUILayerType, TWeakObjectPtr<class UCORCanvasContainer>> LayerPanels;

	// Widget对象池
	FUserWidgetPool WidgetPool;

	// 原子锁
	FCriticalSection DataLock;

protected:
	FDelegateHandle UIBeginPlayHandle;
	void OnUIBeginPlay(AGameModeBase* GameMode);
	
	FDelegateHandle UIEndPlayHandle;
	void OnUIEndPlay();

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
