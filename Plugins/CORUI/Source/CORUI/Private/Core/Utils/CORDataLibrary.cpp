#include "Core/Utils/CORDataLibrary.h"
#include "JsonObjectConverter.h"

FString UCORDataLibrary::CapitalizeFirstLetter(const FString& Str)
{
    if (Str.IsEmpty())
    {
        return Str;
    }
    
    // 首字母大写，其余保持不变
    FString Result = Str;
    Result[0] = FChar::ToUpper(Result[0]);
    return Result;
}

void UCORDataLibrary::ConvertJsonFieldNamesToPascalCase(TSharedPtr<FJsonObject>& JsonObject)
{
    if (!JsonObject.IsValid())
    {
        return;
    }
    
    // 收集所有字段（避免迭代时修改）
    TArray<TPair<FString, TSharedPtr<FJsonValue>>> Fields;
    for (const auto& Field : JsonObject->Values)
    {
        Fields.Add(TPair<FString, TSharedPtr<FJsonValue>>(Field.Key, Field.Value));
    }
    
    // 清空原对象
    JsonObject->Values.Empty();
    
    // 重新添加，转换字段名
    for (const auto& Field : Fields)
    {
        FString NewKey = CapitalizeFirstLetter(Field.Key);
        TSharedPtr<FJsonValue> Value = Field.Value;
        
        // 递归处理嵌套对象
        if (Value.IsValid() && Value->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NestedObj = Value->AsObject();
            ConvertJsonFieldNamesToPascalCase(NestedObj);
        }
        // 递归处理数组中的对象
        else if (Value.IsValid() && Value->Type == EJson::Array)
        {
            TArray<TSharedPtr<FJsonValue>> Array = Value->AsArray();
            for (int32 i = 0; i < Array.Num(); ++i)
            {
                if (Array[i].IsValid() && Array[i]->Type == EJson::Object)
                {
                    TSharedPtr<FJsonObject> ArrayItemObj = Array[i]->AsObject();
                    ConvertJsonFieldNamesToPascalCase(ArrayItemObj);
                }
            }
        }
        
        JsonObject->SetField(NewKey, Value);
    }
}

FString UCORDataLibrary::GetEntireTableAsJson(UDataTable* DataTable)
{
    if (!DataTable)
    {
        return TEXT("{}");
    }
    
    TSharedRef<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject());
    
    for (auto it : DataTable->GetRowMap())
    {
        FName RowName = it.Key;
        uint8* RowData = it.Value;
        
        TSharedPtr<FJsonObject> RowJsonObject = MakeShareable(new FJsonObject());
        if (!FJsonObjectConverter::UStructToJsonObject(DataTable->RowStruct, RowData, RowJsonObject.ToSharedRef(), 0, 0))
        {
            continue;
        }
        
        // 转换字段名为 PascalCase
        ConvertJsonFieldNamesToPascalCase(RowJsonObject);
        
        RootJsonObject->SetObjectField(RowName.ToString(), RowJsonObject);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = 
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    
    FJsonSerializer::Serialize(RootJsonObject, JsonWriter);
    
    return OutputString;
}

FString UCORDataLibrary::GetTableRowAsJson(UDataTable* DataTable, FName RowName)
{
    if (!DataTable)
    {
        return FString();
    }
    
    uint8* RowData = DataTable->FindRowUnchecked(RowName);
    if (!RowData)
    {
        return FString();
    }
    
    TSharedPtr<FJsonObject> RowJsonObject = MakeShareable(new FJsonObject());
    if (!FJsonObjectConverter::UStructToJsonObject(DataTable->RowStruct, RowData, RowJsonObject.ToSharedRef(), 0, 0))
    {
        return FString();
    }
    
    // 转换字段名为 PascalCase
    ConvertJsonFieldNamesToPascalCase(RowJsonObject);
    
    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = 
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
    
    FJsonSerializer::Serialize(RowJsonObject.ToSharedRef(), JsonWriter);
    
    return JsonString;
}