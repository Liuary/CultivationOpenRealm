#include "PmgDesignerExtension.h"

#include "Blueprint/UserWidget.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/InvalidationBox.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/RetainerBox.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Fonts/SlateFontInfo.h"
#include "HAL/IConsoleManager.h"
#include "Runtime/UMG/Public/Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace
{
	// 设计器信息面板总开关：大 UI 多开几个设计器时可以先关掉，避免叠加开销
	static TAutoConsoleVariable<bool> CVarDrawCallOverlay(
		TEXT("CORUI.DrawCallOverlay"),
		true,
		TEXT("在 UMG 设计器左上角显示 CORUI 的 DrawCall 预估面板（编辑器）。"),
		ECVF_Default);
}

FPmgDesignerExtension::FPmgDesignerExtension()
{
	UE_LOG(LogTemp, Log, TEXT("FPmgDesignerExtension::FPmgDesignerExtension"));
	
	// Tick
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FPmgDesignerExtension::HandleTick),
		TickIntervalSeconds
	);
}

FPmgDesignerExtension::~FPmgDesignerExtension()
{
	UE_LOG(LogTemp, Log, TEXT("FPmgDesignerExtension::~FPmgDesignerExtension"));

	// 必须先注销委托，避免资产编辑器关闭时回调到已释放的 this
	RemoveAssetEditorSubsystemEvent();

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

	UE_LOG(LogTemp, Log, TEXT("[PmgDesigner] Initialize >> bp=%s designer=%s"),
	       InBlueprint ? *InBlueprint->GetName() : TEXT("null"), InDesigner ? TEXT("ok") : TEXT("null"));
	
	if (!GEditor || !GEditor->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PmgDesigner] Initialize abort >> no GEditor"));
		return;
	}

	UWorld* const World = GWorld;
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PmgDesigner] Initialize abort >> no GWorld"));
		return;
	}

	// 委托与 extension 生命周期绑定：Initialize 注册，Uninitialize/析构注销
	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditorSubsystem->OnAssetClosedInEditor().RemoveAll(this);
		AssetEditorSubsystem->OnAssetClosedInEditor().AddRaw(this, &FPmgDesignerExtension::OnAssetClosed);
	}
	GEditor->OnEditorClose().RemoveAll(this);
	GEditor->OnEditorClose().AddRaw(this, &FPmgDesignerExtension::OnEditorClose);

	// designer 在 Initialize 时控件树往往还没搭好；而 World 的 next-tick timer 在这条路径上
	// 实测不触发（Initialize 后回调从未执行，面板永远不出现）。改为记入待初始化队列，
	// 由已有的 ticker 在后续 tick 里重试——designer 一就绪就挂上，且天然不持有裸 this。
	if (InDesigner && IsValid(InBlueprint))
	{
		PendingInit.Add(InBlueprint, StaticCastSharedRef<SDesignerView>(static_cast<SDesignerView*>(InDesigner)->AsShared()));
		UE_LOG(LogTemp, Log, TEXT("[PmgDesigner] pending init queued >> bp=%s"), *InBlueprint->GetName());
	}
}

void FPmgDesignerExtension::Uninitialize()
{
	// 先注销，防止 designer 卸载后仍收到 OnAssetClosed
	RemoveAssetEditorSubsystemEvent();

	bIsDestroyed = true;
	
	VBoxs.Empty();
	DesignerViews.Empty();
	LastRenderedSignature.Empty();
	PendingInit.Empty();
	
	FDesignerExtension::Uninitialize();
}

