// Fill out your copyright notice in the Description page of Project Settings.

#include "CORUIManager.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "CORUI/Config/CORUIManagerSetting.h"
#include "CORUI/Public/UI/Container/CORCanvasContainer.h"
#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "UI/Widget/CORWindow.h"
#include "GameFramework/GameModeBase.h"
#include "UEDataBinding.hpp"
#if !UE_BUILD_SHIPPING
#include "UObject/GarbageCollection.h"
#endif

void UCORUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::Initialize"));

	// 初始化层级数组
	{
		LayerTypes.Empty();
		for (const auto& LayerType : TEnumRange<EUILayerType>())
		{
			LayerTypes.Add(LayerType);
		}
	}

	// 初始化层级ZOrder配置
	{
		LayersZOrder.Empty();
		if (UUILayerTypeZOrderConfigData* Config = GetDefault<UCORUIManagerSetting>()->LayerTypeZOrderConfigPath.
			LoadSynchronous())
		{
			LayersZOrder = Config->LayerZOrderConfig;
		}
		if (LayersZOrder.Num() < LayerTypes.Num())
		{
			// 默认层级ZOrder倍率
			int32 DefaultZOrderMultiplier = GetDefault<UCORUIManagerSetting>()->DefaultZOrderMultiplier;
			for (const auto& LayerType : LayerTypes)
			{
				// 如果没有配置过，则用默认的ZOrder
				if (!LayersZOrder.Contains(LayerType))
				{
					LayersZOrder.Add(LayerType, static_cast<int32>(LayerType) * DefaultZOrderMultiplier);
				}
			}
		}
		if (LayersZOrder.Num() < LayerTypes.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::Initialize >> LayersZOrder can't load."))
		}
	}

	// 初始化层级ZOrder配置
	{
		// 默认层级ZOrder倍率
		int32 DefaultZOrderMultiplier = GetDefault<UCORUIManagerSetting>()->DefaultZOrderMultiplier;

		// 根据ZOrder为层级数组排序，Zorder越小，越靠前
		LayerTypes.Sort([this, DefaultZOrderMultiplier](EUILayerType A, EUILayerType B)
		{
			int32 ZOrderA = static_cast<int32>(A) * DefaultZOrderMultiplier;
			if (int32* ZOrderPtrA = LayersZOrder.Find(A))
			{
				ZOrderA = *ZOrderPtrA;
			}

			int32 ZOrderB = static_cast<int32>(B) * DefaultZOrderMultiplier;
			if (int32* ZOrderPtrB = LayersZOrder.Find(B))
			{
				ZOrderB = *ZOrderPtrB;
			}

			return ZOrderA < ZOrderB;
		});
	}

	// 绑定世界开始的委托
	UIBeginPlayHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject(this, &UCORUIManager::OnUIBeginPlay);

	// 世界清理（换图 / 结束 PIE）：统一走关闭边界，避免旧世界的 viewport 残留
	UIWorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UCORUIManager::OnWorldCleanup);
}

void UCORUIManager::Deinitialize()
{
	
	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::Deinitialize"));
	
	// 移除世界开始的委托
	FGameModeEvents::GameModeInitializedEvent.Remove(UIBeginPlayHandle);

	if (UIWorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(UIWorldCleanupHandle);
		UIWorldCleanupHandle.Reset();
	}

	OnUIEndPlay(true);

	Super::Deinitialize();
}

bool UCORUIManager::IsUIInitialized() const
{
	return SessionPhase == ESessionPhase::Ready && IsValid(RootCanvas);
}

