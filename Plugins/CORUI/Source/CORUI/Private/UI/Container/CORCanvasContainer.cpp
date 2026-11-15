// Fill out your copyright notice in the Description page of Project Settings.


#include "CORUI/Public/UI/Container/CORCanvasContainer.h"

#include "Blueprint/UserWidgetPool.h"
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

	if (!IsValid(WindowInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORCanvasContainer::PushUI >> WindowInstance is not valid"));
		return NAME_None;
	}

	// 同一激活只保留一个栈条目：FullScreen 在 bPopSelf=false 弹出后会保留自身，
	// 此时重新 Push 必须“移动/复用到栈顶”，不能再次 AddChildToCanvas 并追加栈条目。
	const bool bAlreadyRegistered = (FindWindow(WindowName) == WindowInstance);

	// 生命周期回调派发期间发起的“去重弹出”可能还在队列里：本次 Push 已表达“重新显示”的意图，
	// 丢弃同名的待执行弹出/移除，避免出口再把刚激活的窗口关掉。
	PendingPops.RemoveAll([WindowName](const FPendingPopRequest& Request)
	{
		return Request.WindowName == WindowName;
	});
	PendingRemoves.RemoveAll([WindowInstance](const TWeakObjectPtr<UCORWindow>& WeakWindow)
	{
		return WeakWindow.Get() == WindowInstance;
	});

	UCanvasPanelSlot* CanvasSlot = nullptr;
	if (bAlreadyRegistered)
	{
		// 已经在本容器中：复用现有 Slot，只调整 ZOrder
		CanvasSlot = Cast<UCanvasPanelSlot>(WindowInstance->GetOwningSlot());
	}
	else
	{
		// 同名的旧实例（理论上管理器已先弹出）：清掉账本里的残留条目，避免双条目
		if (UCORWindow* StaleWindow = FindWindow(WindowName))
		{
			WindowsStack.Remove(StaleWindow);
			Windows.Remove(WindowName);
		}
		// 将UI添加到层级
		CanvasSlot = AddChildToCanvas(WindowInstance);
	}

	if (!IsValid(CanvasSlot))
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORCanvasContainer::PushUI >> Slot is not valid"))
		return NAME_None;
	}

	// 保存UI实例的弱指针
	Windows.Add(WindowName, WindowInstance);
	// 添加UI到逻辑栈：先移除旧条目，保证同一激活在栈内唯一，且移动到栈顶
	WindowsStack.Remove(WindowInstance);
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
	// 生命周期回调派发期间修改栈会破坏遍历：入队，在派发出口统一执行
	if (DispatchDepth > 0)
	{
		FPendingPopRequest Request;
		Request.WindowName = WindowName;
		Request.bPopSelf = bPopSelf;
		PendingPops.Add(Request);
		bStateUpdateRequested = true;
		return;
	}

	// 待移除窗口的快照：锁内填充，锁外统一归还对象池
	TArray<UCORWindow*> WindowsToRemove;

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
					WindowsToRemove.Add(TopWindow.Get());
				}
			}
		}
		else
		{
			// 从缓存的弱指针Map找到对应Window
			UCORWindow* Window = FindWindow(WindowName);
			if (Window)
			{
				// 从栈中查找到目标UI索引，如果不存在，则不做任何处理
				const int32 WindowIndex = WindowsStack.Find(Window);
				if (WindowIndex >= 0)
				{
					// 检查Window类型，如果是FullScreen，则进入下一步判定，否则直接移除
					if (Cast<UCORFullScreen>(Window) != nullptr)
					{
						// 契约（见头文件）：bPopSelf=true 移除目标及上方；false 只移除上方，保留目标
						const int32 TargetIndex = bPopSelf ? WindowIndex : WindowIndex + 1;

						// 倒序快照待移除的UI
						for (int32 i = WindowsStack.Num() - 1; i >= TargetIndex; i--)
						{
							TWeakObjectPtr<UCORWindow> RemoveWindow = WindowsStack[i];
							if (RemoveWindow.IsValid())
							{
								WindowsToRemove.Add(RemoveWindow.Get());
							}
						}
					}
					// 如果不是FullScreen，那么仅需移除自身
					else
					{
						WindowsToRemove.Add(Window);
					}
				}
			}
		}

		// 锁内只做剥离，避免边遍历 WindowsStack 边移除导致索引错位
		for (UCORWindow* WindowToRemove : WindowsToRemove)
		{
			DetachWindow(WindowToRemove);
		}
	}

	// 锁外统一归还对象池，销毁缓存的SObjectWidget并触发 NativeDestruct
	ReleaseWindows(WindowsToRemove);

	if (WindowsToRemove.Num() > 0)
	{
		// 移除窗口后，底层窗口可能需要恢复显示，这里刷新一次窗口状态（保持原有语义）
		UpdateWindowState(EWindowState::None);
	}

	if (WindowName != NAME_None)
	{
		// 刷新界面状态（原有实现在 NAME_None 分支移除后直接 return）
		UpdateUI();
	}
}

