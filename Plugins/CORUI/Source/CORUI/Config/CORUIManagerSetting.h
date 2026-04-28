// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UILayerTypeZOrderConfigData.h"
#include "UObject/Object.h"
#include "CORUIManagerSetting.generated.h"

/**
 * UCORUI管理器配置
 */
UCLASS(Config=UCORUIManager, defaultconfig, meta=(DisplayName="UCORUIManager"))
class CORUI_API UCORUIManagerSetting : public UObject
{
	GENERATED_BODY()
public:
	// 层级配置文件（DA）
	UPROPERTY(Config, EditAnywhere, Category="UCORUIManager")
	TSoftObjectPtr<UUILayerTypeZOrderConfigData> LayerTypeZOrderConfigPath;

	// 默认层级ZOrder倍率（默认层级ZOrder = 枚举值 * 倍率）
	UPROPERTY(Config, EditAnywhere, Category="UCORUIManager")
	int32 DefaultZOrderMultiplier = 100;
};