#if !UE_BUILD_SHIPPING
bool UCORUIManager::VerifyOwnershipAfterGC(FString& OutReport)
{
	TArray<FString> Results;
	int32 Failed = 0;
	int32 Passed = 0;

	// 未就绪时没有可验证的会话：显式 SKIP，避免"空跑也 PASS"的假证据
	if (SessionPhase != ESessionPhase::Ready || !IsValid(RootCanvas))
	{
		OutReport = FString::Printf(
			TEXT("SKIP >> UI not ready (SessionPhase=%d, RootCanvas=%s): open the UI first"),
			static_cast<int32>(SessionPhase), IsValid(RootCanvas) ? TEXT("valid") : TEXT("null"));
		return false;
	}

	const auto Report = [&Results, &Failed, &Passed](const TCHAR* InName, bool bInOk, const FString& InDetail)
	{
		if (bInOk)
		{
			++Passed;
		}
		else
		{
			++Failed;
		}
		Results.Add(FString::Printf(TEXT("%s %s%s%s"),
			bInOk ? TEXT("PASS") : TEXT("FAIL"), InName,
			InDetail.IsEmpty() ? TEXT("") : TEXT(" "), *InDetail));
	};

	// 无判别力的"存活计数"：GC 只置空 TObjectPtr、不增删容器项，中间也没有别的代码，
	// 这类"数量不变"结构上永远成立。只作上下文输出（INFO），不参与 PASS/FAIL 判定。
	const auto Info = [&Results](const TCHAR* InName, const FString& InDetail)
	{
		Results.Add(FString::Printf(TEXT("INFO %s %s"), InName, *InDetail));
	};

	// GC 前快照：用于比较"是否有东西被回收"，而不是只看当前是否非空
	const int32 LayersBefore = LayerPanels.Num();
	const auto& ActiveBeforeView = WidgetPool.GetActiveWidgets();
	const int32 ActiveBefore = ActiveBeforeView.Num();

	// 测试有效性探针：一个没有任何强引用的瞬态对象，GC 必须能把它回收。
	// 否则"root/layers 存活"可能只是这次 GC 什么也没回收，PASS 就没有意义（自证有效）。
	TWeakObjectPtr<UCanvasPanel> Probe = NewObject<UCanvasPanel>(GetTransientPackage(), NAME_None, RF_Transient);
	Report(TEXT("gc-probe"), Probe.IsValid(), Probe.IsValid() ? FString() : FString(TEXT("probe spawn failed")));

	// 全量 GC + purge：所有权链一旦有缺口，根 canvas / 层容器会在这里被回收
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);

	// 探针必须被回收，否则本次验证无效
	Report(TEXT("gc-effective"), !Probe.IsValid(),
	       Probe.IsValid() ? FString(TEXT("unreferenced probe survived GC")) : FString(TEXT("probe collected")));

	// V1 根面板：F1 唯一真正的裸奔点（裸 UWidget，没有 SObjectWidget 的 FGCObject 兜底）
	Report(TEXT("root"), IsValid(RootCanvas),
	       IsValid(RootCanvas) ? FString::Printf(TEXT("(%s)"), *RootCanvas->GetName()) : FString(TEXT("collected by GC")));

	// V2 层容器：数量不变（INFO，无判别力）+ 每个仍然有效
	Info(TEXT("layer-count"), FString::Printf(TEXT("%d->%d"), LayersBefore, LayerPanels.Num()));
	int32 InvalidLayers = 0;
	for (const TPair<EUILayerType, TObjectPtr<UCORCanvasContainer>>& Pair : LayerPanels)
	{
		if (!IsValid(Pair.Value))
		{
			++InvalidLayers;
		}
	}
	Report(TEXT("layer-valid"), InvalidLayers == 0, FString::Printf(TEXT("invalid=%d"), InvalidLayers));

	// V3 对象池：数量不变（INFO，无判别力）+ UObject 与 Slate 不脱钩
	const auto& ActiveAfterView = WidgetPool.GetActiveWidgets();
	Info(TEXT("pool-active"), FString::Printf(TEXT("%d->%d"), ActiveBefore, ActiveAfterView.Num()));
	int32 InvalidWidgets = 0;
	int32 DetachedSlate = 0;
	for (int32 Index = 0; Index < ActiveAfterView.Num(); ++Index)
	{
		UUserWidget* Widget = ActiveAfterView[Index];
		if (!IsValid(Widget))
		{
			++InvalidWidgets;
			continue;
		}
		if (!Widget->GetCachedWidget().IsValid())
		{
			++DetachedSlate;
		}
	}
	Report(TEXT("pool-valid"), InvalidWidgets == 0, FString::Printf(TEXT("invalid=%d"), InvalidWidgets));
	Report(TEXT("pool-slate"), DetachedSlate == 0, FString::Printf(TEXT("detached=%d"), DetachedSlate));

	OutReport = FString::Join(Results, TEXT(" | "));
	// 计数只算判定项（INFO 无判别力，不计入分母）
	OutReport += FString::Printf(TEXT(" || %s %d/%d"),
	                             Failed == 0 ? TEXT("PASS") : TEXT("FAIL"), Passed, Passed + Failed);
	return Failed == 0;
}
#endif

