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
	// 统一关闭边界：先让 UI 在 JS 仍存活时关闭窗口，再销毁 JS 环境
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCORUIManager* UIManager = GameInstance->GetSubsystem<UCORUIManager>())
		{
			UIManager->ShutdownUI();
		}
	}

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

bool UJsEnvManagerSubsystem::IsJsEnvStarted() const
{
	return JsEnv.IsValid();
}

void UJsEnvManagerSubsystem::StartJsEnv()
{
	// 幂等：重复的世界/GameMode 初始化不再替换已有环境（替换会让旧 UI 回调落空）
	if (JsEnv.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("UJsEnvManagerSubsystem::StartJsEnv: already started, skip"));
		return;
	}

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
	// 与 CORUI 的 next-tick InitUI 竞争会丢掉唯一一次 OnUIManagerInit：
	// 这里同步启动 JS（同步执行 CorStart.Start -> BindCppEvent），UI 的 next-tick 初始化必然晚于事件绑定。
	if (GameMode && GameMode->GetWorld() == GetWorld())
	{
		StartJsEnv();
	}
}
