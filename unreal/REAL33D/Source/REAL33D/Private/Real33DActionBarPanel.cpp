#include "Real33DActionBarPanel.h"

#include "Real33DPanelChrome.h"
#include "Real33DUIStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr float SlotEdge = FReal33DUIStyle::ActionSlotSize;
	constexpr float SlotGap = 2.0f;
	constexpr float SliderSize = 17.0f;

	TSharedRef<SWidget> MakeSliders(const FName& Single, const FName& Double)
	{
		const ISlateStyle& Style = FReal33DUIStyle::Get();
		const auto Arrow = [&Style](const FName& Brush)
		{
			return SNew(SBox).WidthOverride(SliderSize).HeightOverride(SliderSize)
				[ SNew(SImage).Image(Style.GetBrush(Brush)) ];
		};
		return SNew(SBox).WidthOverride(SliderSize).HeightOverride(SlotEdge)
			.ToolTipText(FText::FromString(TEXT("No further action buttons")))
			[ SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Arrow(Single) ]
				+ SVerticalBox::Slot().AutoHeight()[ Arrow(Double) ] ];
	}
}

void SReal33DActionSlot::Construct(const FArguments& InArgs)
{
	Index = InArgs._Index;
	OnItemBound = InArgs._OnItemBound;
	OnTextBound = InArgs._OnTextBound;
	OnCleared = InArgs._OnCleared;
	OnActivated = InArgs._OnActivated;
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	ChildSlot
	[ SNew(SBox).WidthOverride(SlotEdge).HeightOverride(SlotEdge)
		[ SNew(SOverlay)
			+ SOverlay::Slot()
			[ SNew(SImage).Image(Style.GetBrush("Real33D.ActionBar.Slot")) ]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[ SAssignNew(Icon, SImage).Visibility(EVisibility::HitTestInvisible) ]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[ SAssignNew(Label, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout")) ]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
			[ SAssignNew(ModeLabel, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FLinearColor::White) ]
			+ SOverlay::Slot()
			[ SAssignNew(UnavailableOverlay, SImage)
				.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.Visibility(EVisibility::Collapsed) ]
		] ];
	SetBinding(FReal33DActionBinding{}, false);
}

void SReal33DActionSlot::SetBinding(const FReal33DActionBinding& InBinding,
	bool bInAvailable)
{
	if (bPresented && Binding.Kind == InBinding.Kind
		&& Binding.TypeId == InBinding.TypeId && Binding.Text == InBinding.Text
		&& bAvailable == bInAvailable) return;
	bPresented = true;
	bAvailable = bInAvailable;
	Binding = InBinding;
	const bool bItem = Binding.Kind == FReal33DActionBinding::EKind::ItemUse
		|| Binding.Kind == FReal33DActionBinding::EKind::ItemUseWith;
	const FSlateBrush* Picture = bItem ? FReal33DUIStyle::ItemBrush(Binding.TypeId) : nullptr;
	Icon->SetImage(Picture);
	Icon->SetVisibility(Picture != nullptr ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	FString Shown;
	if (Binding.Kind == FReal33DActionBinding::EKind::Text)
	{
		Shown = Binding.Text.Left(5);
	}
	else if (bItem && Picture == nullptr)
	{
		Shown = FString::FromInt(Binding.TypeId);
	}
	Label->SetText(FText::FromString(Shown));
	ModeLabel->SetText(FText::FromString(
		Binding.Kind == FReal33DActionBinding::EKind::ItemUseWith ? TEXT("W") : TEXT("")));
	UnavailableOverlay->SetVisibility(bItem && !bInAvailable
		? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	FString Tip = TEXT("Empty action slot. Drag an item here or right-click to bind text.");
	if (bItem)
	{
		Tip = FString::Printf(TEXT("%s item %u%s. Right-click to change or clear."),
			Binding.Kind == FReal33DActionBinding::EKind::ItemUseWith
				? TEXT("Use with") : TEXT("Use"), Binding.TypeId,
			bInAvailable ? TEXT("") : TEXT(" (unavailable)"));
	}
	else if (Binding.Kind == FReal33DActionBinding::EKind::Text)
	{
		Tip = FString::Printf(TEXT("Say: %s. Right-click to change or clear."), *Binding.Text);
	}
	SetToolTipText(FText::FromString(Tip));
}

FReply SReal33DActionSlot::OnMouseButtonDown(const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ShowMenu(Geometry);
		return FReply::Handled();
	}
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnActivated.ExecuteIfBound(Index);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SReal33DActionSlot::OnDragOver(const FGeometry& Geometry,
	const FDragDropEvent& Event)
{
	return Event.GetOperationAs<FReal33DItemDrag>().IsValid()
		? FReply::Handled() : FReply::Unhandled();
}

FReply SReal33DActionSlot::OnDrop(const FGeometry& Geometry,
	const FDragDropEvent& Event)
{
	const TSharedPtr<FReal33DItemDrag> Drag = Event.GetOperationAs<FReal33DItemDrag>();
	if (!Drag.IsValid() || Drag->Origin().TypeId == 0) return FReply::Unhandled();
	OnItemBound.ExecuteIfBound(Index, Drag->Origin().TypeId, false);
	return FReply::Handled();
}

FReply SReal33DActionSlot::SetItemMode(bool bWithTarget)
{
	if (Binding.TypeId != 0)
	{
		OnItemBound.ExecuteIfBound(Index, Binding.TypeId, bWithTarget);
	}
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

FReply SReal33DActionSlot::BindText()
{
	const FString Value = TextInput.IsValid() ? TextInput->GetText().ToString().TrimStartAndEnd() : FString();
	if (!Value.IsEmpty()) OnTextBound.ExecuteIfBound(Index, Value);
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

FReply SReal33DActionSlot::Clear()
{
	OnCleared.ExecuteIfBound(Index);
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

void SReal33DActionSlot::ShowMenu(const FGeometry& Geometry)
{
	const bool bItem = Binding.Kind == FReal33DActionBinding::EKind::ItemUse
		|| Binding.Kind == FReal33DActionBinding::EKind::ItemUseWith;
	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
	if (bItem)
	{
		Menu->AddSlot().AutoHeight()
		[ SNew(SButton).Text(FText::FromString(TEXT("Use")))
			.OnClicked_Lambda([this]() { return SetItemMode(false); }) ];
		Menu->AddSlot().AutoHeight()
		[ SNew(SButton).Text(FText::FromString(TEXT("Use with target")))
			.OnClicked_Lambda([this]() { return SetItemMode(true); }) ];
	}
	Menu->AddSlot().AutoHeight()
	[ SAssignNew(TextInput, SEditableTextBox)
		.HintText(FText::FromString(TEXT("Text or spell words")))
		.Text(FText::FromString(Binding.Kind == FReal33DActionBinding::EKind::Text
			? Binding.Text : TEXT("")))
		.MinDesiredWidth(165.0f) ];
	Menu->AddSlot().AutoHeight()
	[ SNew(SButton).Text(FText::FromString(TEXT("Bind text (Say)")))
		.OnClicked_Lambda([this]() { return BindText(); }) ];
	if (Binding.Kind != FReal33DActionBinding::EKind::Empty)
	{
		Menu->AddSlot().AutoHeight()
		[ SNew(SButton).Text(FText::FromString(TEXT("Clear slot")))
			.OnClicked_Lambda([this]() { return Clear(); }) ];
	}
	FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Menu,
		Geometry.GetAbsolutePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

void SReal33DActionBar::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	TSharedRef<SWidget> SlotRow = InArgs._Vertical
		? StaticCastSharedRef<SWidget>(SNew(SVerticalBox))
		: StaticCastSharedRef<SWidget>(SNew(SHorizontalBox));
	for (int32 Local = 0; Local < InArgs._SlotCount; ++Local)
	{
		TSharedPtr<SReal33DActionSlot> Button;
		TSharedRef<SWidget> Content = SAssignNew(Button, SReal33DActionSlot)
			.Index(InArgs._StartIndex + Local)
			.OnItemBound(InArgs._OnItemBound)
			.OnTextBound(InArgs._OnTextBound)
			.OnCleared(InArgs._OnCleared)
			.OnActivated(InArgs._OnActivated);
		Slots.Add(Button);
		if (InArgs._Vertical)
		{
			StaticCastSharedRef<SVerticalBox>(SlotRow)->AddSlot().AutoHeight()
				.Padding(FMargin(0, 0, 0, SlotGap))[ Content ];
		}
		else
		{
			StaticCastSharedRef<SHorizontalBox>(SlotRow)->AddSlot().AutoWidth()
				.Padding(FMargin(0, 0, SlotGap, 0))[ Content ];
		}
	}
	TSharedRef<SWidget> Body = InArgs._Vertical ? SlotRow
		: StaticCastSharedRef<SWidget>(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			.Padding(FMargin(1, 0, 2, 0))
			[ MakeSliders("Real33D.ActionBar.Previous", "Real33D.ActionBar.First") ]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[ SlotRow ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			.Padding(FMargin(2, 0, 1, 0))
			[ MakeSliders("Real33D.ActionBar.Next", "Real33D.ActionBar.Last") ]);
	ChildSlot
	[ SNew(SBorder).BorderImage(Style.GetBrush("Real33D.ActionBar.Background"))
		.BorderBackgroundColor(FReal33DUIStyle::PanelTint())
		.Padding(FMargin(0, 3, 0, 1))
		.HAlign(InArgs._Vertical ? HAlign_Center : HAlign_Fill)
		[ Body ] ];
}

void SReal33DActionBar::SetSlot(int32 LocalIndex,
	const FReal33DActionBinding& Binding, bool bAvailable)
{
	if (Slots.IsValidIndex(LocalIndex) && Slots[LocalIndex].IsValid())
	{
		Slots[LocalIndex]->SetBinding(Binding, bAvailable);
	}
}
