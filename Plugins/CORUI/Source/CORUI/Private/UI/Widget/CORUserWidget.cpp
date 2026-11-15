// Fill out your copyright notice in the Description page of Project Settings.


#include "CORUI/Public/UI/Widget/CORUserWidget.h"

#include "Blueprint/WidgetTree.h"

void UCORUserWidget::Show()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCORUserWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCORUserWidget::NativeDestruct()
{
	UE_LOG(LogTemp, Log, TEXT("[CORUI][Destruct] %s"), *GetName())

	// 析构时Slate可能已经被释放，直接触发隐藏事件，避免对空的Slate设置可见性
	if (bIsShowing)
	{
		NativeHide();
	}

	Super::NativeDestruct();
}

void UCORUserWidget::SetVisibility(ESlateVisibility InVisibility)
{
	SetVisibilityForEvent(InVisibility);
}

void UCORUserWidget::SetVisibilityForEvent(ESlateVisibility InVisibility, bool TriggerEvent)
{
	// 判断旧的可见状态
	bool bIsVisible = IsVisible();
	Super::SetVisibility(InVisibility);
	switch (InVisibility)
	{
	case ESlateVisibility::Hidden:
	case ESlateVisibility::Collapsed:
		if (bIsVisible && TriggerEvent)
		{
			// 原本可见，现在不可见，触发Hide
			NativeHide();
		}
		break;
	default:
		if (!bIsVisible && TriggerEvent)
		{
			// 原本不可见，现在可见，触发Show
			NativeShow();
		}
		break;
	}
}

bool UCORUserWidget::IsVisible() const
{
	ESlateVisibility CurrentVisibility = GetVisibility();
	return !(CurrentVisibility == ESlateVisibility::Hidden || CurrentVisibility == ESlateVisibility::Collapsed) && Super::IsVisible();
}

void UCORUserWidget::NativeOpen()
{
	OnOpen();
	
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget) {
			if (UCORUserWidget* UserWidget = Cast<UCORUserWidget>(Widget))
			{
				UserWidget->NativeOpen();
			}
		});
	}
}

void UCORUserWidget::NativeClose()
{
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget) {
			if (UCORUserWidget* UserWidget = Cast<UCORUserWidget>(Widget))
			{
				UserWidget->NativeClose();
			}
		});
	}
	
	OnClose();
}

void UCORUserWidget::OnShow_Implementation()
{
}

void UCORUserWidget::OnHide_Implementation()
{
}

void UCORUserWidget::OnOpen_Implementation()
{
}

void UCORUserWidget::OnClose_Implementation()
{
}

void UCORUserWidget::NativeShow()
{
	if (!bIsShowing)
	{
		OnShow();
		bIsShowing = true;
	}
}

void UCORUserWidget::NativeHide()
{
	if (bIsShowing)
	{
		OnHide();
		bIsShowing = false;
	}
}
