#include "UI/Utils/WeakObjectWrapper.h"

#include "ScriptBackend.hpp"
#include "UEDataBinding.hpp"


WeakObjectWrapper::WeakObjectWrapper()
{
}

WeakObjectWrapper::WeakObjectWrapper(UObject* NewPtr)
{
	Set(NewPtr);
}

WeakObjectWrapper::WeakObjectWrapper(TWeakObjectPtr<UObject> NewPtr)
{
	WeakUpPtr = NewPtr;
}

WeakObjectWrapper::~WeakObjectWrapper()
{
	Reset();
}

WeakObjectWrapper WeakObjectWrapper::MakeWeakObjectPtr(UObject* NewPtr)
{
	WeakObjectWrapper NewWeakObjectWrapper;
	NewWeakObjectWrapper.Set(NewPtr);
	return NewWeakObjectWrapper;
}

void WeakObjectWrapper::Set(UObject* NewPtr)
{
	Reset();
	WeakUpPtr = NewPtr;
}

UObject* WeakObjectWrapper::Get()
{
	if (!IsValid())
	{
		return nullptr;
	}
	return WeakUpPtr.Get();
}

void WeakObjectWrapper::Reset()
{
	if (WeakUpPtr.IsValid())
	{
		WeakUpPtr.Reset();
	}
}

bool WeakObjectWrapper::IsValid()
{
	return WeakUpPtr.IsValid();
}


// ///////// puerts导出
// UsingUClass(UObject)
// UsingUStruct(WeakObjectWrapper);
//
// struct AutoRegisterForFWeakObjectWrapper
// {
// 	AutoRegisterForFWeakObjectWrapper()
// 	{
// 		puerts::DefineClass<WeakObjectWrapper>()
// 			.Constructor(CombineConstructors(
// 				MakeConstructor(WeakObjectWrapper, UObject*),
// 				MakeConstructor(WeakObjectWrapper)
// 			))
// 			.Method("Set", MakeFunction(&WeakObjectWrapper::Set))
// 			.Method("Get", MakeFunction(&WeakObjectWrapper::Get))
// 			.Method("IsValid", MakeFunction(&WeakObjectWrapper::IsValid))
// 			.Method("Reset", MakeFunction(&WeakObjectWrapper::Reset))
// 			.Function("MakeWeakObjectPtr", MakeFunction(&WeakObjectWrapper::MakeWeakObjectPtr))
// 			.Register();
// 	}
// };
//
// AutoRegisterForFWeakObjectWrapper _AutoRegisterForFWeakObjectWrapper_;