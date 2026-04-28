// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CORUITypes.h"
#include "CORUserWidget.h"
#include "CORWindow.generated.h"

/**
 * 界面状态枚举
 */
enum EWindowState : uint32
{
	// 无状态
	None			=	0,
	// 已显示
	Shown		 	=	1 << 0,
	// 已隐藏
	Hidden			=	1 << 1,
	// 已被覆盖
	Covered			=	1 << 2,
	// 标记隐藏底层
	HiddenUnder		=	1 << 3,
};

/**
 * UCORWindow为基础窗口，所有窗口蓝图需继承该窗口
 */
UCLASS()
class CORUI_API UCORWindow : public UCORUserWidget
{
	GENERATED_BODY()


public:
	UCORWindow();
	~UCORWindow() = default;

protected:
	// 窗口层级
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "COR")
	EUILayerType WindowLayer = EUILayerType::Normal;

	// 窗口在层级中的相对ZOrder
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "COR")
	int32 WindowZOrder = 0;

	// 是否需要隐藏底部的Window
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "COR")
	bool UnderWindowHide = false;

	// 界面命名（蓝图可见但不可编辑）
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "COR")
	FName WindowName = NAME_None;

public:
	// 获取窗口层级
	EUILayerType GetLayer() const { return WindowLayer; }

	// 设置窗口层级
	void SetLayer(EUILayerType LayerType) { WindowLayer = LayerType; }

	// 获取ZOrder配置
	int32 GetWindowZOrder() const { return WindowZOrder; }

	// 设置ZOrder配置（仅修改配置）
	void SetWindowZOrder(int32 InLayerZOrder) { WindowZOrder = InLayerZOrder; }

	// 获取界面命名
	UFUNCTION(BlueprintCallable, Category = "COR")
	FName GetWindowName() const { return WindowName; }

	// 修改界面命名
	UFUNCTION(BlueprintCallable, Category = "COR")
	void SetWindowName(FName InWindowName) { WindowName = InWindowName; }

	// 窗口被覆盖（允许蓝图重写）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "COR")
	void OnCover();

	// 关闭UI
	UFUNCTION(BlueprintCallable, Category = "COR")
	void CloseUI(bool bCloseSelf = true);

	// 重写PostLoad函数
	virtual void PostLoad() override;

	// 重写PreSave函数
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
protected:
	friend class UCORUIManager;	friend class UCORUIManager;
	friend class UCORCanvasContainer;
	// 初始化窗口
	virtual void NativeOpen() override;
	
	// 销毁窗口
	virtual void NativeClose() override;

	// 窗口被覆盖
	virtual void NativeCover();

	// 关闭当前UI
	virtual void NativeCloseUI(bool bCloseSelf = true);

	/**
	 * 更新状态
	 * @param NewState EWindowState 传入的上层状态
	 * @return 最终计算的当前UI状态
	 */
	uint32 NativeUpdateState(uint32 NewState);

	/**
	 * 更新UI命名
	 */
	void NativeUpdateName();

private:
	// 窗口状态
	uint32 WindowState = EWindowState::None;
};
