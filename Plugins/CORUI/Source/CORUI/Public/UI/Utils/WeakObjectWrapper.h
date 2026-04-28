#pragma once
#include "CoreMinimal.h"
// #include "WeakObjectWrapper.generated.h"

/**
 * 封装WeakObjectPtr的包装类，以便在蓝图和脚本使用弱指针
 */
// USTRUCT(BlueprintType)
struct WeakObjectWrapper
{
	// GENERATED_BODY()
public:
	WeakObjectWrapper();
	WeakObjectWrapper(UObject* NewPtr);
	WeakObjectWrapper(TWeakObjectPtr<UObject> NewPtr);
	~WeakObjectWrapper();
	// 制造一个弱指针
	static WeakObjectWrapper MakeWeakObjectPtr(UObject* NewPtr);
	// 设置指向新的对象
	void Set(UObject* NewPtr);
	// 获取原始对象指针（无效返回nullptr）
	UObject* Get();
	// 释放指针
	void Reset();
	// 检查指针有效性
	bool IsValid();
private:
	// 存储弱指针
	TWeakObjectPtr<UObject> WeakUpPtr;
};
