#pragma once
#include "DesignerExtension.h"
#include "WidgetBlueprint.h"
#include "Designer/SDesignerView.h"

class FPmgDesignerExtension : public FDesignerExtension
{
public:
	// 信息面板的一行（文本 + 颜色，用于 DC 阈值提示）
	struct FInfoLine
	{
		FString Text;
		FLinearColor Color = FLinearColor(0.85f, 0.85f, 0.85f);
	};

	// 绘制元素类别：只用于明细展示与批次键的"图元"维度
	enum class EElementKind : uint8
	{
		Image = 0,
		Text = 1,
		Border = 2,
		Box = 3,
		Effect = 4,
	};

	/**
	 * 一次绘制的批次键：键相同才可能合批，键变化即断批（= 新增一个 DrawCall）。
	 * Slate 合批看的是 shader / 资源 / 裁剪状态，所以资源、图元、裁剪层级、混合状态、字体参数都要进键。
	 */
	struct FBatchKey
	{
		const UObject* Resource = nullptr;
		uint8 Primitive = 0;
		// 裁剪层级：进入新的裁剪区会插入 clip state，打断合批
		int32 ClipLayer = 0;
		// 半透明合成（RenderOpacity < 1）会走不同的渲染路径，不能与不透明元素合批
		bool bBlended = false;
		// 字体相关：字号 / 描边 / 阴影不同都无法合批；同字号不同字重（Bold/Regular）也不行
		int32 FontSize = 0;
		int32 OutlineSize = 0;
		bool bHasShadow = false;
		FName TypefaceFontName = NAME_None;
		// Slate layer（UMG 里来自 ZOrder）：不同 layer 的元素不会进同一批次
		int32 Layer = 0;

		bool operator==(const FBatchKey& Other) const
		{
			return Resource == Other.Resource
				&& Primitive == Other.Primitive
				&& ClipLayer == Other.ClipLayer
				&& bBlended == Other.bBlended
				&& FontSize == Other.FontSize
				&& OutlineSize == Other.OutlineSize
				&& bHasShadow == Other.bHasShadow
				&& TypefaceFontName == Other.TypefaceFontName
				&& Layer == Other.Layer;
		}

		bool operator!=(const FBatchKey& Other) const
		{
			return !(*this == Other);
		}
	};

	// 一次遍历的状态与统计
	struct FDrawCallState
	{
		// 合批状态按绘制顺序线性推进（跨兄弟节点成立），裁剪状态在进出子树时 save/restore
		FBatchKey LastKey;
		bool bHasDrawn = false;
		// 当前有效裁剪层级（Inherit 沿用父级）
		int32 ClipLayer = 0;
		EWidgetClipping CurrentClipping = EWidgetClipping::Inherit;
		// 当前控件所在 Slate layer（来自 UWidget::GetZOrder），由 EmitWidgetElements 逐控件设置
		int32 CurrentLayer = 0;
		// RetainerBox / InvalidationBox 等会强制下一次断批
		bool bForceBreakNext = false;

		// === 统计 ===
		// 预估 DrawCall 数
		int32 Total = 0;
		// 绘制元素总数（DC 是元素的合批结果，元素数说明"有多少东西要画"）
		int32 RenderableCount = 0;
		int32 ImageCount = 0;
		int32 TextCount = 0;
		int32 BorderCount = 0;
		int32 BoxCount = 0;
		int32 EffectCount = 0;
		// 强制独立渲染目标造成的断批次数
		int32 ForceBreakCount = 0;
		// 扫描到的控件数
		int32 VisitedCount = 0;
		// 本估算不认识的叶子控件数（提示"可能偏低"）
		int32 UnknownLeafCount = 0;
	};

	FPmgDesignerExtension();
	virtual ~FPmgDesignerExtension();

	virtual void Initialize(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint) override;
	virtual void Uninitialize() override;

	void OnAssetClosed(UObject* Object, IAssetEditorInstance* AssetEditorInstance);
	void OnEditorClose();
	void RemoveAssetEditorSubsystemEvent();

