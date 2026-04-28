#include "PmgDesignerExtension.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Runtime/UMG/Public/Blueprint/WidgetTree.h"

FPmgDesignerExtension::FPmgDesignerExtension()
{
	UE_LOG(LogTemp, Log, TEXT("FPmgDesignerExtension::FPmgDesignerExtension"));
	
	// Tick
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FPmgDesignerExtension::HandleTick),
		bTickDelay
	);

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	
	if (!AssetEditorSubsystem)
	{
		return;
	}

	// 监听蓝图关闭事件
	AssetEditorSubsystem->OnAssetClosedInEditor().AddRaw(this, &FPmgDesignerExtension::OnAssetClosed);

	// 监听编辑器关闭事件
	GEditor->OnEditorClose().AddRaw(this, &FPmgDesignerExtension::OnEditorClose);
}

FPmgDesignerExtension::~FPmgDesignerExtension()
{
	UE_LOG(LogTemp, Log, TEXT("FPmgDesignerExtension::~FPmgDesignerExtension"));

	// RemoveAssetEditorSubsystemEvent();

	VBoxs.Empty();
	
	this->bIsDestroyed = true;
	
	// 移除Tick
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();		
	}

}

void FPmgDesignerExtension::Initialize(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint)
{
	FDesignerExtension::Initialize(InDesigner, InBlueprint);
	
	if (!GEditor || !GEditor->IsValidLowLevel())
	{
		return;
	}

	UWorld* const World = GWorld;
	if (World == nullptr)
	{
		return;
	}
	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.SetTimerForNextTick([this, InDesigner, InBlueprint]()
	{
		InitVBox(InDesigner, InBlueprint);
	});
}

void FPmgDesignerExtension::Uninitialize()
{
	
	bIsDestroyed = true;
	
	VBoxs.Empty();
	
	FDesignerExtension::Uninitialize();
}

void FPmgDesignerExtension::OnAssetClosed(UObject* Object, IAssetEditorInstance* AssetEditorInstance)
{
	if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Object))
	{
		if (IsValid(WidgetBlueprint) && VBoxs.Find(WidgetBlueprint))
		{
			VBoxs.Remove(WidgetBlueprint);
		}
	}
}

void FPmgDesignerExtension::OnEditorClose()
{
	RemoveAssetEditorSubsystemEvent();
	
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();		
	}
	
	VBoxs.Empty();
	this->bIsDestroyed = true;
}

void FPmgDesignerExtension::RemoveAssetEditorSubsystemEvent()
{
	if (!GEditor || !GEditor->IsValidLowLevel())
	{
		return;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	
	if (!AssetEditorSubsystem || !AssetEditorSubsystem->IsValidLowLevel())
	{
		return;
	}

	// 移除蓝图关闭事件监听
	AssetEditorSubsystem->OnAssetClosedInEditor().RemoveAll(this);
	GEditor->OnEditorClose().RemoveAll(this);
}

void FPmgDesignerExtension::InitVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint)
{
	TSharedPtr<SVerticalBox> NewVBox = GetOrCreateVBox(InDesigner, InBlueprint);
	if (IsValid(InBlueprint) && NewVBox.IsValid())
	{
		VBoxs.Add(InBlueprint, NewVBox);
	}
}

