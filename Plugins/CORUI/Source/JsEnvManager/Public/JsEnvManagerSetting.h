// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JsEnvManagerSetting.generated.h"

/**
 * Js虚拟机管理器的配置文件
 */
UCLASS(Config=JsEnvManagerSetting, defaultconfig, meta=(DisplayName="JsEnvManagerSetting"))
class JSENVMANAGER_API UJsEnvManagerSetting : public UObject
{
	GENERATED_BODY()
public:
	// 启动模块名
	UPROPERTY(Config, EditAnywhere, Category="JsEnvManagerSetting")
	FString StartModuleName = "CorStart";

	// 启动端口
	UPROPERTY(Config, EditAnywhere, Category="JsEnvManagerSetting")
	int32 DebugPort = 8080;

	// 开启等待连接调试
	UPROPERTY(Config, EditAnywhere, Category="JsEnvManagerSetting")
	bool bWaitDebug = false;
};
