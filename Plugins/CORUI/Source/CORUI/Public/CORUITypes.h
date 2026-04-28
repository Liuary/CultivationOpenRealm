#pragma once

#include "CoreMinimal.h"
#include "CORUITypes.generated.h"

/**
 * UI层级
 */
UENUM(BlueprintType)
enum class EUILayerType : uint8
{
	// 未定义标记(标记首位)
	None,
	// 背景层
	BackGround,
	// 常规层
	Normal,
	// 顶层
	Top,
	// 消息层（仅用于显示消息, 不支持关闭）
	Message,
	// 调试层（标记末位，仅用于开发和调试, 不会用于正式环境）
	Debug
};

ENUM_RANGE_BY_FIRST_AND_LAST(EUILayerType, EUILayerType::None, EUILayerType::Debug)