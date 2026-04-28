// Fill out your copyright notice in the Description page of Project Settings.


#include "JsEnvManagerSubsystem.h"

#include "CORUIManager.h"
#include "JsEnvManagerSetting.h"
#include "GameFramework/GameModeBase.h"

void UJsEnvManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

	// 声明对UI管理器的依赖
	// 声明依赖关系
	Collection.InitializeDependency(UCORUIManager::StaticClass());
	
	Super::Initialize(Collection);

	JsEnvBeginPlayHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject(this, &UJsEnvManagerSubsystem::OnJsEnvBeginPlay);
}

void UJsEnvManagerSubsystem::Deinitialize()
{
	// 关闭JS环境
	JsEnv.Reset();
	
	// 移除世界开始的委托
	FGameModeEvents::GameModeInitializedEvent.Remove(JsEnvBeginPlayHandle);
	
	Super::Deinitialize();
}

TSharedPtr<puerts::FJsEnv> UJsEnvManagerSubsystem::GetJsEnv()
{
	return JsEnv;
}

void UJsEnvManagerSubsystem::StartJsEnv()
{
	// 获取设置
	const UJsEnvManagerSetting* Setting = GetDefault<UJsEnvManagerSetting>();

	if (!IsValid(Setting))
	{
		UE_LOG(LogTemp, Error, TEXT("UJsEnvManagerSubsystem::Initialize: Invalid Setting"));
		return;
	}

	// 初始化JS环境
	// 设置端口
	JsEnv = MakeShared<puerts::FJsEnv>(std::make_unique<puerts::DefaultJSModuleLoader>(TEXT("JavaScript")), std::make_shared<puerts::FDefaultLogger>(), Setting->DebugPort);

	if (!JsEnv.IsValid())
	{
		return;
	}
	
	// 判断是否需要等待
	if (Setting->bWaitDebug)
	{
		JsEnv->WaitDebugger();
	}
	
	TArray<TPair<FString, UObject*>> Arguments;
	Arguments.Add(TPair<FString, UObject*>(TEXT("GameInstance"), this));
	JsEnv->Start(Setting->StartModuleName, Arguments);
}

void UJsEnvManagerSubsystem::OnJsEnvBeginPlay(AGameModeBase* GameMode)
{
	// 世界加载完成后，延迟一帧创建JsEnv
	if (GameMode && GameMode->GetWorld() == GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::StartJsEnv));
	}
}
