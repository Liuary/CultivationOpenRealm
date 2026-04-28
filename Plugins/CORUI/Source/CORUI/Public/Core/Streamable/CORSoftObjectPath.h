#pragma once

// #include "CORSoftObjectPath.generated.h"

/**
 * COR的SoftObjectPath，简单封装了FSoftObjectPath，便于ts调用
 */
// USTRUCT()
struct CORSoftObjectPath
{
	// GENERATED_BODY()

	CORSoftObjectPath() = default;
	CORSoftObjectPath(const FSoftObjectPath& InSoftObjectPath) : SoftObjectPath(InSoftObjectPath) {}
	CORSoftObjectPath(const FString& InSoftObjectPath) : SoftObjectPath(InSoftObjectPath) {}
	~CORSoftObjectPath() = default;

	// 重载赋值运算符
	CORSoftObjectPath& operator=(const FSoftObjectPath& InSoftObjectPath);
	// 重载等于运算符
	bool operator==(const CORSoftObjectPath& InSoftObjectPath) const { return SoftObjectPath == InSoftObjectPath.SoftObjectPath; }

	// 获取原始路径
	FString GetPath() const { return SoftObjectPath.ToString(); }

	// 设置路径
	void SetPath(const FString& InSoftObjectPath) { SoftObjectPath = FSoftObjectPath(InSoftObjectPath); }

	// 是否有效
	bool IsValid() const { return SoftObjectPath.IsValid(); }

	// 获取资产
	UObject* GetAsset() const { return SoftObjectPath.ResolveObject(); }
	
	// 同步加载资产
	UObject* Load() const;
	

	// 创建一个新的 SoftObjectPath
	static CORSoftObjectPath MakeSoftObjectPath(const FString& InSoftObjectPath)
	{
		return CORSoftObjectPath(InSoftObjectPath);
	}
	
	/**
	 *根据一个 TSoftObjectPtr<UObject> 创建一个新的 SoftObjectPath
	 */
	static CORSoftObjectPath MakeSoftObjectPathBySoftUObject(const TSoftObjectPtr<UObject>& InSoftObjectPtr)
	{
		return CORSoftObjectPath(InSoftObjectPtr.ToSoftObjectPath());
	}
	

	// UPROPERTY()
	FSoftObjectPath SoftObjectPath;
};