void UCORUIManager::ShutdownUI()
{
	// 幂等：重复调用与 OnUIEndPlay 共享同一关闭序列（会话级，不销毁对外订阅）
	OnUIEndPlay(false);
}

void UCORUIManager::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	// 只处理当前世界：其他世界的清理不影响本管理器
	if (World && World == GetWorld())
	{
		UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::OnWorldCleanup >> world cleaned up"));
		OnUIEndPlay(false);
	}
}

void UCORUIManager::OnUIBeginPlay(AGameModeBase* GameMode)
{
	// 世界加载完成后，延迟一帧创建UI
	if (GameMode && GameMode->GetWorld() == GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::InitUI));
	}
}

void UCORUIManager::OnUIEndPlay(bool bFinalTeardown)
{
	// 幂等：已经完成本次会话关闭时只处理最终销毁的收尾
	if (SessionPhase != ESessionPhase::Stopped)
	{
		const bool bHadLiveSession = SessionPhase == ESessionPhase::Ready || SessionPhase == ESessionPhase::Starting;

		// 关闭顺序：拒绝新工作 -> 在 JS 存活时关窗口（广播 Deinit 后由 ClearUI 触发 JS Destruct）-> 释放 UI
		SessionPhase = ESessionPhase::Stopping;

		if (bHadLiveSession)
		{
			UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::OnWorldEndPlay >> Call ts OnDeinit"));
			OnUIManagerDeinit.Broadcast();
		}

		bSuppressPendingCallbacks = bFinalTeardown;
		ClearUI();
		bSuppressPendingCallbacks = false;

		SessionPhase = ESessionPhase::Stopped;
	}

	if (bFinalTeardown)
	{
		// 仅在子系统销毁时清空对外委托：换图是会话级关闭，JS 订阅必须保留以便重建后再次初始化
		OnUIManagerInit.Clear();
		OnUIManagerDeinit.Clear();
	}
}

void UCORUIManager::InitUI()
{
	// 幂等：重复的 GameMode 初始化 / 重复 next-tick 不再重建根节点
	if (SessionPhase == ESessionPhase::Ready || SessionPhase == ESessionPhase::Starting)
	{
		UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::InitUI >> already initialized, skip"));
		return;
	}

	// 防御：上一次会话残留的根节点先清掉，避免新旧 viewport 内容并存
	if (IsValid(RootCanvas))
	{
		ClearUI();
	}

	SessionPhase = ESessionPhase::Starting;

	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::InitUI >> InitUI"));

	// 对象池必须在创建分层容器之前绑定世界，容器才能拿到有效的池引用
	WidgetPool.SetWorld(GetWorld());

	// 获取根Canvas
	UCanvasPanel* NewRootCanvas = NewObject<UCanvasPanel>(GetWorld(), UCanvasPanel::StaticClass(),
	                                                      FName(TEXT("RootCanvas")));
	if (!NewRootCanvas)
	{
		SessionPhase = ESessionPhase::Created;
		return;
	}

	// 开始创建分层容器
	for (const auto& LayerType : LayerTypes)
	{
		// 创建分层容器
		if (UCORCanvasContainer* LayerContainer = NewObject<UCORCanvasContainer>(
			NewRootCanvas, UCORCanvasContainer::StaticClass()))
		{
			LayerContainer->LayerType = LayerType;
			LayerContainer->LayerZOrder = GetLayerZOrder(LayerType);

			// 添加到根Canvas
			NewRootCanvas->AddChildToCanvas(LayerContainer);
			LayerPanels.Add(LayerType, LayerContainer);

			// 注入对象池，窗口移除时归还到该池
			LayerContainer->SetWidgetPool(&WidgetPool);

			// 获取Slot，设置参数
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(LayerContainer->GetSlot()))
			{
				// 铺满
				Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				// 设置起点为左上角
				Slot->SetPosition(FVector2D::ZeroVector);
				Slot->SetSize(FVector2D::ZeroVector);
				Slot->SetAutoSize(true);
				// 设置ZOrder
				Slot->SetZOrder(LayerContainer->LayerZOrder);
			}
		}
	}

	// 添加到视口
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(NewRootCanvas->TakeWidget());
	}
	else
	{
		// 半初始化回滚：容器和根节点都还在，直接丢弃，不进入 Ready
		NewRootCanvas->ClearChildren();
		LayerPanels.Empty();
		SessionPhase = ESessionPhase::Created;
		return;
	}

	RootCanvas = NewRootCanvas;

	// 就绪后再广播：JS 侧收到事件时窗口系统已经可用
	SessionPhase = ESessionPhase::Ready;

	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::InitUI >> Call ts OnInit"));
	OnUIManagerInit.Broadcast();
}