void FPmgDesignerExtension::OnAssetClosed(UObject* Object, IAssetEditorInstance* AssetEditorInstance)
{
	if (bIsDestroyed)
	{
		return;
	}

	if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Object))
	{
		const TWeakObjectPtr<UWidgetBlueprint> Key(WidgetBlueprint);
		VBoxs.Remove(Key);
		// 三个表必须同步清：残留签名叫"同实例重新 Initialize"时首帧被误判为无变化
		DesignerViews.Remove(Key);
		LastRenderedSignature.Remove(Key);
		PendingInit.Remove(Key);
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
	DesignerViews.Empty();
	LastRenderedSignature.Empty();
	PendingInit.Empty();
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
	// 记住 designer：designer 在资产加载/切换预览模式时会重建子控件，把我们的 slot 一起冲掉，
	// 所以刷新时要能重新挂一次（否则面板会"注册成功但永远不显示"）。
	if (SDesignerView* DesignerView = static_cast<SDesignerView*>(InDesigner))
	{
		DesignerViews.Add(InBlueprint, StaticCastSharedRef<SDesignerView>(DesignerView->AsShared()));
	}

	TSharedPtr<SVerticalBox> NewVBox = GetOrCreateVBox(InDesigner, InBlueprint);
	if (IsValid(InBlueprint) && NewVBox.IsValid())
	{
		VBoxs.Add(InBlueprint, NewVBox);
	}

	UE_LOG(LogTemp, Log, TEXT("[PmgDesigner] InitVBox >> bp=%s vbox=%s registered=%d"),
	       *GetBlueprintName(InBlueprint), NewVBox.IsValid() ? TEXT("ok") : TEXT("null"), VBoxs.Num());
}

TSharedPtr<SVerticalBox>  FPmgDesignerExtension::GetOrCreateVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint)
{
	SDesignerView* DesignerView = static_cast<SDesignerView*>(InDesigner);
	if (!DesignerView)
	{
		return nullptr;
	}
	// 查找 DesignerView 内部的 SOverlay
	// 注意：BFS 找到的"第一个" SOverlay 不一定可见（预览用的 hit-test overlay 在非预览态是隐藏的），
	// 挂到不可见锚点上会导致面板永远不显示，所以收集全部候选后优先选可见的那个。
	TArray<TSharedPtr<SOverlay>> OverlayCandidates;
	FChildren* Children = DesignerView->GetChildren();
	TArray<SWidget*> WidgetQueue;
	for (int32 i = 0; i < Children->Num(); ++i)
	{
		const TSharedRef<SWidget> Child = Children->GetChildAt(i);
		WidgetQueue.Add(&Child.Get());
	}
	int32 VisitedCount = 0;
	while (WidgetQueue.Num() > 0 && VisitedCount < 4096)
	{
		SWidget* CurrentWidget = WidgetQueue[0];
		WidgetQueue.RemoveAt(0);
		++VisitedCount;
		if (CurrentWidget->GetTypeAsString() == TEXT("SOverlay"))
		{
			OverlayCandidates.Add(StaticCastSharedRef<SOverlay>(CurrentWidget->AsShared()));
		}
		// 将子控件添加到队列中以进行进一步搜索
		FChildren* CurrentChildren = CurrentWidget->GetChildren();
		for (int32 i = 0; i < CurrentChildren->Num(); ++i)
		{
			const TSharedRef<SWidget> Child = CurrentChildren->GetChildAt(i);
			WidgetQueue.Add(&Child.Get());
		}
	}

	TSharedPtr<SOverlay> TargetOverlay = nullptr;
	for (const TSharedPtr<SOverlay>& Candidate : OverlayCandidates)
	{
		if (!Candidate.IsValid())
		{
			continue;
		}
		const bool bCandidateVisible = Candidate->GetVisibility().IsVisible();
		UE_LOG(LogTemp, Verbose, TEXT("[PmgDesigner] overlay candidate: visible=%d children=%d"), bCandidateVisible ? 1 : 0,
		       Candidate->GetChildren()->Num());
		if (!TargetOverlay.IsValid() && bCandidateVisible)
		{
			TargetOverlay = Candidate;
		}
	}

	if (!TargetOverlay.IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PmgDesigner] GetOrCreateVBox >> no visible SOverlay found (visited=%d, candidates=%d, root_children=%d)"),
		       VisitedCount, OverlayCandidates.Num(), Children->Num());
		return nullptr;
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

