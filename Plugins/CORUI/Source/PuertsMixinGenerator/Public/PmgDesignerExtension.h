#pragma once
#include "DesignerExtension.h"
#include "WidgetBlueprint.h"
#include "Designer/SDesignerView.h"

class FPmgDesignerExtension : public FDesignerExtension
{
public:
	FPmgDesignerExtension();
	virtual ~FPmgDesignerExtension();
	
	virtual void Initialize(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint) override;
	virtual void Uninitialize() override;

	void OnAssetClosed(UObject* Object, IAssetEditorInstance* AssetEditorInstance);
	void OnEditorClose();
	void RemoveAssetEditorSubsystemEvent();

	void InitVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint);
	TSharedPtr<SVerticalBox> GetOrCreateVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint);

	// 重新加载VBoxList
	void ReloadVBoxList(TWeakObjectPtr<UWidgetBlueprint> InBlueprint, TWeakPtr<SVerticalBox> InVBox, TArray<FString> InList);

	// 更新显示数据
	void UpdateShowData();
	
	// 清理
	void Cleanup();

protected:
	virtual bool HandleTick(float DeltaTime);

	// 获取蓝图名字
	FString GetBlueprintName(TWeakObjectPtr<UWidgetBlueprint> InBlueprint);

	// 计算 DrawCall 数
	int32 CalculateWidgets(TWeakObjectPtr<UWidgetBlueprint> InBlueprint);

protected:
	// Tick 的等待时间
	bool bTickDelay = 1.0f;
	// VBox 名字
	FName VBoxName = FName(TEXT("VBox_PmgDesignerExtension_InfoList"));
	
	// 用于记录遍历过程中的渲染状态，以判断是否需要新增 DrawCall
	struct FDrawCallState
	{
		// 当前正在使用的贴图、材质或字体
		const UObject* CurrentResource = nullptr; 
		// 当前的裁剪状态
		EWidgetClipping CurrentClipping = EWidgetClipping::Inherit;
		// 是否已经开始绘制有效像素
		bool bHasStartDrawing = false;
		// 当前模拟的 ZOrder 渲染层级
		int32 CurrentSimulatedLayer = 0;
		// 上一次绘制的渲染层级
		int32 LastRenderedLayer = 0;
	};

	// 递归计算 DC
	void CalculateDrawCallRecursive(UWidget* Widget, FDrawCallState& State, int32& OutDrawCalls);

	// 获取画刷中的资源对象
	UObject* GetBrushResource(const FSlateBrush& Brush);
private:

	// Ticker Handle
	FTSTicker::FDelegateHandle TickerHandle;

	// VBox 列表
	TMap<TWeakObjectPtr<UWidgetBlueprint>, TWeakPtr<SVerticalBox>> VBoxs;
	
	bool bIsDestroyed = false;
};