void UCORUIManager::ClearUI()
{
	// F5/F6：先失效会话并取消在途请求（完成回调会拿到旧 generation，无法再开窗），
	// 再逐层释放窗口。
	++UISessionGeneration;
	CancelPendingPushes();

	// 逐层清理窗口，并断开容器对对象池的引用（必须在 SetWorld(nullptr) 之前）
	for (const TPair<EUILayerType, TObjectPtr<UCORCanvasContainer>>& LayerPanel : LayerPanels)
	{
		if (UCORCanvasContainer* Container = LayerPanel.Value.Get())
		{
			// 逐窗剥离并归还对象池，触发 NativeDestruct / JS Destruct
			Container->ClearAllWindows();
			Container->SetWidgetPool(nullptr);
		}
	}
	LayerPanels.Empty();

	// 释放顺序：容器/窗口 -> 移除 viewport 内容 -> 断开根强引用 -> 释放对象池
	if (IsValid(RootCanvas))
	{
		RootCanvas->ClearChildren();

		// 与 InitUI 中的 AddViewportWidgetContent 配对移除
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(RootCanvas->TakeWidget());
		}
	}

	// 强引用必须显式断开，否则根 canvas 与层容器会一直存活到管理器销毁
	RootCanvas = nullptr;

	WidgetPool.ReleaseAll(true);
	WidgetPool.ResetPool();
	WidgetPool.SetWorld(nullptr);
}