bool FPmgDesignerExtension::ReloadVBoxList(TWeakObjectPtr<UWidgetBlueprint> InBlueprint, TWeakPtr<SVerticalBox> InVBox, const TArray<FInfoLine>& InLines)
{
	
	TSharedPtr<SVerticalBox> VBoxPtr = InVBox.Pin();
	
	if (!VBoxPtr.IsValid() || !InBlueprint.IsValid())
	{
		return false;
	}

	// 检查 VBox，保证槽位数量足够；非 STextBlock 的子控件直接跳过（Tag 撞车时不能 StaticCast 断言崩溃）
	const int32 ReloadCount = FMath::Max(InLines.Num(), VBoxPtr->GetChildren()->Num());
	for (int32 i = 0; i < ReloadCount; ++i)
	{
		// AddSlot 会让 FChildren 失效，每轮重新取
		if (i >= VBoxPtr->GetChildren()->Num())
		{
			VBoxPtr->AddSlot()[SNew(STextBlock)];
		}

		FChildren* Children = VBoxPtr->GetChildren();
		if (!Children || i >= Children->Num())
		{
			continue;
		}

		TSharedRef<SWidget> ChildRef = Children->GetChildAt(i);
		if (ChildRef->GetTypeAsString() != TEXT("STextBlock"))
		{
			continue;
		}

		// TSharedRef 保证非空，无需再校验
		TSharedRef<STextBlock> TextBlock = StaticCastSharedRef<STextBlock>(ChildRef);

		if (i < InLines.Num())
		{
			TextBlock->SetText(FText::FromString(InLines[i].Text));
			TextBlock->SetColorAndOpacity(FSlateColor(InLines[i].Color));
			TextBlock->SetVisibility(EVisibility::SelfHitTestInvisible);
		}
		else
		{
			TextBlock->SetVisibility(EVisibility::Collapsed);
		}
	}
	return true;
}

void FPmgDesignerExtension::UpdateShowData()
{
	if (VBoxs.Num() == 0)
	{
		return;
	}

	TArray<TWeakObjectPtr<UWidgetBlueprint>> InvalidKeys;
	// 本函数只在 game thread tick 里跑，用 game thread 版本读取
	const bool bOverlayEnabled = CVarDrawCallOverlay.GetValueOnGameThread();

	// DC 阈值配色：绿=常态，黄=偏多，红=需要关注
	const auto ColorForTotal = [](int32 Value)
	{
		if (Value >= 64)
		{
			return FLinearColor(0.95f, 0.35f, 0.28f);
		}
		return Value >= 24 ? FLinearColor(0.95f, 0.75f, 0.30f) : FLinearColor(0.85f, 0.85f, 0.85f);
	};
	const FLinearColor DimColor(0.55f, 0.55f, 0.55f);

	TArray<TPair<TWeakObjectPtr<UWidgetBlueprint>, TSharedPtr<SVerticalBox>>> Reattached;

	for (const TPair<TWeakObjectPtr<UWidgetBlueprint>, TWeakPtr<SVerticalBox>>& Iter : VBoxs)
	{
		TSharedPtr<SVerticalBox> VBox = Iter.Value.Pin();
		UWidgetBlueprint* WidgetBlueprint = Iter.Key.Get();
		if (!WidgetBlueprint)
		{
			InvalidKeys.Add(Iter.Key);
			continue;
		}

		if (!VBox.IsValid())
		{
			// 自愈：designer 重建子控件会把我们挂的 slot 一起冲掉，VBox 变成悬空弱引用。
			// 重新挂一次，而不是把这个蓝图当成失效键删掉（否则面板会"注册成功但永远不显示"）。
			const TWeakPtr<SDesignerView> DesignerView = DesignerViews.FindRef(Iter.Key);
			VBox = DesignerView.IsValid() ? GetOrCreateVBox(DesignerView.Pin().Get(), WidgetBlueprint) : nullptr;
			if (!VBox.IsValid())
			{
				InvalidKeys.Add(Iter.Key);
				continue;
			}
			// 不能边遍历 TMap 边写它，先收集，循环外再落表
			Reattached.Emplace(Iter.Key, VBox);
			LastRenderedSignature.Remove(Iter.Key);
		}

		// 开关关闭时保留 VBox 但隐藏，避免反复创建/销毁 Slate 控件
		if (!bOverlayEnabled)
		{
			VBox->SetVisibility(EVisibility::Collapsed);
			continue;
		}
		VBox->SetVisibility(EVisibility::SelfHitTestInvisible);

		const FDrawCallState Stats = CalculateWidgets(Iter.Key);

		TArray<FInfoLine> Lines;
		Lines.Add({FString::Printf(TEXT("Current Widget:\t %s"), *GetBlueprintName(Iter.Key)), FLinearColor::White});
		Lines.Add({FString::Printf(TEXT("Widget DrawCalls:\t %d"), Stats.Total), ColorForTotal(Stats.Total)});
		Lines.Add({FString::Printf(TEXT("  Image %d · Text %d · Border %d · Box %d · Effect %d"),
		                          Stats.ImageCount, Stats.TextCount, Stats.BorderCount, Stats.BoxCount, Stats.EffectCount), DimColor});
		Lines.Add({FString::Printf(TEXT("  元素 %d · 强制断批 %d"), Stats.RenderableCount, Stats.ForceBreakCount), DimColor});
		Lines.Add({FString::Printf(TEXT("  扫描 %d 控件 · 未识别 %d"), Stats.VisitedCount, Stats.UnknownLeafCount),
		           Stats.UnknownLeafCount > 0 ? FLinearColor(0.95f, 0.75f, 0.30f) : DimColor});
		Lines.Add({TEXT("  预估，非实测"), DimColor});

		// 内容没变就不重刷 Slate：SetText 会让布局失效，每秒重刷纯属浪费
		FString Signature;
		for (const FInfoLine& Line : Lines)
		{
			Signature.Append(Line.Text);
			Signature.AppendChar(TEXT('\n'));
		}
		FString& LastSignature = LastRenderedSignature.FindOrAdd(Iter.Key);
		if (LastSignature == Signature)
		{
			continue;
		}

		// 每次内容变化留一条可抓取的证据（MCP/日志回归用）
		UE_LOG(LogTemp, Log, TEXT("[PmgDesigner] dc >> bp=%s dc=%d elements=%d visited=%d unknown=%d force=%d"),
		       *GetBlueprintName(Iter.Key), Stats.Total, Stats.RenderableCount, Stats.VisitedCount,
		       Stats.UnknownLeafCount, Stats.ForceBreakCount);

		// 必须传自愈后的 VBox（Iter.Value 此时可能已是悬空弱指针），
		// 且只有真正写进 Slate 才提交签名——否则刷新失败会被误判为"已最新"，面板永久停在旧值。
		if (ReloadVBoxList(Iter.Key, VBox, Lines))
		{
			LastSignature = Signature;
		}
	}
	
	for (const TPair<TWeakObjectPtr<UWidgetBlueprint>, TSharedPtr<SVerticalBox>>& Pair : Reattached)
	{
		VBoxs.Add(Pair.Key, Pair.Value);
		UE_LOG(LogTemp, Log, TEXT("[PmgDesigner] reattach >> bp=%s"), *GetBlueprintName(Pair.Key));
	}

	for (const TWeakObjectPtr<UWidgetBlueprint>& Key : InvalidKeys)
	{
		VBoxs.Remove(Key);
		LastRenderedSignature.Remove(Key);
		DesignerViews.Remove(Key);
	}
}

