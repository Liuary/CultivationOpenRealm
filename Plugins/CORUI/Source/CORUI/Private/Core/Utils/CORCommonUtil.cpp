#include "Core/Utils/CORCommonUtil.h"

FString UCORCommonUtil::GetSoftObjectPath(const TSoftObjectPtr<UObject>& SoftObjectPtr)
{
	return SoftObjectPtr.ToString();
}