FName UCORUIManager::NativePushUI(UClass* WindowClass, FName WindowName, EUILayerType PushLayer,
                                  FJsObject Callback)
{
	FScopeLock Lock(&DataLock);

	if (!IsValid(RootCanvas))
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePushUI >> RootCanvas is not valid"))
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	if (!IsValid(WindowClass))
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePushUI >> WindowClass is not valid"))
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// 获取CDO
	UCORWindow* WindowCDO = Cast<UCORWindow>(WindowClass->GetDefaultObject());
	if (!WindowCDO)
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePushUI >> WindowCDO is not valid"))
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// 如果传入的命名是None，则获取CDO中存储的命名
	if (WindowName == NAME_None)
	{
		WindowName = WindowCDO->GetWindowName();
	}

	// F8：标识校验必须在取池之前完成，避免取出的实例成为“失败激活”残留
	if (WindowName == NAME_None)
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePushUI >> WindowName is None after CDO resolve"))
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// 如果传入的Layer是None，则获取CDO中配置的Layer信息
	if (PushLayer == EUILayerType::None)
	{
		PushLayer = WindowCDO->GetLayer();
	}

	// 检查目标层级是否已创建
	UCORCanvasContainer* LayerPanel = GetLayer(PushLayer);
	if (!IsValid(LayerPanel))
	{
		// 打印错误的层级信息
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePushUI >> Layer %s is not created"),
		       *UEnum::GetValueAsString(PushLayer))
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// 要处理的窗口实例
	UCORWindow* WindowInstance = LayerPanel->FindWindow(WindowName);
	if (IsValid(WindowInstance))
	{
		// 如果已经Push了这个UI，那么Pop掉它
		NativePopUI(WindowName, PushLayer, false);

		// 普通窗口会被移除并归还对象池，旧指针已不可用；需要重新从池中取实例
		// FullScreen 在 bPopSelf=false 时保留自身（仍注册在 LayerPanel 中），继续复用旧实例
		if (!LayerPanel->FindWindow(WindowName))
		{
			WindowInstance = nullptr;
		}
	}

	// 是否从对象池新取/复用（用于插入失败时归还，避免残留活跃实例）
	bool bAcquiredFromPool = false;
	if (!IsValid(WindowInstance))
	{
		// 没有可复用的实例，从对象池创建（复用 InactiveWidgets 中的实例）
		WindowInstance = WidgetPool.GetOrCreateInstance(TSubclassOf<UCORWindow>(WindowClass));
		bAcquiredFromPool = true;
	}

	if (!IsValid(WindowInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePushUI >> WindowInstance is not valid"))
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// 修改其指向的Layer
	WindowInstance->SetLayer(PushLayer);

	// 添加到Layer中
	const FName ResultName = LayerPanel->PushUI(WindowName, WindowInstance);
	if (ResultName == NAME_None)
	{
		// F8：插入失败，把刚取得的实例归还对象池，池计数与生命周期回到请求前
		if (bAcquiredFromPool)
		{
			WidgetPool.Release(WindowInstance, true);
		}
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// F3：事务校验——插入过程/回调可能已经关闭或替换这个激活
	if (!IsValid(WindowInstance) || LayerPanel->FindWindow(ResultName) != WindowInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUIManager::NativePushUI >> activation %s was closed during push"),
		       *ResultName.ToString())
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return NAME_None;
	}

	// F13：先让脚本完成数据初始化（OnScriptInit），再 Open/Show；
	// 脚本初始化失败时会在回调里回滚（关闭该窗口），下面的事务校验会跳过打开。
	Callback.Action(true, ResultName, WindowInstance);

	// 回调可能已关闭/替换该激活（脚本初始化里主动 CloseUI，属于预期路径）：不再打开
	if (!IsValid(WindowInstance) || LayerPanel->FindWindow(ResultName) != WindowInstance)
	{
		UE_LOG(LogTemp, Log,
		       TEXT("[CORUI] CORUIManager::NativePushUI >> activation %s was closed during script init"),
		       *ResultName.ToString())
		return NAME_None;
	}

	WindowInstance->NativeOpen();

	NativeUpdateUI();

	return ResultName;
}

void UCORUIManager::AsyncPushUI(FSoftClassPath WindowClassPath, FName WindowName, EUILayerType PushLayer,
                                FJsObject Callback)
{
	// 空路径：请求阶段就拒绝，且恰好一次失败回调（pending 不等于失败）
	if (WindowClassPath.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::AsyncPushUI >> WindowClassPath is empty"));
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return;
	}

	TSharedPtr<FPendingPushRequest> Request = MakeShared<FPendingPushRequest>();
	Request->RequestId = ++NextPushRequestId;
	Request->Generation = UISessionGeneration;
	Request->WindowClassPath = WindowClassPath;
	Request->WindowName = WindowName;
	Request->PushLayer = PushLayer;
	Request->Callback = Callback;

	// 请求被接受：回调只由完成路径触发，pending 不再是失败
	FStreamableDelegate LoadDelegate = FStreamableDelegate::CreateWeakLambda(
		this, [this, Request]()
		{
			CompleteAsyncPush(Request);
		});

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	TSharedPtr<FStreamableHandle> StreamableHandle = StreamableManager.RequestAsyncLoad(
		Request->WindowClassPath, LoadDelegate);

	if (!StreamableHandle.IsValid())
	{
		// 请求被加载器拒绝：这是确定的终态，恰好一次失败回调
		FailPushRequest(Request, TEXT("request rejected by streamable manager"));
		return;
	}

	// 已在加载器内部同步完成（资产已缓存）时，CompleteAsyncPush 已经跑完并移除了在途记录；
	// 此时不能再登记，否则会永久残留一条已完成记录（钉住 FJsObject 与句柄）。
	if (!Request->bCompleted)
	{
		FInFlightPushRequest InFlight;
		InFlight.Request = Request;
		InFlight.Handle = StreamableHandle;
		InFlightPushes.Add(InFlight);
	}
}

void UCORUIManager::CompleteAsyncPush(const TSharedPtr<FPendingPushRequest>& Request)
{
	if (!Request.IsValid() || Request->bCompleted)
	{
		// exactly-once：重复完成 / 已取消的请求直接忽略
		return;
	}

	// F6：会话已切换（Teardown / 重建 UI），旧请求不得在新根节点上创建窗口
	if (Request->Generation != UISessionGeneration)
	{
		FailPushRequest(Request, TEXT("UI session changed before request completed"));
		return;
	}

	UClass* WindowClass = Request->WindowClassPath.ResolveClass();
	if (!WindowClass)
	{
		// F2：类解析失败走统一完成路径，不再静默返回
		FailPushRequest(Request, TEXT("ResolveClass failed"));
		return;
	}

	// 由 NativePushUI 负责唯一一次成功/失败回调；先标记完成，防止重复终止
	Request->bCompleted = true;
	RemoveInFlightPush(Request);

	NativePushUI(WindowClass, Request->WindowName, Request->PushLayer, Request->Callback);
}

void UCORUIManager::FailPushRequest(const TSharedPtr<FPendingPushRequest>& Request, const TCHAR* Reason)
{
	if (!Request.IsValid() || Request->bCompleted)
	{
		return;
	}

	Request->bCompleted = true;
	RemoveInFlightPush(Request);

	UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUIManager::AsyncPushUI >> request %d failed: %s"),
	       Request->RequestId, Reason);

	Request->Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
}

void UCORUIManager::RemoveInFlightPush(const TSharedPtr<FPendingPushRequest>& Request)
{
	InFlightPushes.RemoveAll([&Request](const FInFlightPushRequest& Item)
	{
		return Item.Request == Request;
	});
}

void UCORUIManager::CancelPendingPushes()
{
	// 移出表后再取消：取消回调即使同步触发，也看不到这些请求
	TArray<FInFlightPushRequest> Pending = MoveTemp(InFlightPushes);
	InFlightPushes.Reset();

	for (FInFlightPushRequest& Item : Pending)
	{
		if (Item.Request.IsValid() && !Item.Request->bCompleted)
		{
			// 会话已结束：明确终止（取消也是一次终止结果）
			Item.Request->bCompleted = true;

			// 最终销毁时 JS 环境可能正在销毁，且回调重入 Push 只会再失败一次：不再回调
			if (!bSuppressPendingCallbacks)
			{
				Item.Request->Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
			}
		}

		if (Item.Handle.IsValid())
		{
			Item.Handle->CancelHandle();
		}
	}
}

void UCORUIManager::SyncPushUI(FSoftClassPath WindowClassPath, FName WindowName, EUILayerType PushLayer,
                               FJsObject Callback)
{
	// 空路径同样走统一失败回调
	if (WindowClassPath.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::SyncPushUI >> WindowClassPath is empty"));
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return;
	}

	UClass* WindowClass = WindowClassPath.TryLoadClass<UCORWindow>();
	if (!WindowClass)
	{
		// F2：同步加载失败也必须对外终止，不能静默返回
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::SyncPushUI >> load class failed: %s"),
		       *WindowClassPath.ToString());
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return;
	}
	NativePushUI(WindowClass, WindowName, PushLayer, Callback);
}

