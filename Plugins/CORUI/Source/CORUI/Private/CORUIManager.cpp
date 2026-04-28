// Fill out your copyright notice in the Description page of Project Settings.

#include "CORUIManager.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "CORUI/Config/CORUIManagerSetting.h"
#include "CORUI/Public/UI/Container/CORCanvasContainer.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "UI/Widget/CORWindow.h"
#include "GameFramework/GameModeBase.h"
#include "UEDataBinding.hpp"

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
}

void UCORUIManager::Deinitialize()
{
	
	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::Deinitialize"));
	
	// 移除世界开始的委托
	FGameModeEvents::GameModeInitializedEvent.Remove(UIBeginPlayHandle);

	OnUIEndPlay();

	Super::Deinitialize();
}

void UCORUIManager::OnUIBeginPlay(AGameModeBase* GameMode)
{
	// 世界加载完成后，延迟一帧创建UI
	if (GameMode && GameMode->GetWorld() == GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::InitUI));
	}
}

void UCORUIManager::OnUIEndPlay()
{
	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::OnWorldEndPlay >> Call ts OnDeinit"));
	OnUIManagerDeinit.Broadcast();
	ClearUI();

	// 移除事件绑定
	OnUIManagerInit.Clear();
	OnUIManagerDeinit.Clear();
}

void UCORUIManager::InitUI()
{
	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::InitUI >> InitUI"));
	// 获取根Canvas
	UCanvasPanel* NewRootCanvas = NewObject<UCanvasPanel>(GetWorld(), UCanvasPanel::StaticClass(),
	                                                      FName(TEXT("RootCanvas")));
	if (!NewRootCanvas)
	{
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
		return;
	}

	RootCanvas = NewRootCanvas;

	WidgetPool.SetWorld(GetWorld());

	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUIManager::InitUI >> Call ts OnInit"));
	OnUIManagerInit.Broadcast();
}

void UCORUIManager::ClearUI()
{
	if (RootCanvas.IsValid())
	{
		RootCanvas->ClearChildren();

		if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(GetWorld()))
		{
			Subsystem->RemoveWidget(RootCanvas.Get());
		}
		RootCanvas.Reset();
	}

	WidgetPool.ReleaseAll(true);
	WidgetPool.SetWorld(nullptr);
}

FName UCORUIManager::NativePushUI(UClass* WindowClass, FName WindowName, EUILayerType PushLayer,
                                  FJsObject Callback)
{
	FScopeLock Lock(&DataLock);

	if (!RootCanvas.IsValid())
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
	}
	else
	{
		// 没有在缓存里找到，那么创建一个新的
		WindowInstance = WidgetPool.GetOrCreateInstance(TSubclassOf<UCORWindow>(WindowClass));
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
	WindowName = LayerPanel->PushUI(WindowName, WindowInstance);
	if (WindowName == NAME_None)
	{
		Callback.Action(false, NAME_None, WindowInstance);
		return NAME_None;
	}
	
	// 执行初始化操作
	WindowInstance->NativeOpen();

	NativeUpdateUI();

	Callback.Action(true, WindowName, WindowInstance);

	return WindowName;
}

void UCORUIManager::AsyncPushUI(FSoftClassPath WindowClassPath, FName WindowName, EUILayerType PushLayer,
                                FJsObject Callback)
{
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	auto LoadWeakLambda =
		FStreamableDelegate::
		CreateWeakLambda(this,
		                 [this, WindowClassPath, WindowName, PushLayer, Callback]()
		                 {
			                 if (!IsValid(this))
			                 {
			                 }
			                 UClass* WindowClass = WindowClassPath.ResolveClass();
			                 if (!WindowClass)
			                 {
				                 return;
			                 }
			                 NativePushUI(WindowClass, WindowName, PushLayer, Callback);
		                 });
	TSharedPtr<FStreamableHandle> StreamableHandle = StreamableManager.
		RequestAsyncLoad(WindowClassPath, LoadWeakLambda);
	// 打印失败日志
	if (!StreamableHandle.IsValid() || !StreamableHandle->HasLoadCompleted())
	{
		UE_LOG(LogTemp, Error, TEXT("[CORUI] CORUIManager::AsyncPushUI >> Load Asset is not completed."));
		Callback.Action(false, NAME_None, static_cast<UObject*>(nullptr));
	}
}

void UCORUIManager::SyncPushUI(FSoftClassPath WindowClassPath, FName WindowName, EUILayerType PushLayer,
                               FJsObject Callback)
{
	UClass* WindowClass = WindowClassPath.TryLoadClass<UCORWindow>();
	if (!WindowClass)
	{
		return;
	}
	NativePushUI(WindowClass, WindowName, PushLayer, Callback);
}

void UCORUIManager::PushUI(FString WindowClassPath, FJsObject Callback, FName WindowName,
                           EUILayerType PushLayer, bool bIsAsync)
{
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
	TWeakObjectPtr<UCORCanvasContainer>* LayerPtr = LayerPanels.Find(Layer);
	if (!LayerPtr->IsValid())
	{
		return nullptr;
	}
	return LayerPtr->Get();
}

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
