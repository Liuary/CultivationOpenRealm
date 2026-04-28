// Fill out your copyright notice in the Description page of Project Settings.


#include "CorGameInstance.h"

void UCorGameInstance::OnStart()
{
	Super::OnStart();

	// // 初始化JS环境
	// JsEnv = MakeShared<puerts::FJsEnv>();
	// TArray<TPair<FString, UObject*>> Arguments;
	// Arguments.Add(TPair<FString, UObject*>(TEXT("GameInstance"), this));
	// JsEnv->Start(ModuleName, Arguments);
}

void UCorGameInstance::Shutdown()
{
	// // 关闭JS环境
	// JsEnv.Reset();
	
	Super::Shutdown();
}