void FPmgDesignerExtension::Cleanup()
{
	// 终态且幂等：模块关闭 / 编辑器关闭 / 析构可能重复调用
	if (bIsCleanedUp)
	{
		return;
	}
	bIsCleanedUp = true;

	// 1) 停止 tick，避免模块卸载后仍有回调进入模块代码
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	// 3) 注销全部委托
	RemoveAssetEditorSubsystemEvent();

	// 4) 释放 UI 状态：清空 VBox 内容并隐藏，断开对控件树的强持有
	for (auto& Pair : VBoxs)
	{
		if (TSharedPtr<SVerticalBox> VBox = Pair.Value.Pin())
		{
			VBox->ClearChildren();
			VBox->SetVisibility(EVisibility::Collapsed);
		}
	}
	VBoxs.Empty();
	DesignerViews.Empty();
	LastRenderedSignature.Empty();
	PendingInit.Empty();

	bIsDestroyed = true;
}

bool FPmgDesignerExtension::HandleTick(float DeltaTime)
{
	if (this->bIsDestroyed || this->bIsCleanedUp)
	{
		return false;
	}
	
	if (!GEditor || !GEditor->IsValidLowLevel())
	{
		// 不要返回 false：那会把 ticker 永久摘掉且不再重挂（编辑器会话重建后就不再刷新）
		return true;
	}

	// 节流：控件树遍历 + ZOrder 排序 + 字符串格式化不便宜，按墙钟限频。
	// 不能用 DeltaTime 累加：FTSTicker 传的是帧 delta，不是 tick 间隔。
	const double NowSeconds = FPlatformTime::Seconds();
	if (NowSeconds - LastRefreshTimeSeconds < RefreshIntervalSeconds)
	{
		return true;
	}
	LastRefreshTimeSeconds = NowSeconds;

	TryPendingInit();
	UpdateShowData();
	return true;
}

