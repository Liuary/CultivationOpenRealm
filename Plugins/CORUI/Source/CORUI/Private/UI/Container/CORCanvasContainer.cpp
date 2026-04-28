// Fill out your copyright notice in the Description page of Project Settings.


#include "CORUI/Public/UI/Container/CORCanvasContainer.h"

#include "Components/CanvasPanelSlot.h"
#include "UI/Widget/CORFullScreen.h"
#include "UI/Widget/CORWindow.h"

void UCORCanvasContainer::UpdateUI()
{
	
}

FName UCORCanvasContainer::PushUI(FName WindowName, UCORWindow* WindowInstance)
{

	FScopeLock Lock(&ContainerLock);

	if (WindowName == NAME_None)
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORCanvasContainer::PushUI >> WindowName is None"));
		return NAME_None;
	}
	
	// 将UI添加到层级
	UCanvasPanelSlot* CanvasSlot = AddChildToCanvas(WindowInstance);
	if (!IsValid(CanvasSlot))
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORCanvasContainer::PushUI >> Slot is not valid"))
		return NAME_None;
	}

	// 保存UI实例的弱指针
	Windows.Add(WindowName, WindowInstance);
	// 添加UI到逻辑栈
	WindowsStack.Push(WindowInstance);
	
	// 计算ZOrder
	int32 WindowZOrder = WindowInstance->GetWindowZOrder();
	if (WindowZOrder <= ChildMaxZOrder)
	{
		WindowZOrder = ChildMaxZOrder + 1;
	}
	ChildMaxZOrder = WindowZOrder;

	// 设置Window autosize 并铺满
	CanvasSlot->SetAutoSize(true);
	CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	CanvasSlot->SetPosition(FVector2D::ZeroVector);
	CanvasSlot->SetSize(FVector2D::ZeroVector);
	// 设置ZOrder
	CanvasSlot->SetZOrder(WindowZOrder);
	
	WindowInstance->SetWindowZOrder(WindowZOrder);
	WindowInstance->SetWindowName(WindowName);

	return WindowName;
	
}

void UCORCanvasContainer::PopUI(FName WindowName, bool bPopSelf)
{
	FScopeLock Lock(&ContainerLock);
	
	// 如果传入了未知，则移除最顶层的UI
	if (WindowName == NAME_None)
	{
		// 从栈中取出最顶层的UI
		if (WindowsStack.Num() > 0)
		{
			TWeakObjectPtr<UCORWindow> TopWindow = WindowsStack.Top();
			if (TopWindow.IsValid())
			{
				NativeRemoveWindow(TopWindow.Get());
			}
		}
		return;
	}

	// 从缓存的弱指针Map找到对应Window
	UCORWindow* Window = FindWindow(WindowName);
	if (!Window)
	{
		return;
	}
	// 从栈中查找到目标UI索引，如果不存在，则不做任何处理
	int32 WindowIndex = WindowsStack.Find(Window);
	if (WindowIndex < 0)
	{
		return;
	}

	// 检查Window类型，如果是FullScreen，则进入下一步判定，否则直接移除
	if (UCORFullScreen* FullScreen = Cast<UCORFullScreen>(Window))
	{
		// 如果不需要弹出自身，那么索引+1
		if (!bPopSelf)
		{
			WindowIndex++;
		}
		
		int32 TargetIndex = bPopSelf ? WindowIndex : WindowIndex + 1;
		
		// 倒序移除UI
		for (int32 i = WindowsStack.Num() - 1; i >= TargetIndex; i--)
		{
			TWeakObjectPtr<UCORWindow> RemoveWindow = WindowsStack[i];
			if (RemoveWindow.IsValid())
			{
				NativeRemoveWindow(RemoveWindow.Get());
			}
		}
	}
	// 如果不是FullScreen，那么仅需移除自身
	else
	{
		NativeRemoveWindow(Window);
	}

	// 刷新界面状态
	UpdateUI();
	
}

void UCORCanvasContainer::NativeRemoveWindow(UCORWindow* WindowInstance)
{
	FScopeLock Lock(&ContainerLock);
	
	if (!IsValid(WindowInstance))
	{
		return;
	}
	
	WindowsStack.Remove(WindowInstance);
	Windows.Remove(WindowInstance->GetWindowName());
	WindowInstance->Hide();
	WindowInstance->NativeClose();
	WindowInstance->RemoveFromParent();
	UpdateWindowState(EWindowState::None);
}

uint32 UCORCanvasContainer::UpdateWindowState(uint32 WindowState)
{
	FScopeLock Lock(&ContainerLock);

	// 如果LayerPanel为隐藏，则直接返回
	if (GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden)
	{
		return WindowState;
	}

	// 从栈顶窗口向下检查状态
	for (int32 i = WindowsStack.Num() - 1; i >= 0; i--)
	{
		TWeakObjectPtr<UCORWindow> CheckWindow = WindowsStack[i];
		if (!CheckWindow.IsValid())
		{
			continue;
		}

		WindowState = CheckWindow.Get()->NativeUpdateState(WindowState);
	}
	
	return WindowState;
}

UCORWindow* UCORCanvasContainer::FindWindow(FName WindowName)
{
	FScopeLock Lock(&ContainerLock);
	
	TWeakObjectPtr<class UCORWindow>* WeakWindow = Windows.Find(WindowName);
	if (WeakWindow && WeakWindow->IsValid())
	{
		return WeakWindow->Get();
	}
	return nullptr;
}
