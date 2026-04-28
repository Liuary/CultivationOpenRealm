// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CORUITypes.h"
#include "Components/CanvasPanel.h"
#include "CORCanvasContainer.generated.h"

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
	 * @param WindowName UI命名 
	 * @param WindowInstance UI实例
	 * @return 
	 */
	FName PushUI(FName WindowName, class UCORWindow* WindowInstance);

	/**
	 * 移除一个UI
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

private:

	// 存储当前Layer的全部窗口
	TMap<FName, TWeakObjectPtr<class UCORWindow>> Windows;
	// 存储子控件的逻辑栈
	TArray<TWeakObjectPtr<class UCORWindow>> WindowsStack;

	// 标记子控件的最高ZOrder
	int32 ChildMaxZOrder = 0;

	// 锁
	mutable FCriticalSection ContainerLock; 

private:
	friend class UCORUIManager;
	UPanelSlot* GetSlot() {return Slot;};
};
