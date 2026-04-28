// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsEnv.h"
#include "Engine/GameInstance.h"
#include "CorGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class CULTIVATIONOPENREALM_API UCorGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	
	virtual void OnStart() override;
	
	virtual void Shutdown() override;

protected:
	// TODO 将加载 ts 虚拟机放进一个单的插件，继承自 UGameInstanceSubsystem
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CorGameInstance")
	FString ModuleName = "CorStart";
private:
	// JS 环境
	TSharedPtr<puerts::FJsEnv> JsEnv;
};