void UCORUIManager::PushUI(FString WindowClassPath, FJsObject Callback, FName WindowName,
                           EUILayerType PushLayer, bool bIsAsync)
{
	// 关闭/未就绪的会话不接受新工作（F5：拒绝新工作先于释放在途请求）
	if (!IsUIInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUIManager::PushUI >> UI is not ready, request rejected."));
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
		return;
	}

	if (bIsAsync)
	{
		AsyncPushUI(WindowClassPath, WindowName, PushLayer, Callback);
	}
	else
	{
		SyncPushUI(WindowClassPath, WindowName, PushLayer, Callback);
	}
}

void UCORUIManager::PopUI(FName WindowName, EUILayerType PushLayer, bool bPopSelf)
{
	NativePopUI(WindowName, PushLayer, bPopSelf);
}

void UCORUIManager::NativePopUI(FName WindowName, EUILayerType PushLayer, bool bPopSelf)
{
	FScopeLock Lock(&DataLock);
	UCORCanvasContainer* LayerPanel = GetLayer(PushLayer);
	if (!IsValid(LayerPanel))
	{
		// 打印错误的层级信息
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::NativePopUI >> Layer %s is not created"),
		       *UEnum::GetValueAsString(PushLayer))
		return;
	}

	LayerPanel->PopUI(WindowName, bPopSelf);
}