void UCORCanvasContainer::NativeRemoveWindow(UCORWindow* WindowInstance)
{
	if (!IsValid(WindowInstance))
	{
		return;
	}

	// 生命周期回调派发期间：入队，派发出口统一执行
	if (DispatchDepth > 0)
	{
		PendingRemoves.Add(WindowInstance);
		bStateUpdateRequested = true;
		return;
	}

	bool bDetached = false;
	{
		FScopeLock Lock(&ContainerLock);

		// 先确认窗口属于当前容器，避免把其它层/已移除的窗口误归还对象池
		bDetached = WindowsStack.Contains(WindowInstance) ||
			Windows.Contains(WindowInstance->GetWindowName());

		if (bDetached)
		{
			// 锁内只做剥离，不销毁底层Slate
			DetachWindow(WindowInstance);
		}
	}

	if (!bDetached)
	{
		return;
	}

	// 锁作用域结束后再归还对象池：Release(..., true) 会销毁缓存的 SObjectWidget，
	// 从而触发 SObjectWidget::ResetWidget -> NativeDestruct -> JS Destruct()
	TArray<UCORWindow*> WindowsToRelease;
	WindowsToRelease.Add(WindowInstance);
	ReleaseWindows(WindowsToRelease);

	UpdateWindowState(EWindowState::None);
}

void UCORCanvasContainer::DetachWindow(UCORWindow* WindowInstance)
{
	// 必须在 ContainerLock 内调用
	if (!IsValid(WindowInstance))
	{
		return;
	}

	// 已经被移除过（例如 NativeClose 中触发的递归 PopUI 已处理），避免重复触发关闭事件
	const bool bInStack = WindowsStack.Contains(WindowInstance);
	const bool bInWindows = Windows.Contains(WindowInstance->GetWindowName());
	if (!bInStack && !bInWindows)
	{
		return;
	}

	// 先完成账本与 Slate 结构脱钩，再对外发关闭通知：
	// 关闭回调里同步 Push/Pop 时看到的是“已移除”的一致状态，不会重复释放。
	WindowsStack.Remove(WindowInstance);
	Windows.Remove(WindowInstance->GetWindowName());

	WindowInstance->RemoveFromParent();

	// 结构脱钩完成后再触发隐藏/关闭事件（回调可能重入 PopUI，DetachWindow 会因账本已清空而早退）
	WindowInstance->Hide();
	WindowInstance->NativeClose();

	// 释放窗口自身持有的Slate资源，配合对象池释放 SObjectWidget
	WindowInstance->ReleaseSlateResources(true);
}

void UCORCanvasContainer::ReleaseWindows(const TArray<UCORWindow*>& WindowsToRelease)
{
	if (WindowsToRelease.Num() == 0)
	{
		return;
	}

	if (!WidgetPool)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("[CORUI] UCORCanvasContainer::ReleaseWindows >> WidgetPool is not set, %d window(s) can not be released"),
		       WindowsToRelease.Num())
		return;
	}

	// 去重，避免同一个窗口被重复归还；同时保持传入顺序（栈顶优先）
	TSet<UCORWindow*> UniqueWindows;
	TArray<UCORWindow*> OrderedWindows;
	OrderedWindows.Reserve(WindowsToRelease.Num());
	for (UCORWindow* Window : WindowsToRelease)
	{
		if (IsValid(Window) && !UniqueWindows.Contains(Window))
		{
			UniqueWindows.Add(Window);
			OrderedWindows.Add(Window);
		}
	}

	for (UCORWindow* Window : OrderedWindows)
	{
		UE_LOG(LogTemp, Log, TEXT("[CORUI][Release] %s"), *Window->GetName())
		// 必须在 RemoveFromParent / ReleaseSlateResources 之后调用，
		// 只有移出 ActiveWidgets 并删除缓存Slate，SObjectWidget 才会析构
		WidgetPool->Release(Window, true);
	}
}

