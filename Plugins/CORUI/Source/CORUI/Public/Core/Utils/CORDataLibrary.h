#pragma once

#include "CoreMinimal.h"
#include "CORDataLibrary.generated.h"

// 继承自蓝图函数库的工具类
UCLASS()
class UCORDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
    
public:
	/**
	 * 获取数据表的行数据（JSON格式）
	 * @param DataTable 数据表
	 * @param RowName 行名
	 * @return JSON格式的字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static FString GetTableRowAsJson(UDataTable* DataTable, FName RowName);

	/**
	 * 获取数据表的全部行数据（JSON格式）
	 * @param DataTable 数据表
	 * @return JSON格式的字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static FString GetEntireTableAsJson(UDataTable* DataTable);
	
private:
	// 字段名首字母大写
	static FString CapitalizeFirstLetter(const FString& Str);
    
	// 递归转换 JsonObject 字段名
	static void ConvertJsonFieldNamesToPascalCase(TSharedPtr<FJsonObject>& JsonObject);
};