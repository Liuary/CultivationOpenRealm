#pragma once
#include "CoreMinimal.h"
#include "CORCommonUtil.generated.h"

// 继承自蓝图函数库的工具类
UCLASS()
class UCORCommonUtil : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:
    // 获取 TSoftObjectPtr<UObject> 的 Path
    UFUNCTION(BlueprintCallable, Category = "CORUI|Utils")
    static FString GetSoftObjectPath(const TSoftObjectPtr<UObject>& SoftObjectPtr);
};