void UCORCanvasContainer::ClearAllWindows()
{
	// 待移除窗口的快照：锁内填充，锁外统一归还对象池
	TArray<UCORWindow*> WindowsToRemove;

	{
		FScopeLock Lock(&ContainerLock);

		// 倒序快照全部有效窗口
		for (int32 i = WindowsStack.Num() - 1; i >= 0; i--)
		{
			TWeakObjectPtr<UCORWindow> Window = WindowsStack[i];
			if (Window.IsValid())
			{
				WindowsToRemove.Add(Window.Get());
			}
		}

		// 兜底：清理不在栈中的残留窗口
		for (const TPair<FName, TWeakObjectPtr<UCORWindow>>& Pair : Windows)
		{
			UCORWindow* Window = Pair.Value.Get();
			if (IsValid(Window) && !WindowsToRemove.Contains(Window))
			{
				WindowsToRemove.Add(Window);
			}
		}

		for (UCORWindow* WindowToRemove : WindowsToRemove)
		{
			DetachWindow(WindowToRemove);
		}

		Windows.Empty();
		WindowsStack.Empty();
		ChildMaxZOrder = 0;
		// 清空待执行队列：容器已被整体清空，旧请求不再有意义
		PendingPops.Empty();
		PendingRemoves.Empty();
		bStateUpdateRequested = false;
	}

	// 锁外统一归还对象池
	ReleaseWindows(WindowsToRemove);
}

uint32 UCORCanvasContainer::UpdateWindowState(uint32 WindowState)
{
	// 派发中再次请求刷新：合并到当前派发出口，避免嵌套遍历
	if (DispatchDepth > 0)
	{
		bStateUpdateRequested = true;
		return WindowState;
	}

	// 遍历稳定快照：回调可能关闭/替换窗口，实时数组会失效
	TArray<TWeakObjectPtr<UCORWindow>> WindowSnapshot;
	{
		FScopeLock Lock(&ContainerLock);

		// 如果LayerPanel为隐藏，则直接返回
		if (GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden)
		{
			return WindowState;
		}

		WindowSnapshot = WindowsStack;
	}

	++DispatchDepth;

	// 从栈顶窗口向下检查状态
	for (int32 i = WindowSnapshot.Num() - 1; i >= 0; i--)
	{
		UCORWindow* CheckWindow = WindowSnapshot[i].Get();
		if (!IsValid(CheckWindow))
		{
			continue;
		}

		// 回调可能已经移除该窗口：每次调用前重新校验成员资格
		if (!IsWindowRegistered(CheckWindow))
		{
			continue;
		}

		WindowState = CheckWindow->NativeUpdateState(WindowState);
	}

	--DispatchDepth;

	// 派发出口：执行回调期间累积的弹出/移除请求，并返回重算后的状态
	return DrainPendingMutations(WindowState);
}

bool UCORCanvasContainer::IsWindowRegistered(const UCORWindow* WindowInstance) const
{
	if (!IsValid(WindowInstance))
	{
		return false;
	}

	FScopeLock Lock(&ContainerLock);

	if (WindowsStack.Contains(WindowInstance))
	{
		return true;
	}

	const TWeakObjectPtr<UCORWindow>* Found = Windows.Find(WindowInstance->GetWindowName());
	return Found && Found->Get() == WindowInstance;
}

uint32 UCORCanvasContainer::DrainPendingMutations(uint32 CurrentState)
{
	if (DispatchDepth > 0)
	{
		return CurrentState;
	}

	bool bExecutedMutation = false;
	const int32 MaxDrainRounds = 16;
	for (int32 Round = 0; Round < MaxDrainRounds; ++Round)
	{
		if (PendingPops.Num() == 0 && PendingRemoves.Num() == 0)
		{
			break;
		}

		bExecutedMutation = true;

		TArray<FPendingPopRequest> PopsToRun;
		Swap(PopsToRun, PendingPops);

		TArray<TWeakObjectPtr<UCORWindow>> RemovesToRun;
		Swap(RemovesToRun, PendingRemoves);

		for (const FPendingPopRequest& Request : PopsToRun)
		{
			PopUI(Request.WindowName, Request.bPopSelf);
		}

		for (const TWeakObjectPtr<UCORWindow>& WeakWindow : RemovesToRun)
		{
			if (UCORWindow* Window = WeakWindow.Get())
			{
				NativeRemoveWindow(Window);
			}
		}
	}

	// 栈已经变化：返回值必须是按新栈重算后的状态，而不是旧快照的结果
	if (bStateUpdateRequested || bExecutedMutation)
	{
		bStateUpdateRequested = false;
		return UpdateWindowState(EWindowState::None);
	}

	return CurrentState;
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
