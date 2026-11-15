// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsEnv.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JsEnvManagerSubsystem.generated.h"

/**
 * JS虚拟机管理器
 */
UCLASS(Blueprintable)
class JSENVMANAGER_API UJsEnvManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// 获取 Js 虚拟机
	TSharedPtr<puerts::FJsEnv> GetJsEnv();

	// 启动Js虚拟机（幂等：已启动时直接返回）
	void StartJsEnv();

	// Js 环境是否已启动
	bool IsJsEnvStarted() const;
	
private:
	// JS 环境
	TSharedPtr<puerts::FJsEnv> JsEnv;
	
protected:
	
	void OnJsEnvBeginPlay(AGameModeBase* GameMode);
	// Js 环境启动时的委托
	FDelegateHandle JsEnvBeginPlayHandle;
};
