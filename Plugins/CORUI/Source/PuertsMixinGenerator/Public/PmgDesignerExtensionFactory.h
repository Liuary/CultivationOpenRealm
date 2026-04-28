#pragma once
#include "IHasDesignerExtensibility.h"

/**
 * 界面扩展工厂类
 */
class FPmgDesignerExtensionFactory : public IDesignerExtensionFactory, public TSharedFromThis<FPmgDesignerExtensionFactory>
{
public:
	FPmgDesignerExtensionFactory();
	virtual ~FPmgDesignerExtensionFactory() override;
	// 创建界面扩展
	virtual TSharedRef<FDesignerExtension> CreateDesignerExtension() const override;
};