void UCORUIManager::NativeUpdateUI()
{
	FScopeLock Lock(&DataLock);

	// 生命周期回调里再次请求刷新：合并到外层循环出口，避免嵌套遍历
	if (bIsUpdatingUI)
	{
		bStateUpdateRequested = true;
		return;
	}

	bIsUpdatingUI = true;

	for (int32 i = LayerTypes.Num() - 1; i >= 0; i--)
	{
		UCORCanvasContainer* LayerPanel = GetLayer(LayerTypes[i]);
		if (!IsValid(LayerPanel))
		{
			continue;
		}

		// 更新目标层的状态
		LayerPanel->UpdateWindowState(EWindowState::None);
	}

	bIsUpdatingUI = false;

	// 回调期间累积的刷新请求，在出口补一次
	if (bStateUpdateRequested)
	{
		bStateUpdateRequested = false;
		NativeUpdateUI();
	}
}

TArray<EUILayerType> UCORUIManager::GetLayerTypes()
{
	return LayerTypes;
}

int32 UCORUIManager::GetLayerZOrder(EUILayerType Layer)
{
	// 默认层级ZOrder倍率
	int32 DefaultZOrderMultiplier = GetDefault<UCORUIManagerSetting>()->DefaultZOrderMultiplier;

	if (int32* ZOrderPtr = LayersZOrder.Find(Layer))
	{
		return *ZOrderPtr;
	}

	return static_cast<int32>(Layer) * DefaultZOrderMultiplier;
}

UCORCanvasContainer* UCORUIManager::GetLayer(EUILayerType Layer)
{
	TObjectPtr<UCORCanvasContainer>* LayerPtr = LayerPanels.Find(Layer);
	if (!LayerPtr || !IsValid(*LayerPtr))
	{
		return nullptr;
	}
	return LayerPtr->Get();
}

// ---------------------------------------------------------------------------
// 已废弃：早期的手写 Puerts 导出（保留仅作历史参考，**请勿取消注释**）
//
// 现状：UCORUIManager 不需要手动导出。Puerts 会按 UCLASS + UFUNCTION 自动生成绑定与
// TypeScript 声明（见 Typing/ue/ue.d.ts 的 `class CORUIManager`）。TS 侧通过
// `UE.CORUIManager` 直接调用 PushUI / PopUI / GetLayer / GetLayerTypes / GetLayerZOrder /
// IsUIInitialized，委托 OnUIManagerInit / OnUIManagerDeinit 同样由自动绑定提供
// （UIManager.ts 里的 CppSubSystem.PushUI(...) 走的就是这条路径）。
//
// 下面这段为什么不能直接用：
//   1) 类型名已过期：FOnCORUIStateNotify 现为 FOnCORUIManagerStateNotify（CORUIManager.h:12），
//      取消注释会编译失败；
//   2) 自动绑定已覆盖同名函数，再 DefineClass 属于重复注册；
//   3) 与 UFUNCTION 自动导出重复维护，容易再次漂移。
//
// 若未来确实要导出 UFUNCTION 覆盖不到的类型（例如纯 C++ 结构），请按现有写法加在
// Plugins/CORUI/Source/CORUI/Private/Core/AutoRegister/AutoRegister.cpp，不要恢复本段。
// ---------------------------------------------------------------------------
//
// ///////// puerts导出
// UsingUClass(UObject)
//
// UsingUClass(UCORUIManager)
//
// UsingCppType(FOnCORUIStateNotify)
//
// struct AutoRegisterForUCORUIManager
// {
// 	AutoRegisterForUCORUIManager()
// 	{
// 		puerts::DefineClass<FOnCORUIStateNotify>()
// 			.Register();
//
// 		puerts::DefineClass<UCORUIManager>()
// 			.Method("PushUI", MakeFunction(&UCORUIManager::PushUI))
// 			.Method("PopUI", MakeFunction(&UCORUIManager::PopUI))
// 			.Register();
// 	}
// };
//
// AutoRegisterForUCORUIManager _AutoRegisterForUCORUIManager_;
