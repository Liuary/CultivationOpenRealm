#pragma once
#include "IHasDesignerExtensibility.h"

/**
 * 界面扩展工厂类
 * - 创建的扩展会登记到模块的 ActiveExtensions，保证模块关闭时能触达存活扩展
 */
class FPmgDesignerExtensionFactory : public IDesignerExtensionFactory, public TSharedFromThis<FPmgDesignerExtensionFactory>
{
public:
	FPmgDesignerExtensionFactory();
	virtual ~FPmgDesignerExtensionFactory() override;
	// 创建界面扩展
	virtual TSharedRef<FDesignerExtension> CreateDesignerExtension() const override;

	// 设置扩展登记表（由模块在创建工厂时注入，生命周期由模块持有）
	void SetExtensionRegistry(TArray<TWeakPtr<class FPmgDesignerExtension>>* InRegistry) { ExtensionRegistry = InRegistry; }

private:
	// 存活扩展登记表（非拥有）
	TArray<TWeakPtr<class FPmgDesignerExtension>>* ExtensionRegistry = nullptr;
};
