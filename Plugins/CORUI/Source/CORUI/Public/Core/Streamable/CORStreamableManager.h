// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CORSoftObjectPath.h"
#include "JsObject.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CORStreamableManager.generated.h"


/**
 * COR的流式加载管理器
 */
UCLASS()
class CORUI_API UCORStreamableManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * 异步加载资源数组
	 * @param Target 目标 Object 
	 * @param Paths 资源路径数组
	 * @param Callback 加载完成回调
	 * @return 管理器分配的句柄ID
	 */
	int32 RequestAsyncLoadList(TArray<CORSoftObjectPath> Paths, FJsObject Callback);
	/**
	 * 异步加载资源
	 * @param Target 目标 Object 
	 * @param Path 资源路径
	 * @param Callback 加载完成回调
	 * @return 管理器分配的句柄ID
	 */
	int32 RequestAsyncLoad(CORSoftObjectPath Path, FJsObject Callback);

	/**
	 * 释放句柄
	 * @param TargetHandleID 句柄ID
	 */
	void ReleaseHandle(int32 TargetHandleID);

private:
	// 异步加载的资源句柄缓存，用int32传递，以免TS持有引用
	TMap<int32, TSharedPtr<FStreamableHandle>> Handles;
	// 递增句柄ID
	int32 HandleID = 0;
	// 获取一个新的递增句柄ID
	int32 GetNewHandleID();
};
