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
	 * @param Paths 资源路径数组
	 * @param Callback 加载完成回调 (bool 是否成功, int32 句柄ID)
	 * @return 管理器分配的句柄ID；请求被拒绝时返回 0
	 */
	int32 RequestAsyncLoadList(TArray<CORSoftObjectPath> Paths, FJsObject Callback);
	/**
	 * 异步加载资源
	 * @param Path 资源路径
	 * @param Callback 加载完成回调 (bool 是否成功, int32 句柄ID)
	 * @return 管理器分配的句柄ID；请求被拒绝时返回 0
	 */
	int32 RequestAsyncLoad(CORSoftObjectPath Path, FJsObject Callback);

	/**
	 * 释放句柄（无论句柄是否仍然有效都会移除条目）
	 * @param TargetHandleID 句柄ID
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Streamable")
	void ReleaseHandle(int32 TargetHandleID);

	/**
	 * 取消句柄（取消后不再回调成功，条目移除）
	 * @param TargetHandleID 句柄ID
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Streamable")
	void CancelHandle(int32 TargetHandleID);

	/**
	 * 释放全部句柄
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Streamable")
	void ReleaseAll();

private:
	// 加载完成（统一成功/失败终态，并清理失败条目）
	void OnLoadCompleted(int32 InHandleID, FJsObject Callback);

	// 异步加载的资源句柄缓存，用int32传递，以免TS持有引用
	TMap<int32, TSharedPtr<FStreamableHandle>> Handles;
	// 递增句柄ID
	int32 HandleID = 0;
	// 获取一个新的递增句柄ID
	int32 GetNewHandleID();
};