void FPmgDesignerExtension::TryPendingInit()
{
	if (PendingInit.Num() == 0)
	{
		return;
	}

	for (auto It = PendingInit.CreateIterator(); It; ++It)
	{
		UWidgetBlueprint* PendingBP = It.Key().Get();
		TSharedPtr<SDesignerView> PendingDesigner = It.Value().Pin();

		// 蓝图关了就丢弃；designer 暂时失效（预览模式切换/重建）就等下一 tick
		if (!PendingBP)
		{
			It.RemoveCurrent();
			continue;
		}
		if (!PendingDesigner.IsValid())
		{
			continue;
		}

		InitVBox(PendingDesigner.Get(), PendingBP);
		if (VBoxs.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

FString FPmgDesignerExtension::GetBlueprintName(TWeakObjectPtr<UWidgetBlueprint> InBlueprint)
{
	if (!InBlueprint.IsValid())
	{
		return FString();
	}
	return InBlueprint.Get()->GetName();
}

FPmgDesignerExtension::FDrawCallState FPmgDesignerExtension::CalculateWidgets(TWeakObjectPtr<UWidgetBlueprint> InBlueprint)
{
	FDrawCallState State;

	if (!InBlueprint.IsValid())
	{
		return State;
	}

	UWidgetBlueprint* BlueprintPtr = InBlueprint.Get();
	// 取得蓝图中的控件树
	UWidgetTree* WidgetTree = BlueprintPtr ? BlueprintPtr->WidgetTree : nullptr;
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		return State;
	}

	TSet<const UWidget*> Visited;
	CalculateDrawCallRecursive(WidgetTree->RootWidget, State, Visited, 0);

	// 不再兜底成 1：全部 Collapsed/Hidden 的界面真实 DC 就是 0
	return State;
}

void FPmgDesignerExtension::CalculateDrawCallRecursive(UWidget* Widget, FDrawCallState& State, TSet<const UWidget*>& Visited, int32 Depth)
{
	// 防御：控件树理论上无环，但蓝图重编译/复制期间出现过脏引用，双保险
	if (!Widget || Depth > 64 || Visited.Contains(Widget))
	{
		return;
	}
	Visited.Add(Widget);

	++State.VisitedCount;

	// 注意：不能用 UWidget::IsVisible()——它读的是 GetCachedWidget()（活的 Slate 控件），
	// 而蓝图 WidgetTree 里的控件从未实例化成 Slate，IsVisible() 恒为 false，
	// 结果是递归在第一层就返回、DC 永远是 0（旧实现再兜底成 1，所以面板一直显示常数 1）。
	// 这里只按控件自身的 Visibility 属性判断。
	const ESlateVisibility WidgetVisibility = Widget->GetVisibility();
	if (WidgetVisibility == ESlateVisibility::Collapsed || WidgetVisibility == ESlateVisibility::Hidden)
	{
		return;
	}

	// 裁剪是层级语义而不是线性语义：Inherit 沿用父级，其它值进入新的裁剪区
	// 进入新裁剪区会插入 clip state，必然打断合批
	const EWidgetClipping OwnClipping = Widget->GetClipping();
	const bool bEnteredClipRegion = OwnClipping != EWidgetClipping::Inherit && OwnClipping != State.CurrentClipping;
	const EWidgetClipping PrevClipping = State.CurrentClipping;
	const int32 PrevClipLayer = State.ClipLayer;
	if (bEnteredClipRegion)
	{
		State.CurrentClipping = OwnClipping;
		++State.ClipLayer;
	}

	// RenderOpacity < 1 会换 shader，不能与不透明元素合批
	const bool bBlended = Widget->GetRenderOpacity() < 1.0f;

	// 控件自身的绘制元素（Image/Text/Border/Button 等在 Slate 里都各自产出 draw element）
	EmitWidgetElements(Widget, State, bBlended);

	// ScrollBox 内部始终裁剪，子控件必然处在新的裁剪区
	const bool bScrollClips = Widget->IsA<UScrollBox>();
	if (bScrollClips)
	{
		++State.ClipLayer;
	}

	// 递归处理子控件
	if (UWidgetSwitcher* WidgetSwitcher = Cast<UWidgetSwitcher>(Widget))
	{
		// WidgetSwitcher 只画当前激活的那个子控件，其余子控件不产生绘制
		if (UWidget* ActiveChild = WidgetSwitcher->GetActiveWidget())
		{
			CalculateDrawCallRecursive(ActiveChild, State, Visited, Depth + 1);
		}
	}
	else if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
	{
		TArray<UWidget*> Children;
		Children.Reserve(PanelWidget->GetChildrenCount());
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
			CalculateDrawCallRecursive(Child, State, Visited, Depth + 1);
		}
	}
	// 嵌套蓝图：展开它自己的 WidgetTree（旧实现对 UUserWidget 完全不递归，是估算最大的漏项）
	else if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		if (UWidgetTree* NestedTree = UserWidget->WidgetTree)
		{
			CalculateDrawCallRecursive(NestedTree->RootWidget, State, Visited, Depth + 1);
		}
	}

	if (bScrollClips)
	{
		--State.ClipLayer;
	}

	// 退出子树时恢复裁剪状态：否则兄弟子树会被上一个小节的裁剪状态污染
	if (bEnteredClipRegion)
	{
		State.CurrentClipping = PrevClipping;
		State.ClipLayer = PrevClipLayer;
	}
}

