#include "Core/Utils/CORDataLibrary.h"
#include "Engine/DataTable.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

bool UCORDataLibrary::ImportTableFromJson(UDataTable* DataTable, const FString& JsonString)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> DataTable is null."));
        return false;
    }
    
    UScriptStruct* RowStruct = DataTable->RowStruct;
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> DataTable '%s' has no RowStruct."),
            *DataTable->GetName());
        return false;
    }
    
    if (JsonString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> JsonString is empty."));
        return false;
    }
    
    // 解析根对象：{ "RowName": { "Field": value } }
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(JsonReader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> failed to parse JSON string."));
        return false;
    }
    
    // 两阶段导入：先把所有行解析/校验到临时缓冲，全部成功后再一次性替换表内容，
    // 避免中途失败留下“半张表”（与 CSV 导入的替换语义保持一致）。
    TArray<TPair<FName, uint8*>> ParsedRows;
    ParsedRows.Reserve(RootObject->Values.Num());

    bool bParseSucceeded = true;
    for (const auto& RowPair : RootObject->Values)
    {
        // 键为行名（UE 5.8 起 FJsonObject 的键可能不是 FString，统一走视图构造 FName）
        const FName RowName(RowPair.Key.ToView());

        if (!RowPair.Value.IsValid() || RowPair.Value->Type != EJson::Object)
        {
            UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> row '%s' is not a JSON object."),
                *RowName.ToString());
            bParseSucceeded = false;
            break;
        }

        const TSharedPtr<FJsonObject> RowObject = RowPair.Value->AsObject();
        if (!RowObject.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> row '%s' is an invalid JSON object."),
                *RowName.ToString());
            bParseSucceeded = false;
            break;
        }

        // 按 RowStruct 分配临时行内存
        uint8* RowData = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
        RowStruct->InitializeStruct(RowData);

        // 严格模式：未知字段/类型不匹配直接报错，而不是静默丢字段
        FText FailReason;
        if (!FJsonObjectConverter::JsonObjectToUStruct(RowObject.ToSharedRef(), RowStruct, RowData,
            0, 0, /*bStrictMode*/ true, &FailReason))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> failed to convert row '%s' to RowStruct '%s': %s"),
                *RowName.ToString(), *RowStruct->GetName(), *FailReason.ToString());
            RowStruct->DestroyStruct(RowData);
            FMemory::Free(RowData);
            bParseSucceeded = false;
            break;
        }

        ParsedRows.Add(TPair<FName, uint8*>(RowName, RowData));
    }

    if (!bParseSucceeded)
    {
        // 回滚：释放已解析的行缓冲，表保持原样
        for (const TPair<FName, uint8*>& Parsed : ParsedRows)
        {
            RowStruct->DestroyStruct(Parsed.Value);
            FMemory::Free(Parsed.Value);
        }
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> import aborted, table unchanged."));
        return false;
    }

    // 全部解析成功：替换表内容
    DataTable->EmptyTable();
    for (const TPair<FName, uint8*>& Parsed : ParsedRows)
    {
        // AddRow 是拷贝语义，之后即可销毁临时行内存
        DataTable->AddRow(Parsed.Key, Parsed.Value, RowStruct);
        RowStruct->DestroyStruct(Parsed.Value);
        FMemory::Free(Parsed.Value);
    }

    UE_LOG(LogTemp, Log, TEXT("[CORUI] UCORDataLibrary::ImportTableFromJson >> imported %d row(s) into '%s'."),
        ParsedRows.Num(), *DataTable->GetName());

    return true;
}

bool UCORDataLibrary::ImportTableFromCsv(UDataTable* DataTable, const FString& CsvString)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromCsv >> DataTable is null."));
        return false;
    }
    
    if (!DataTable->GetRowStruct())
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromCsv >> DataTable '%s' has no RowStruct."),
            *DataTable->GetName());
        return false;
    }
    
    const TArray<FString> Errors = DataTable->CreateTableFromCSVString(CsvString);
    for (const FString& Error : Errors)
    {
        UE_LOG(LogTemp, Error, TEXT("[CORUI] UCORDataLibrary::ImportTableFromCsv >> %s"), *Error);
    }

    if (Errors.Num() > 0)
    {
        return false;
    }

    // 仅表头（0 行）视为无效导入：此时表已被清空，必须让调用方知道没有数据
    if (DataTable->GetRowNames().Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CORUI] UCORDataLibrary::ImportTableFromCsv >> no data rows imported."));
        return false;
    }

    return true;
}

FString UCORDataLibrary::ExportTableToCsv(UDataTable* DataTable)
{
    if (!DataTable)
    {
        return FString();
    }
    
#if WITH_EDITOR
    return DataTable->GetTableAsCSV();
#else
    // GetTableAsCSV 仅在编辑器构建中存在
    UE_LOG(LogTemp, Warning, TEXT("[CORUI] UCORDataLibrary::ExportTableToCsv >> CSV export is only available in editor builds."));
    return FString();
#endif
}

FString UCORDataLibrary::ExportTableToJson(UDataTable* DataTable)
{
    return GetEntireTableAsJson(DataTable);
}

UDataTable* UCORDataLibrary::CreateTableFromTemplate(UDataTable* TemplateTable, FName TableName)
{
    if (!TemplateTable || !TemplateTable->GetRowStruct())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CORUI] UCORDataLibrary::CreateTableFromTemplate >> TemplateTable or its RowStruct is null."));
        return nullptr;
    }

    // 唯一对象名：重复调用不会替换/销毁上一张内存表（同名会触发引擎的 in-place replace）
    const FName BaseName = TableName.IsNone() ? FName(TEXT("DT_CORTemp")) : TableName;
    const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), BaseName);
    UDataTable* NewTable = NewObject<UDataTable>(GetTransientPackage(), UDataTable::StaticClass(), UniqueName);
    if (!NewTable)
    {
        return nullptr;
    }

    // 复制模板的行结构，使新表可以独立接收导入数据（GetRowStruct 返回 const，RowStruct 需要可变指针）
    NewTable->RowStruct = const_cast<UScriptStruct*>(TemplateTable->GetRowStruct());
    return NewTable;
}