TSharedPtr<SVerticalBox>  FPmgDesignerExtension::GetOrCreateVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint)
{
	SDesignerView* DesignerView = static_cast<SDesignerView*>(InDesigner);
	if (!DesignerView)
	{
		return nullptr;
	}
	// 查找 DesignerView 内部的 SOverlay
	TSharedPtr<SOverlay> TargetOverlay = nullptr;
	FChildren* Children = DesignerView->GetChildren();
	TArray<SWidget*> WidgetQueue;
	for (int32 i = 0; i < Children->Num(); ++i)
	{
		const TSharedRef<SWidget> Child = Children->GetChildAt(i);
		WidgetQueue.Add(&Child.Get());
	}
	while (WidgetQueue.Num() > 0)
	{
		SWidget* CurrentWidget = WidgetQueue[0];
		WidgetQueue.RemoveAt(0);
		if (CurrentWidget->GetTypeAsString() == TEXT("SOverlay"))
		{
			TargetOverlay = StaticCastSharedRef<SOverlay>(CurrentWidget->AsShared());
			break;
		}
		// 将子控件添加到队列中以进行进一步搜索
		FChildren* CurrentChildren = CurrentWidget->GetChildren();
		for (int32 i = 0; i < CurrentChildren->Num(); ++i)
		{
			const TSharedRef<SWidget> Child = CurrentChildren->GetChildAt(i);
			WidgetQueue.Add(&Child.Get());
		}
	}
	if (TargetOverlay.IsValid())
	{
		TSharedPtr<SVerticalBox> NewVerticalBox = nullptr;
		// 查找 TargetOverlay 内部的 SVerticalBox
		FChildren* TargetChildren = TargetOverlay->GetChildren();
		for (int32 i = 0; i < TargetChildren->Num(); ++i)
		{
			const TSharedPtr<SWidget> Child = TargetChildren->GetChildAt(i);
			// 检查是不是这个类生成的目标VBox
			if (Child->GetTag() == VBoxName)
			{
				NewVerticalBox = StaticCastSharedPtr<SVerticalBox>(Child);
				break;
			}
			
		}
		// 创建一个新的 垂直框，用以保存弱指针到这个对象，以便操作，这个NewVerticalBox向下偏移36像素
		if (!NewVerticalBox.IsValid())
		{
			NewVerticalBox = SNew(SVerticalBox).Tag(VBoxName);
			TargetOverlay->AddSlot().VAlign(VAlign_Top).Padding(FMargin(12, 36, 0, 0))[NewVerticalBox.ToSharedRef()];
		}
		return NewVerticalBox;
	}
	return nullptr;
}

void FPmgDesignerExtension::ReloadVBoxList(TWeakObjectPtr<UWidgetBlueprint> InBlueprint, TWeakPtr<SVerticalBox> InVBox, TArray<FString> InList)
{
	
	TSharedPtr<SVerticalBox> VBoxPtr = InVBox.Pin();
	
	if (!VBoxPtr.IsValid() || !InBlueprint.IsValid())
	{
		return;
	}

	// 检查 VBox，清除所有非 STextBlock 控件
	FChildren* Children = InVBox.Pin()->GetChildren();
	int32 ReloadCount = FMath::Max(InList.Num(), Children->Num());
	for (int32 i = 0; i < ReloadCount; ++i)
	{
		if (i >= Children->Num())
		{
			InVBox.Pin()->AddSlot()[SNew(STextBlock)];
		}

		TSharedPtr<STextBlock> TextBlock = StaticCastSharedRef<STextBlock>(Children->GetChildAt(i));
		if (i < InList.Num())
		{
			TextBlock->SetText(FText::FromString(InList[i]));
			TextBlock->SetVisibility(EVisibility::SelfHitTestInvisible);
		}
		else
		{
			TextBlock->SetVisibility(EVisibility::Collapsed);
		}
	}
}

void FPmgDesignerExtension::UpdateShowData()
{
	
	TArray<TWeakObjectPtr<UWidgetBlueprint>> InvalidKeys;

	for (auto Iter : VBoxs)
	{
		
		if (!Iter.Key.IsValid() || !Iter.Value.IsValid())
		{
			InvalidKeys.Add(Iter.Key);
			continue;
		}
		
		TArray<FString> ShowData;
		ShowData.Add(FString::Printf(TEXT("Current Widget:\t %s"), *GetBlueprintName(Iter.Key)));
		ShowData.Add(FString::Printf(TEXT("Widget DrawCalls:\t %d"), CalculateWidgets(Iter.Key)));

		ReloadVBoxList(Iter.Key, Iter.Value , ShowData);
	}
	
	for (auto Iter : InvalidKeys)
	{
		VBoxs.Remove(Iter);
	}
}

void FPmgDesignerExtension::Cleanup()
{
	RemoveAssetEditorSubsystemEvent();
	VBoxs.Empty();
}

bool FPmgDesignerExtension::HandleTick(float DeltaTime)
{
	if (this->bIsDestroyed)
	{
		return false;
	}
	
	if (!GEditor || !GEditor->IsValidLowLevel())
	{
		return false;
	}
	
	UpdateShowData();
	return true;
}

FString FPmgDesignerExtension::GetBlueprintName(TWeakObjectPtr<UWidgetBlueprint> InBlueprint)
{
	if (!InBlueprint.IsValid())
	{
		return FString();
	}
	return InBlueprint.Get()->GetName();
}

