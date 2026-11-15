// Copyright Epic Games, Inc. All Rights Reserved.

#include "CORUI.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "CORUIManager.h"
#include "Subsystems/Subsystem.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#endif
#include "CORUI/Config/CORUIManagerSetting.h"

#define LOCTEXT_NAMESPACE "FCORUIModule"

namespace
{
	// 命令侧统一的世界查找：只认运行中的 PIE/Game 世界（编辑器世界切图走编辑器自身流程）
	UWorld* FindRunningWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return nullptr;
	}
}

FCORUIModule::~FCORUIModule() = default;

void FCORUIModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// 注册设置
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "UCORUIManager",
										 FText::FromString("COR UIManager Settings"),
										 FText::FromString("Open COR UIManager Settings Settings"),
										 GetMutableDefault<UCORUIManagerSetting>());
	}
#endif

	// 调试命令：在 PIE/Game 世界内切图，用于验证 UI 会话拆建（同 GameInstance 换图）
	// 仅非 Shipping 构建注册，避免把调试入口和测试关卡带进发行包
#if !UE_BUILD_SHIPPING
	TravelCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("CORUI.Travel"),
		TEXT("Travel to a level in the current PIE/Game world (dev). Usage: CORUI.Travel /Game/Map/TestTravelMap"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&FCORUIModule::TravelToLevel));

	// F1 验证：跑一次全量 GC，再检查根面板/层容器/对象池是否仍被正确持有
	VerifyOwnershipCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("CORUI.VerifyOwnership"),
		TEXT("Run a full GC, then verify CORUI UI ownership chain (root/layers/pool). Dev only."),
		FConsoleCommandDelegate::CreateStatic(&FCORUIModule::VerifyOwnership));
#endif
}

void FCORUIModule::TravelToLevel(const TArray<FString>& Args)
{
	if (Args.Num() < 1 || Args[0].IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUI.Travel >> usage: CORUI.Travel <LevelPath>"));
		return;
	}

	// 只在运行中的 PIE/Game 世界切图（编辑器世界切图走编辑器自身流程）
	UWorld* TargetWorld = FindRunningWorld();

	if (!TargetWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUI.Travel >> no PIE/Game world found."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CORUI] CORUI.Travel >> travelling to '%s'."), *Args[0]);
	UGameplayStatics::OpenLevel(TargetWorld, FName(*Args[0]));
}

#if !UE_BUILD_SHIPPING
void FCORUIModule::VerifyOwnership()
{
	UWorld* TargetWorld = FindRunningWorld();
	if (!TargetWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUI.VerifyOwnership >> no PIE/Game world found."));
		// 无世界不是"验证失败"，是没有可验证对象：显式 SKIP，保证脚本能拿到结论行
		UE_LOG(LogTemp, Display, TEXT("[CORUI][Owner] RESULT=SKIP no-world"));
		return;
	}

	UGameInstance* GameInstance = TargetWorld->GetGameInstance();
	UCORUIManager* Manager = GameInstance ? GameInstance->GetSubsystem<UCORUIManager>() : nullptr;
	if (!Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] CORUI.VerifyOwnership >> UCORUIManager subsystem not found."));
		UE_LOG(LogTemp, Display, TEXT("[CORUI][Owner] RESULT=SKIP no-manager"));
		return;
	}

	FString Report;
	const bool bPass = Manager->VerifyOwnershipAfterGC(Report);

	// SKIP（UI 未就绪）与 FAIL 必须区分，否则自动化会把"没验证"当成"验证失败"
	const TCHAR* Result = bPass ? TEXT("PASS")
	                      : (Report.StartsWith(TEXT("SKIP")) ? TEXT("SKIP") : TEXT("FAIL"));

	// 单行汇总，便于脚本/MCP 抓取：PASS/FAIL/SKIP + 分项 + 计数
	UE_LOG(LogTemp, Display, TEXT("[CORUI][Owner] %s"), *Report);
	UE_LOG(LogTemp, Display, TEXT("[CORUI][Owner] RESULT=%s"), Result);
}
#endif

void FCORUIModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	TravelCommand.Reset();
	VerifyOwnershipCommand.Reset();

	// 注销设置
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "UCORUIManager");
	}
#endif
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCORUIModule, CORUI)