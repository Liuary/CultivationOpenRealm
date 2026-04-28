// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CORUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class CORUI_API UCORUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 显示UI
	UFUNCTION(BlueprintCallable, Category = "COR")
	virtual void Show();

	// 隐藏UI
	UFUNCTION(BlueprintCallable, Category = "COR")
	virtual void Hide();

	// 销毁UI
	UFUNCTION(BlueprintCallable, Category = "COR")
	virtual void NativeDestruct() override;
	
	// 设置控件可见性
	virtual void SetVisibility(ESlateVisibility InVisibility) override;

	/*
	 * 设置控件可见性（控制触发显隐事件）
	 * @param InVisibility ESlateVisibility 新的可见性
	 * @param TriggerEvent bool 是否触发事件
	 */
	void SetVisibilityForEvent(ESlateVisibility InVisibility, bool TriggerEvent = true);

	// 检查控件可见性
	bool IsVisible() const;

	// 窗口显示（允许蓝图重写）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnShow();

	// 窗口隐藏（允许蓝图重写）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnHide();
	
	//控件打开（允许蓝图重写）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "COR")
	void OnOpen();
	
	// 控件关闭（允许蓝图重写）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "COR")
	void OnClose();

protected:
	// 窗口显示
	virtual void NativeShow();

	// 窗口隐藏
	virtual void NativeHide();
	
	// 打开控件
	virtual void NativeOpen();
	
	// 关闭控件
	virtual void NativeClose();
	
private:
	// 标记是否显示
	bool bIsShowing = false;
};