void FPmgDesignerExtension::EmitWidgetElements(UWidget* Widget, FDrawCallState& State, bool bBlended)
{
	if (!Widget)
	{
		return;
	}

	// 本控件所在 Slate layer：UWidget 本身没有 ZOrder，能影响 layer 的是 CanvasPanel 槽位
	State.CurrentLayer = 0;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		State.CurrentLayer = CanvasSlot->GetZOrder();
	}

	// RetainerBox 把内容画到独立渲染目标（SRetainerWidget）：内容入口必然断批。
	// InvalidationBox 只是绘制元素缓存（SInvalidationPanel），不换渲染目标，不计强制断批。
	if (Widget->IsA<URetainerBox>())
	{
		EmitElement(State, Widget, EElementKind::Effect, bBlended);
		State.bForceBreakNext = true;
		++State.ForceBreakCount;
		return;
	}

	// 背景模糊走独立像素着色器，永远无法与别的东西合批
	if (UBackgroundBlur* BackgroundBlur = Cast<UBackgroundBlur>(Widget))
	{
		EmitElement(State, BackgroundBlur, EElementKind::Effect, bBlended);
		State.bForceBreakNext = true;
		++State.ForceBreakCount;
		return;
	}

	if (UImage* Image = Cast<UImage>(Widget))
	{
		const FSlateBrush& Brush = Image->GetBrush();
		// 没有贴图/材质的 Image 不产生绘制（纯底色由 Border 承担）
		if (Brush.DrawAs != ESlateBrushDrawType::NoDrawType)
		{
			EmitElement(State, GetBrushResource(Brush), EElementKind::Image, bBlended);
		}
		return;
	}

	if (UBorder* Border = Cast<UBorder>(Widget))
	{
		// SBorder 没有画刷资源时用默认白色画刷铺底色，仍然会产生一次绘制
		EmitElement(State, GetBrushResource(Border->Background), EElementKind::Border, bBlended);
		return;
	}

	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		// 文本按字体合批：字号 / 描边 / 阴影 / 字重（Typeface）不同都会断批
		const FSlateFontInfo& Font = TextBlock->GetFont();
		const bool bHasShadow = TextBlock->GetShadowOffset().SizeSquared() > 0.0f;
		EmitElement(State, Font.FontObject.Get(), EElementKind::Text, bBlended,
		            FMath::RoundToInt(Font.Size), Font.OutlineSettings.OutlineSize, bHasShadow,
		            Font.TypefaceFontName);
		return;
	}

	// SButton 自己画 style 背景
	if (UButton* Button = Cast<UButton>(Widget))
	{
		EmitElement(State, GetBrushResource(Button->GetStyle().Normal), EElementKind::Box, bBlended);
		return;
	}

	// SCheckBox = 按钮底 + 勾选图
	if (UCheckBox* CheckBox = Cast<UCheckBox>(Widget))
	{
		const FCheckBoxStyle& Style = CheckBox->GetWidgetStyle();
		EmitElement(State, GetBrushResource(Style.BackgroundImage), EElementKind::Box, bBlended);
		EmitElement(State, GetBrushResource(CheckBox->IsChecked() ? Style.CheckedImage : Style.UncheckedImage),
		            EElementKind::Image, bBlended);
		return;
	}

	// SProgressBar = 底图 + 填充图
	if (UProgressBar* ProgressBar = Cast<UProgressBar>(Widget))
	{
		const FProgressBarStyle& Style = ProgressBar->GetWidgetStyle();
		EmitElement(State, GetBrushResource(Style.BackgroundImage), EElementKind::Image, bBlended);
		EmitElement(State, GetBrushResource(Style.FillImage), EElementKind::Image, bBlended);
		return;
	}

	// SSlider = 轨道 + 滑块
	if (USlider* Slider = Cast<USlider>(Widget))
	{
		const FSliderStyle& Style = Slider->GetWidgetStyle();
		EmitElement(State, GetBrushResource(Style.NormalBarImage), EElementKind::Image, bBlended);
		EmitElement(State, GetBrushResource(Style.NormalThumbImage), EElementKind::Image, bBlended);
		return;
	}

	// SScrollBox = 滚动条底 + 滑块（内容裁剪在递归处处理）
	if (UScrollBox* ScrollBox = Cast<UScrollBox>(Widget))
	{
		const FScrollBarStyle& BarStyle = ScrollBox->GetWidgetBarStyle();
		EmitElement(State, GetBrushResource(BarStyle.VerticalBackgroundImage), EElementKind::Image, bBlended);
		EmitElement(State, GetBrushResource(BarStyle.NormalThumbImage), EElementKind::Image, bBlended);
		return;
	}

	// 可编辑文本与 TextBlock 同族，合批规则一致
	if (UEditableText* EditableText = Cast<UEditableText>(Widget))
	{
		const FSlateFontInfo& Font = EditableText->GetFont();
		EmitElement(State, Font.FontObject.Get(), EElementKind::Text, bBlended,
		            FMath::RoundToInt(Font.Size), Font.OutlineSettings.OutlineSize, false,
		            Font.TypefaceFontName);
		return;
	}

	// 已知纯布局控件（自身不画东西）：不计入"未识别"
	if (Widget->IsA<UPanelWidget>() || Widget->IsA<UUserWidget>())
	{
		return;
	}

	// 其它叶子控件：可能是不认识的绘制控件，只计数不臆测 DC，提示"估算可能偏低"
	++State.UnknownLeafCount;
}

