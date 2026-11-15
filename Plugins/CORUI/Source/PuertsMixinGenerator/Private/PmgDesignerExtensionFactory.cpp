#include "PmgDesignerExtensionFactory.h"

#include "PmgDesignerExtension.h"

FPmgDesignerExtensionFactory::FPmgDesignerExtensionFactory()
{
	
}

FPmgDesignerExtensionFactory::~FPmgDesignerExtensionFactory()
{
	// 登记表由模块持有；析构时断开地址，避免悬空指针被后续使用
	ExtensionRegistry = nullptr;
}

TSharedRef<FDesignerExtension> FPmgDesignerExtensionFactory::CreateDesignerExtension() const
{
	TSharedRef<FPmgDesignerExtension> Extension = MakeShareable(new FPmgDesignerExtension());

	// 登记存活扩展：模块关闭时 CleanupActiveExtensions 才能触达它们。
	// 先清理已失效条目，避免登记表随开关设计师无限增长。
	if (ExtensionRegistry)
	{
		ExtensionRegistry->RemoveAll([](const TWeakPtr<FPmgDesignerExtension>& WeakExtension)
		{
			return !WeakExtension.IsValid();
		});
		ExtensionRegistry->Add(TWeakPtr<FPmgDesignerExtension>(Extension));
	}

	return Extension;
}
