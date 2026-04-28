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
	for (const auto& [key, value] : Handles)
	{
		if (value.IsValid())
		{
			value->ReleaseHandle();
		}
	}

	Handles.Empty();
	
	Super::Deinitialize();
}

int32 UCORStreamableManager::RequestAsyncLoadList(TArray<CORSoftObjectPath> Paths, FJsObject Callback)
{
	TArray<FSoftObjectPath> SoftPaths;
	for (const auto& Path : Paths)
	{
		SoftPaths.Add(Path.GetPath());
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	auto LoadWeakLambda =
		FStreamableDelegate::
		CreateWeakLambda(this, [Callback]()
		{
			Callback.Action();
		});

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(SoftPaths, LoadWeakLambda);
	int32 NewHandleID = GetNewHandleID();
	Handles.Add(NewHandleID, Handle);
	return NewHandleID;
}

int32 UCORStreamableManager::RequestAsyncLoad(CORSoftObjectPath Path, FJsObject Callback)
{
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	auto LoadWeakLambda =
		FStreamableDelegate::
		CreateWeakLambda(this, [Callback]()
		{
			Callback.Action();
		});

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(Path.GetPath(), LoadWeakLambda);
	int32 NewHandleID = GetNewHandleID();
	Handles.Add(NewHandleID, Handle);
	return NewHandleID;
}

void UCORStreamableManager::ReleaseHandle(int32 TargetHandleID)
{
	auto Handle = Handles.Find(TargetHandleID);
	if (Handle && Handle->IsValid())
	{
		Handle->Get()->ReleaseHandle();
		Handles.Remove(TargetHandleID);
	}
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