	void InitVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint);
	TSharedPtr<SVerticalBox> GetOrCreateVBox(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint);

	// 重新加载VBoxList（行带颜色）
	// 刷新某个蓝图的 VBox 内容。返回 false 表示 VBox/蓝图已失效、本次没写成功，
	// 调用方不应记录内容签名（否则下一帧会被误判为"没变化"而永久留在旧值）。
	bool ReloadVBoxList(TWeakObjectPtr<UWidgetBlueprint> InBlueprint, TWeakPtr<SVerticalBox> InVBox, const TArray<FInfoLine>& InLines);

	// 更新显示数据
	void UpdateShowData();

	// 清理（终态、幂等：失效延迟工作、注销委托、移除 ticker、释放 UI 状态）
	void Cleanup();

protected:
	virtual bool HandleTick(float DeltaTime);

	// 延迟初始化：designer 就绪前先记在队列里，由 ticker 重试
	void TryPendingInit();

	// 获取蓝图名字
	FString GetBlueprintName(TWeakObjectPtr<UWidgetBlueprint> InBlueprint);

	// 计算某个蓝图窗口的 DC 统计
	FDrawCallState CalculateWidgets(TWeakObjectPtr<UWidgetBlueprint> InBlueprint);

	// 递归计算 DC（Visited/Depth 防御脏控件树成环）
	void CalculateDrawCallRecursive(UWidget* Widget, FDrawCallState& State, TSet<const UWidget*>& Visited, int32 Depth);

	// 某个控件自身产生的绘制元素（按绘制顺序喂给合批状态）
	void EmitWidgetElements(UWidget* Widget, FDrawCallState& State, bool bBlended);

	// 把单个绘制元素喂给合批状态：批次键变化才新增 DC
	void EmitElement(FDrawCallState& State, const UObject* Resource, EElementKind Kind, bool bBlended,
	                 int32 FontSize = 0, int32 OutlineSize = 0, bool bHasShadow = false,
	                 FName TypefaceFontName = NAME_None);

	// 获取画刷中的资源对象
	UObject* GetBrushResource(const FSlateBrush& Brush);

protected:
	// Tick 间隔（秒）：FTSTicker::AddTicker 的第二个参数是 float，
	// 旧代码把它声明成 bool 靠 true -> 1.0f 的隐式转换"碰巧"生效，这里改回真实类型语义
	float TickIntervalSeconds = 0.5f;

	// 重算节流：控件树遍历 + 排序 + 字符串格式化不便宜，按此间隔限频。
	// 注意必须用墙钟计时——FTSTicker 虽然按间隔触发，但传给委托的 DeltaTime 是帧 delta（~0.01s），
	// 累加 DeltaTime 永远攒不到阈值（旧实现踩过这个坑，面板只在第一帧刷新一次）。
	float RefreshIntervalSeconds = 1.0f;
	double LastRefreshTimeSeconds = 0.0;

	// VBox 名字
	FName VBoxName = FName(TEXT("VBox_PmgDesignerExtension_InfoList"));

private:
	// Ticker Handle
	FTSTicker::FDelegateHandle TickerHandle;

	// VBox 列表
	TMap<TWeakObjectPtr<UWidgetBlueprint>, TWeakPtr<SVerticalBox>> VBoxs;

	// 每个蓝图对应的 designer，用于 VBox 被重建冲掉后自愈重挂
	TMap<TWeakObjectPtr<UWidgetBlueprint>, TWeakPtr<class SDesignerView>> DesignerViews;

	// 待初始化队列：Initialize 时 designer 控件树可能还没搭好，ticker 里重试直到挂上
	TMap<TWeakObjectPtr<UWidgetBlueprint>, TWeakPtr<class SDesignerView>> PendingInit;

	// 上次渲染的文本签名：内容没变就不重刷 Slate（SetText 会让布局失效）
	TMap<TWeakObjectPtr<UWidgetBlueprint>, FString> LastRenderedSignature;

	// 延迟初始化 lambda 的生命周期令牌已移除：改为 PendingInit 队列 + ticker 重试。

	bool bIsDestroyed = false;

	// Cleanup 是否已执行（保证终态幂等）
	bool bIsCleanedUp = false;
};
