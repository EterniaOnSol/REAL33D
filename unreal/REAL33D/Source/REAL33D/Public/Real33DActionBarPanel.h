#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SImage;
class STextBlock;

/** Local preference only. Item coordinates are resolved from WorldState on use. */
struct FReal33DActionBinding
{
	enum class EKind : uint8 { Empty, ItemUse, ItemUseWith, Text };
	EKind Kind = EKind::Empty;
	uint16 TypeId = 0;
	FString Text;
};

DECLARE_DELEGATE_ThreeParams(FReal33DOnActionItemBound, int32, uint16, bool);
DECLARE_DELEGATE_TwoParams(FReal33DOnActionTextBound, int32, const FString&);
DECLARE_DELEGATE_OneParam(FReal33DOnActionIndex, int32);

/** One of the 2D styled 34px buttons. Its callbacks go to the HUD owner. */
class SReal33DActionSlot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DActionSlot) {}
		SLATE_ARGUMENT(int32, Index)
		SLATE_EVENT(FReal33DOnActionItemBound, OnItemBound)
		SLATE_EVENT(FReal33DOnActionTextBound, OnTextBound)
		SLATE_EVENT(FReal33DOnActionIndex, OnCleared)
		SLATE_EVENT(FReal33DOnActionIndex, OnActivated)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetBinding(const FReal33DActionBinding& InBinding, bool bInAvailable);
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry,
		const FPointerEvent& Event) override;
	virtual FReply OnDragOver(const FGeometry& Geometry,
		const FDragDropEvent& Event) override;
	virtual FReply OnDrop(const FGeometry& Geometry,
		const FDragDropEvent& Event) override;

private:
	void ShowMenu(const FGeometry& Geometry);
	FReply SetItemMode(bool bWithTarget);
	FReply BindText();
	FReply Clear();
	int32 Index = 0;
	FReal33DActionBinding Binding;
	bool bPresented = false;
	bool bAvailable = false;
	FReal33DOnActionItemBound OnItemBound;
	FReal33DOnActionTextBound OnTextBound;
	FReal33DOnActionIndex OnCleared;
	FReal33DOnActionIndex OnActivated;
	TSharedPtr<SImage> Icon;
	TSharedPtr<STextBlock> Label;
	TSharedPtr<STextBlock> ModeLabel;
	TSharedPtr<SImage> UnavailableOverlay;
	TSharedPtr<SEditableTextBox> TextInput;
};

/** The left, right and bottom bars share the same local configuration. */
class SReal33DActionBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DActionBar) : _SlotCount(10), _StartIndex(0), _Vertical(false) {}
		SLATE_ARGUMENT(int32, SlotCount)
		SLATE_ARGUMENT(int32, StartIndex)
		SLATE_ARGUMENT(bool, Vertical)
		SLATE_EVENT(FReal33DOnActionItemBound, OnItemBound)
		SLATE_EVENT(FReal33DOnActionTextBound, OnTextBound)
		SLATE_EVENT(FReal33DOnActionIndex, OnCleared)
		SLATE_EVENT(FReal33DOnActionIndex, OnActivated)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetSlot(int32 LocalIndex, const FReal33DActionBinding& Binding,
		bool bAvailable);
	static constexpr float BarThickness = 36.0f;

private:
	TArray<TSharedPtr<SReal33DActionSlot>> Slots;
};
