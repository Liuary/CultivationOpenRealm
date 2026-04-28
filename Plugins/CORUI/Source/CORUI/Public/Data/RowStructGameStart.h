#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "UObject/NoExportTypes.h"
#include "RowStructGameStart.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FRowStructGameStart: public FTableRowBase
{
    GENERATED_BODY()

public:
	// ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ID = 0;
	
	// 图片软引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> ImageSoftRef = nullptr;
	
	// 排序
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Sort = 0;
	
	// 描述
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Desc = TEXT("");
};