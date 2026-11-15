// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Streamable/CORStreamableManager.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "UEDataBinding.hpp"

void UCORStreamableManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCORStreamableManager::Deinitialize()
{
	// 清除全部尚未清除的Handle
	ReleaseAll();
	
	Super::Deinitialize();
}

int32 UCORStreamableManager::RequestAsyncLoadList(TArray<CORSoftObjectPath> Paths, FJsObject Callback)
{
	TArray<FSoftObjectPath> SoftPaths;
	int32 InvalidPathCount = 0;
	for (const auto& Path : Paths)
	{
		if (Path.IsValid())
		{
			SoftPaths.Add(FSoftObjectPath(Path.GetPath()));
		}
		else
		{
			++InvalidPathCount;
		}
	}

	// 空列表 / 含无效路径：整体拒绝（返回 0），避免静默只加载“有效子集”
	if (SoftPaths.Num() == 0 || InvalidPathCount > 0)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("[CORUI] UCORStreamableManager::RequestAsyncLoadList >> rejected: %d valid, %d invalid"),
		       SoftPaths.Num(), InvalidPathCount);
		int32 InvalidHandleID = 0;
		Callback.Action(false, InvalidHandleID);
		return 0;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	const int32 NewHandleID = GetNewHandleID();
	auto LoadWeakLambda =
		FStreamableDelegate::
		CreateWeakLambda(this, [this, Callback, NewHandleID]()
		{
			OnLoadCompleted(NewHandleID, Callback);
		});

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(SoftPaths, LoadWeakLambda);
	if (!Handle.IsValid())
	{
		// 加载器拒绝：不登记句柄，返回无效 ID
		Callback.Action(false, 0);
		return 0;
	}

	Handles.Add(NewHandleID, Handle);
	return NewHandleID;
}

int32 UCORStreamableManager::RequestAsyncLoad(CORSoftObjectPath Path, FJsObject Callback)
{
	// 无效路径：拒绝请求，不分配句柄 ID
	if (!Path.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CORUI] UCORStreamableManager::RequestAsyncLoad >> invalid path"));
		Callback.Action(false, 0);
		return 0;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	const int32 NewHandleID = GetNewHandleID();
	auto LoadWeakLambda =
		FStreamableDelegate::
		CreateWeakLambda(this, [this, Callback, NewHandleID]()
		{
			OnLoadCompleted(NewHandleID, Callback);
		});

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(FSoftObjectPath(Path.GetPath()),
	                                                                         LoadWeakLambda);
	if (!Handle.IsValid())
	{
		Callback.Action(false, 0);
		return 0;
	}

	Handles.Add(NewHandleID, Handle);
	return NewHandleID;
}

void UCORStreamableManager::OnLoadCompleted(int32 InHandleID, FJsObject Callback)
{
	bool bSuccess = false;

	const TSharedPtr<FStreamableHandle>* Found = Handles.Find(InHandleID);
	if (!Found)
	{
		// 条目已被 ReleaseHandle/CancelHandle 移除：调用方已经收到终态，不再补发第二次结果
		return;
	}

	if (Found->IsValid())
	{
		TArray<UObject*> LoadedAssets;
		Found->Get()->GetLoadedAssets(LoadedAssets);
		bSuccess = Found->Get()->HasLoadCompleted() && LoadedAssets.Num() > 0;
	}

	// 失败请求不保留条目：句柄 map 不会被无效请求撑大
	if (!bSuccess)
	{
		Handles.Remove(InHandleID);
	}

	// 结构化终态：成功与否 + 句柄 ID
	Callback.Action(bSuccess, InHandleID);
}

void UCORStreamableManager::ReleaseHandle(int32 TargetHandleID)
{
	if (TargetHandleID <= 0)
	{
		return;
	}

	if (TSharedPtr<FStreamableHandle> Handle = Handles.FindRef(TargetHandleID))
	{
		Handle->ReleaseHandle();
	}

	// 无论句柄是否有效，条目都移除
	Handles.Remove(TargetHandleID);
}

void UCORStreamableManager::CancelHandle(int32 TargetHandleID)
{
	if (TargetHandleID <= 0)
	{
		return;
	}

	if (TSharedPtr<FStreamableHandle> Handle = Handles.FindRef(TargetHandleID))
	{
		Handle->CancelHandle();
	}

	Handles.Remove(TargetHandleID);
}

void UCORStreamableManager::ReleaseAll()
{
	for (const auto& Pair : Handles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->ReleaseHandle();
		}
	}

	Handles.Empty();
}

int32 UCORStreamableManager::GetNewHandleID()
{
	return ++HandleID;
}


// ///////// puerts导出
// UsingUClass(UObject)
// UsingUClass(UCORStreamableManager)
//
// UsingCppType(FCORSoftObjectPath)
// UsingCppType(FOnStreamableComplete)
// UsingCppType(TArray<FCORSoftObjectPath>)
//
// struct AutoRegisterForUCORStreamableManager
// {
// 	AutoRegisterForUCORStreamableManager()
// 	{
//
// 		puerts::DefineClass<FOnStreamableComplete>()
// 			.Register();
//
// 		puerts::DefineClass<TArray<FCORSoftObjectPath>>()
// 			.Register();
// 		
// 		puerts::DefineClass<UCORStreamableManager>()
// 			.Method("RequestAsyncLoadList", MakeFunction(&UCORStreamableManager::RequestAsyncLoadList))
// 			.Method("RequestAsyncLoad", MakeFunction(&UCORStreamableManager::RequestAsyncLoad))
// 			.Register();
// 	}
// };
//
// AutoRegisterForUCORStreamableManager _AutoRegisterForUCORStreamableManager_;
