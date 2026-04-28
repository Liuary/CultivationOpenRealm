// Fill out your copyright notice in the Description page of Project Settings.


#include "UILayerTypeZOrderConfigData.h"

void UUILayerTypeZOrderConfigData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 缓存当前层级数据
	TMap<EUILayerType, int32> LayerZOrders;
	for (const auto& Config : LayerZOrderConfig)
	{
		LayerZOrders.Add(Config.Key, Config.Value);
	}

	// 清空旧数据
	LayerZOrderConfig.Empty();
	// 重新读取枚举数据
	for (const auto &LayerType : TEnumRange<EUILayerType>())
	{
		// 先检查缓存
		if (LayerZOrders.Contains(LayerType))
		{
			LayerZOrderConfig.Add(LayerType, LayerZOrders[LayerType]);
		} else
		{
			LayerZOrderConfig.Add(LayerType, static_cast<int32>(LayerType) * 100);
		}
	}
	
}
