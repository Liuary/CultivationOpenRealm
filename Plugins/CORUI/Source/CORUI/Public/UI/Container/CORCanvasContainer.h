// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CORUITypes.h"
#include "Components/CanvasPanel.h"
#include "CORCanvasContainer.generated.h"

// Widget对象池（非拥有引用，仅做前置声明，避免头文件循环依赖）
struct FUserWidgetPool;

/**
 * 分层容器
 */
UCLASS()
class CORUI_API UCORCanvasContainer : public UCanvasPanel
{
	GENERATED_BODY()

public:
	// 用于标记分层容器类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "COR|UI")
	EUILayerType LayerType = EUILayerType::Normal;

	// 分层容器的层级配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "COR|UI")
	int32 LayerZOrder = 0;

public:
	// 更新UI
	void UpdateUI();

	/**
	 * 添加一个UI
	 * - 同一激活在栈内只保留一个条目：已存在的条目会被移动到栈顶，而不是重复入栈
	 * @param WindowName UI命名 
	 * @param WindowInstance UI实例
	 * @return 
	 */
	FName PushUI(FName WindowName, class UCORWindow* WindowInstance);

	/**
	 * 移除一个UI
	 * 语义契约：
	 * - bPopSelf=true：移除目标窗口及其上方的全部窗口
	 * - bPopSelf=false：只移除目标窗口上方的窗口，目标自身保留在栈内（FullScreen 复用语义）
	 * - 生命周期回调派发期间调用会入队，在派发出口统一执行，避免遍历中修改栈
	 * @param WindowName UI命名 
	 * @param bPopSelf 是否移除这个UI本身，默认为移除
	 */
	void PopUI(FName WindowName = NAME_None, bool bPopSelf = true);

	// 移除一个窗口，并且从Windows和WindowsStack中删除相关数据
	void NativeRemoveWindow(class UCORWindow* WindowInstance);

	// 更新窗口的状态
	uint32 UpdateWindowState(uint32 WindowState);

	// 根据FName获取一个当前Layer的Window
	class UCORWindow* FindWindow(FName WindowName);

	/**
	 * 设置Widget对象池（非拥有）
	 * @param InPool 由UIManager持有的对象池，窗口移除时会归还到该池
	 */
	void SetWidgetPool(FUserWidgetPool* InPool) { WidgetPool = InPool; }

	// 移除当前层内的全部窗口，并归还对象池
	void ClearAllWindows();

private:

	/**
	 * 把窗口从容器中剥离（移除Slate父子关系、触发关闭），但不销毁底层Slate
	 * 注意：必须在 ContainerLock 内调用
	 * @param WindowInstance 目标窗口
	 */
	void DetachWindow(class UCORWindow* WindowInstance);

	/**
	 * 把窗口归还对象池并销毁缓存的SObjectWidget，从而触发 NativeDestruct
	 * 注意：必须在 ContainerLock 之外调用（Release 会触发 NativeDestruct，可能回调容器）
	 * @param WindowsToRelease 待归还的窗口
	 */
	void ReleaseWindows(const TArray<class UCORWindow*>& WindowsToRelease);

	// 存储当前Layer的全部窗口
	TMap<FName, TWeakObjectPtr<class UCORWindow>> Windows;
	// 存储子控件的逻辑栈
	TArray<TWeakObjectPtr<class UCORWindow>> WindowsStack;

	// 标记子控件的最高ZOrder
	int32 ChildMaxZOrder = 0;

	// 锁
	mutable FCriticalSection ContainerLock; 

	// Widget对象池（非拥有，由UIManager注入）
	FUserWidgetPool* WidgetPool = nullptr;

	// 一次等待执行的弹出请求（生命周期回调派发期间累积，派发出口统一执行）
	struct FPendingPopRequest
	{
		FName WindowName = NAME_None;
		bool bPopSelf = true;
	};

	// 判断窗口是否仍在本容器账本中（回调可能已关闭/替换它）
	bool IsWindowRegistered(const class UCORWindow* WindowInstance) const;

	// 执行派发期间累积的弹出请求；只在最外层派发出口调用，返回（可能重算后的）窗口状态
	uint32 DrainPendingMutations(uint32 CurrentState);

	// 生命周期派发深度（>0 表示正在调用可重写的生命周期回调，此时不得遍历实时栈）
	int32 DispatchDepth = 0;
	// 派发期间累积的弹出请求
	TArray<FPendingPopRequest> PendingPops;
	// 派发期间累积的单窗口移除请求（NativeRemoveWindow）
	TArray<TWeakObjectPtr<class UCORWindow>> PendingRemoves;
	// 派发期间收到的状态刷新请求
	bool bStateUpdateRequested = false;

private:
	friend class UCORUIManager;
	UPanelSlot* GetSlot() {return Slot;};
};
