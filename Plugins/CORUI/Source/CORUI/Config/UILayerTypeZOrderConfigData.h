// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CORUIManager.h"
#include "Engine/DataAsset.h"
#include "UILayerTypeZOrderConfigData.generated.h"

/**
 * UI层级枚举ZOrder配置数据
 */
UCLASS()
class CORUI_API UUILayerTypeZOrderConfigData : public UDataAsset
{
	GENERATED_BODY()

public:

	// 层级配置表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	TMap<EUILayerType, int32> LayerZOrderConfig;

	// 属性变更时回调
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
