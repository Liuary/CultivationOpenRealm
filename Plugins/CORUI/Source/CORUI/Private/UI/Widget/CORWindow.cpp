// Fill out your copyright notice in the Description page of Project Settings.


#include "CORUI/Public/UI/Widget/CORWindow.h"

#include "CORUIManager.h"

void UCORWindow::NativeOpen()
{
	WindowState = EWindowState::None;
	// 这一次设置可见性不触发事件，等到后面更新UI状态时触发界面显示的事件
	SetVisibilityForEvent(ESlateVisibility::Collapsed, false);
	
	Super::NativeOpen();
}

void UCORWindow::NativeClose()
{
	Super::NativeClose();
}

void UCORWindow::NativeCover()
{
	OnCover();
}

uint32 UCORWindow::NativeUpdateState(uint32 NewState)
{

	// 如果传入的状态是空白状态，并且自身状态并非空白，说明自身更高层的 UI 被移除，因此要移除自身的 Covered 和 Hidden 标记
	if (NewState == EWindowState::None && WindowState != EWindowState::None)
	{
		// 移除隐藏标记
		WindowState = WindowState & (~EWindowState::Hidden);
		// 移除被覆盖状态
		WindowState = WindowState & (~EWindowState::Covered);
	}

	// 如果顶层的UI要求底部UI隐藏，则检查隐藏
	if (NewState & EWindowState::HiddenUnder)
	{
		// 假设没有触发过隐藏，则触发一次隐藏方法
		if (!(WindowState & EWindowState::Hidden) && IsVisible())
		{
			Hide();
		}
		// 标记隐藏底部UI
		WindowState = (WindowState | EWindowState::Hidden);
		// 移除界面显示标记
		WindowState = WindowState & (~EWindowState::Shown);
	}
	else
	{
		// 根据自身是否要隐藏底部UI，更新State
		if (UnderWindowHide)
		{
			// 标记隐藏底部UI
			WindowState = WindowState | EWindowState::HiddenUnder;
		}
		else
		{
			// 标记不隐藏底部UI
			WindowState = WindowState & (~EWindowState::HiddenUnder);
		}
	}

	// 如果顶层UI已经显示或者被覆盖，那么触发当前UI的被覆盖状态
	if (NewState & EWindowState::Shown || NewState & EWindowState::Covered)
	{
		// 假如没有触发过被覆盖，触发一次被覆盖方法
		if (!(WindowState & EWindowState::Covered))
		{
			NativeCover();
		}
		// 将这个UI标记设置为被覆盖
		WindowState = WindowState | EWindowState::Covered;
		// 移除界面显示标记
		WindowState = WindowState & (~EWindowState::Shown);
	}
	else
	{
		// 不存在覆盖情况，将标记设置为未被覆盖
		if (!(WindowState & EWindowState::Hidden))
		{
			WindowState = WindowState & (~EWindowState::Covered);
		}
	}

	// 当前界面既不隐藏，也不被覆盖，且传入的标记中不显示，则触发当前界面的显示
	if (!(NewState & EWindowState::Shown) &&
		!(WindowState & EWindowState::Covered) && !(WindowState & EWindowState::Hidden))
	{
		if (!(WindowState & EWindowState::Shown) && !IsVisible())
		{
			Show();
		}

		// 标记显示UI
		WindowState = WindowState | EWindowState::Shown;
	}
	else
	{
		// 标记不显示UI
		WindowState = WindowState & (~EWindowState::Shown);
		// // 移除隐藏和被覆盖标记
		// WindowState = WindowState & (~EWindowState::Hidden);
		// WindowState = WindowState & (~EWindowState::Covered);
	}
	
	return WindowState;
}

void UCORWindow::NativeUpdateName()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	
	// 设置WindowName为这个对象的名字（即资产实例的名字）
	if (WindowName.IsNone())
	{
		FString PackgeName = GetPackage()->GetName();
		FString ShortName = FPackageName::GetShortName(PackgeName);
		if (ShortName.Len() > 0)
		{
			FString AssetName = ShortName + "_C";
			WindowName = FName(AssetName);
		}
	}
}

void UCORWindow::NativeCloseUI(bool bCloseSelf)
{
	UGameInstance* Instance = GetGameInstance();
	if (!IsValid(Instance))
	{
		return;
	}
	UCORUIManager* Manager = Instance->GetSubsystem<UCORUIManager>();
	if (!Manager)
	{
		return;
	}
	Manager->PopUI(this->GetWindowName(), this->GetLayer(), bCloseSelf);
}

UCORWindow::UCORWindow()
{
	// 默认设置可聚焦
	SetIsFocusable(true);
}

void UCORWindow::CloseUI(bool bCloseSelf)
{
	NativeCloseUI(bCloseSelf);
}

void UCORWindow::PostLoad()
{
	Super::PostLoad();
	
	NativeUpdateName();
}

void UCORWindow::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	NativeUpdateName();

	Super::PreSave(ObjectSaveContext);
}

void UCORWindow::OnCover_Implementation()
{
}