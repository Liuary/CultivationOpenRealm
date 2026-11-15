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

	/**
	 * 从 JSON 导入数据表；JSON 格式与 GetEntireTableAsJson 一致：{ "RowName": { "Field": value } }
	 * @param DataTable 目标数据表（RowStruct 必须已设置）
	 * @param JsonString JSON 格式的字符串
	 * @return 是否导入成功
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static bool ImportTableFromJson(UDataTable* DataTable, const FString& JsonString);

	/**
	 * 从 CSV 导入数据表（引擎 CSV 格式，RowStruct 必须已设置）
	 * @param DataTable 目标数据表（RowStruct 必须已设置）
	 * @param CsvString 引擎 CSV 格式的字符串
	 * @return 是否无错误
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static bool ImportTableFromCsv(UDataTable* DataTable, const FString& CsvString);

	/**
	 * 导出 CSV（仅编辑器构建有效；运行时返回空串）
	 * @param DataTable 数据表
	 * @return CSV格式的字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static FString ExportTableToCsv(UDataTable* DataTable);

	/**
	 * 导出 JSON（等价 GetEntireTableAsJson，语义更明确的别名）
	 * @param DataTable 数据表
	 * @return JSON格式的字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static FString ExportTableToJson(UDataTable* DataTable);

	/**
	 * 以模板表的 RowStruct 新建一张内存表（transient，不落盘）。
	 * 用于导入工具/往返校验：DataTable 的 RowStruct 在编辑器外不可写，需要由 C++ 创建。
	 * @param TemplateTable 提供 RowStruct 的模板表
	 * @param TableName 新表名（可空）
	 * @return 新建的内存表；失败返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "CORUI|Data|Library")
	static UDataTable* CreateTableFromTemplate(UDataTable* TemplateTable, FName TableName = NAME_None);
	
private:
	// 字段名首字母大写
	static FString CapitalizeFirstLetter(const FString& Str);
    
	// 递归转换 JsonObject 字段名
	static void ConvertJsonFieldNamesToPascalCase(TSharedPtr<FJsonObject>& JsonObject);
};