int32 FPmgDesignerExtension::CalculateWidgets(TWeakObjectPtr<UWidgetBlueprint> InBlueprint)
{
	if (!InBlueprint.IsValid())
	{
		return 0;
	}

	UWidgetBlueprint* BlueprintPtr = InBlueprint.Get();
	// 取得蓝图中的控件树
	UWidgetTree* WidgetTree = BlueprintPtr->WidgetTree;
	if (!WidgetTree)
	{
		return 0;
	}

	int32 DcCount = 0;
	FDrawCallState InitialState;
	
	CalculateDrawCallRecursive(WidgetTree->RootWidget, InitialState, DcCount);
	
	return FMath::Max(1, DcCount);
}

void FPmgDesignerExtension::CalculateDrawCallRecursive(UWidget* Widget, FDrawCallState& State, int32& OutDrawCalls)
{
	if (!Widget)
	{
		return;
	}
	
	// 如果控件被隐藏，就不会产生DrawCall
	if (!Widget->IsVisible())
	{
		return;
	}
	
	// 如果控件启用了裁剪，那么会打断合批，产生DrawCall
	EWidgetClipping WidgetClipping = Widget->GetClipping();
	if (WidgetClipping != EWidgetClipping::Inherit && WidgetClipping != State.CurrentClipping)
	{
		State.CurrentClipping = WidgetClipping;
		State.CurrentResource = nullptr;
		State.CurrentSimulatedLayer++;
	}
	
	// 根据资源状态预估DrawCall
	const UObject* WidgetResource = nullptr;
	bool bIsRenderable = false;
	
	if (UImage* ImageWidget = Cast<UImage>(Widget))
	{
		WidgetResource = GetBrushResource(ImageWidget->Brush);
		bIsRenderable = true;
	}
	else if (UBorder* BorderWidget = Cast<UBorder>(Widget))
	{
		WidgetResource = GetBrushResource(BorderWidget->Background);
		bIsRenderable = true;
	}
	else if (UTextBlock* TextWidget = Cast<UTextBlock>(Widget))
	{
		// 文本按照字体合批，不过这里是预估，按照字体对象计算
		WidgetResource = TextWidget->GetFont().FontObject;
		bIsRenderable = true;
	}
	
	if (bIsRenderable)
	{
		
		bool bShouldBreakBatch = false;
		// 第一个绘制的控件
		if (!State.bHasStartDrawing)
		{
			bShouldBreakBatch = true;
			State.bHasStartDrawing = true;
		}
		// 资源不一致，需要打断合批
		else if (WidgetResource != State.CurrentResource)
		{
			bShouldBreakBatch = true;
		}
		// 层不一致，需要打断合批
		else if (State.CurrentSimulatedLayer != State.LastRenderedLayer)
		{
			bShouldBreakBatch = true;
		}
		
		if (bShouldBreakBatch)
		{
			++OutDrawCalls;
			State.CurrentResource = WidgetResource;
			State.LastRenderedLayer = State.CurrentSimulatedLayer;
		}
	}
	
	// 递归处理子控件
	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
	{
		TArray<UWidget*> Children;
		for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
		{
			if (UWidget* Child = PanelWidget->GetChildAt(i))
			{
				Children.Add(Child);
			}
		}
		
		// 如果是 CanvasPanel ，受 ZOrder 影响，必须排序
		if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(PanelWidget))
		{
			Children.Sort([](UWidget& L, UWidget& R)
			{
				UCanvasPanelSlot* LSlot = Cast<UCanvasPanelSlot>(L.Slot);
				UCanvasPanelSlot* RSlot = Cast<UCanvasPanelSlot>(R.Slot);
				int32 ZOrderL = LSlot ? LSlot->GetZOrder() : 0;
				int32 ZOrderR = RSlot ? RSlot->GetZOrder() : 0;
				return ZOrderL < ZOrderR;
			});
		}
		
		for (UWidget* Child : Children)
		{
			CalculateDrawCallRecursive(Child, State, OutDrawCalls);
		}
	}
}

UObject* FPmgDesignerExtension::GetBrushResource(const FSlateBrush& Brush)
{
	// 贴图或材质
	if (UObject* Resource = Brush.GetResourceObject())
	{
		return Resource;
	}
	return nullptr;
}