void FPmgDesignerExtension::EmitElement(FDrawCallState& State, const UObject* Resource, EElementKind Kind, bool bBlended,
                                        int32 FontSize, int32 OutlineSize, bool bHasShadow, FName TypefaceFontName)
{
	FBatchKey Key;
	Key.Resource = Resource;
	Key.Primitive = static_cast<uint8>(Kind);
	Key.ClipLayer = State.ClipLayer;
	Key.bBlended = bBlended;
	Key.FontSize = FontSize;
	Key.OutlineSize = OutlineSize;
	Key.bHasShadow = bHasShadow;
	Key.TypefaceFontName = TypefaceFontName;
	Key.Layer = State.CurrentLayer;

	const bool bBreak = State.bForceBreakNext || !State.bHasDrawn || Key != State.LastKey;
	if (bBreak)
	{
		++State.Total;
		State.LastKey = Key;
		State.bHasDrawn = true;
		State.bForceBreakNext = false;
	}

	++State.RenderableCount;
	switch (Kind)
	{
	case EElementKind::Image:  ++State.ImageCount;  break;
	case EElementKind::Text:   ++State.TextCount;   break;
	case EElementKind::Border: ++State.BorderCount; break;
	case EElementKind::Box:    ++State.BoxCount;    break;
	case EElementKind::Effect: ++State.EffectCount; break;
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
