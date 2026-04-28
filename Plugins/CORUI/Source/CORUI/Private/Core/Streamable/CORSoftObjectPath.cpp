#include "Core/Streamable/CORSoftObjectPath.h"
#include "UEDataBinding.hpp"

// ///////// puerts导出
// UsingUClass(UObject)
// UsingCppType(FCORSoftObjectPath);
//
// struct AutoRegisterForFCORSoftObjectPath
// {
// 	AutoRegisterForFCORSoftObjectPath()
// 	{
// 		puerts::DefineClass<FCORSoftObjectPath>()
// 			.Constructor(CombineConstructors(
// 				MakeConstructor(FCORSoftObjectPath, const FString&),
// 				MakeConstructor(FCORSoftObjectPath)
// 			))
// 			.Method("SetPath", MakeFunction(&FCORSoftObjectPath::SetPath))
// 			.Method("GetAsset", MakeFunction(&FCORSoftObjectPath::GetAsset))
// 			.Method("GetPath", MakeFunction(&FCORSoftObjectPath::GetPath))
// 			.Method("IsValid", MakeFunction(&FCORSoftObjectPath::IsValid))
// 			.Function("MakeSoftObjectPath", MakeFunction(&FCORSoftObjectPath::MakeSoftObjectPath))
// 			.Register();
// 	}
// };

// AutoRegisterForFCORSoftObjectPath _AutoRegisterForFCORSoftObjectPath_;
CORSoftObjectPath& CORSoftObjectPath::operator=(const FSoftObjectPath& InSoftObjectPath)
{
	SoftObjectPath = InSoftObjectPath;
	return *this;
}

UObject* CORSoftObjectPath::Load() const
{
	auto Obj = SoftObjectPath.ResolveObject();
	if (!Obj)
	{
		Obj = SoftObjectPath.TryLoad();
	}
	return Obj;
}
