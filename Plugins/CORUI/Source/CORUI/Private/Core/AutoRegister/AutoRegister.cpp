#include "Core/AutoRegister/AutoRegister.h"

#include "CORUIManager.h"
#include "Core/Streamable/CORStreamableManager.h"
#include "UI/Utils/WeakObjectWrapper.h"

UsingUClass(UObject)

UsingUClass(UDataTable)

UsingUClass(UCORUIManager)

UsingUClass(UCORStreamableManager)

UsingCppType(WeakObjectWrapper)

UsingCppType(CORSoftObjectPath)

UsingUStruct(TSoftObjectPtr<UObject>)

UsingContainer(TArray<CORSoftObjectPath>)

struct CORCommonAutoRegister
{
	CORCommonAutoRegister()
	{

		puerts::DefineClass<CORSoftObjectPath>()
			.Constructor(CombineConstructors(
				MakeConstructor(CORSoftObjectPath, const FString&),
				MakeConstructor(CORSoftObjectPath)
			))
			.Method("SetPath", MakeFunction(&CORSoftObjectPath::SetPath))
			.Method("GetAsset", MakeFunction(&CORSoftObjectPath::GetAsset))
			.Method("GetPath", MakeFunction(&CORSoftObjectPath::GetPath))
			.Method("IsValid", MakeFunction(&CORSoftObjectPath::IsValid))
			.Method("Load", MakeFunction(&CORSoftObjectPath::Load))
			.Function("MakeSoftObjectPath", MakeFunction(&CORSoftObjectPath::MakeSoftObjectPath))
			.Register();

		puerts::DefineClass<WeakObjectWrapper>()
			.Constructor(CombineConstructors(
				MakeConstructor(WeakObjectWrapper, UObject*),
				MakeConstructor(WeakObjectWrapper)
			))
			.Method("Set", MakeFunction(&WeakObjectWrapper::Set))
			.Method("Get", MakeFunction(&WeakObjectWrapper::Get))
			.Method("IsValid", MakeFunction(&WeakObjectWrapper::IsValid))
			.Method("Reset", MakeFunction(&WeakObjectWrapper::Reset))
			.Function("MakeWeakObjectPtr", MakeFunction(&WeakObjectWrapper::MakeWeakObjectPtr))
			.Register();

		RegisterTArray(CORSoftObjectPath);

		puerts::DefineClass<UCORStreamableManager>()
			.Method("RequestAsyncLoadList", MakeFunction(&UCORStreamableManager::RequestAsyncLoadList))
			.Method("RequestAsyncLoad", MakeFunction(&UCORStreamableManager::RequestAsyncLoad))
			.Register();
	}
};

CORCommonAutoRegister _CORCommonAutoRegister_;
