// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * CultivationOpenRealm UI Module
 */
class FCORUIModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// 析构放在 cpp 中定义：TUniquePtr<FAutoConsoleCommand> 需要完整类型
	virtual ~FCORUIModule() override;

private:
	/**
	 * 调试命令 CORUI.Travel <LevelPath>：在 PIE/Game 世界内切图（验证 UI 会话拆建）。
	 * @param Args 命令参数，第一个为目标关卡包路径
	 */
	static void TravelToLevel(const TArray<FString>& Args);

#if !UE_BUILD_SHIPPING
	/**
	 * 调试命令 CORUI.VerifyOwnership：全量 GC 后校验 UI 所有权链（F1 运行期验证）。
	 */
	static void VerifyOwnership();
#endif

	// 调试命令句柄
	TUniquePtr<class FAutoConsoleCommand> TravelCommand;
	TUniquePtr<class FAutoConsoleCommand> VerifyOwnershipCommand;
};
