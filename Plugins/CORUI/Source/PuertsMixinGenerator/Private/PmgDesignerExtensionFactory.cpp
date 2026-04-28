#include "PmgDesignerExtensionFactory.h"

#include "PmgDesignerExtension.h"

FPmgDesignerExtensionFactory::FPmgDesignerExtensionFactory()
{
	
}

FPmgDesignerExtensionFactory::~FPmgDesignerExtensionFactory()
{
	
}

TSharedRef<FDesignerExtension> FPmgDesignerExtensionFactory::CreateDesignerExtension() const
{
	return MakeShareable(new FPmgDesignerExtension());
}
