// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "CORImage.generated.h"

/**
 * 
 */
UCLASS()
class CORUI_API UCORImage : public UImage
{
	GENERATED_BODY()
	
public:
	// 通过路径设置图片
	UFUNCTION(BlueprintCallable, Category = "COR|Image")
	void SetImagePath(FString Path, bool bMatchSize = true, bool AsyncLoad = true);
};
