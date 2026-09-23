#include "Real33DInventoryPanel.h"

#include "Real33DPanelChrome.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// inventory.otui: slots sit 3 apart, the panel is inset 8 from the left,
	// and the header row of buttons is 5 down from the top.
	constexpr float SlotGap = 3.0f;
	/** The 12px header button plus its 3px gap, per inventory.otui. */
	constexpr float ColumnStagger = 15.0f;
	constexpr float PanelInset = 8.0f;
	constexpr float TopInset = 5.0f;
	constexpr float ButtonSize = 20.0f;
	constexpr float ReadoutWidth = FReal33DUIStyle::SlotSize;
	constexpr int32 TextSize = 8;
	constexpr int32 ValueSize = 9;
}

TSharedRef<SWidget> SReal33DInventoryPanel::MakeSlot(
	const FName& Placeholder, const FText& Tooltip)
{
	return SNew(SReal33DSlot)
		.SlotBrush("Real33D.Inventory.Slot")
		.PlaceholderBrush(Placeholder)
		.Tooltip(Tooltip);
}

TSharedRef<SWidget> SReal33DInventoryPanel::MakeReadout(
	const FText& Caption, TSharedPtr<STextBlock>& OutValue)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SBox)
		.WidthOverride(ReadoutWidth)
		.HeightOverride(FReal33DUIStyle::SlotSize)
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
			.Padding(FMargin(1.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(Caption)
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(OutValue, STextBlock)
					.Text(FText::FromString(TEXT("--")))
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", ValueSize))
				]
			]
		];
}

TSharedRef<SWidget> SReal33DInventoryPanel::MakeCombatButton(
	const FName& Brush, const FText& Tooltip)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SBox)
		.WidthOverride(ButtonSize)
		.HeightOverride(ButtonSize)
		.ToolTipText(Tooltip)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(Style.GetBrush(Brush))
			]
			+ SOverlay::Slot()
			[
				// The 2D's own way of saying a control is unavailable. This
				// client sends no fight-mode command and the server reports
				// none back, so there is no state these could truthfully show.
				SNew(SImage)
				.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
				.Visibility(EVisibility::HitTestInvisible)
			]
		];
}

void SReal33DInventoryPanel::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// The three slot columns of inventory.otui, in its own order. Column one is
	// amulet, sword, ring and then Soul; column two helmet, armor, legs, boots;
	// column three backpack, shield, tools and then Cap.
	TSharedRef<SVerticalBox> Left = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> Middle = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);

	const auto Stack = [](TSharedRef<SVerticalBox> Column, TSharedRef<SWidget> Child)
	{
		Column->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, SlotGap))
			[
				Child
			];
	};

	Stack(Left, MakeSlot("Real33D.Inventory.Slot.neck",
		FText::FromString(TEXT("Amulet"))));
	Stack(Left, MakeSlot("Real33D.Inventory.Slot.right_hand",
		FText::FromString(TEXT("Weapon"))));
	Stack(Left, MakeSlot("Real33D.Inventory.Slot.finger",
		FText::FromString(TEXT("Ring"))));
	Stack(Left, MakeReadout(FText::FromString(TEXT("Soul")), SoulValue));

	Stack(Middle, MakeSlot("Real33D.Inventory.Slot.head",
		FText::FromString(TEXT("Helmet"))));
	Stack(Middle, MakeSlot("Real33D.Inventory.Slot.torso",
		FText::FromString(TEXT("Armor"))));
	Stack(Middle, MakeSlot("Real33D.Inventory.Slot.legs",
		FText::FromString(TEXT("Legs"))));
	Stack(Middle, MakeSlot("Real33D.Inventory.Slot.feet",
		FText::FromString(TEXT("Boots"))));

	Stack(Right, MakeSlot("Real33D.Inventory.Slot.back",
		FText::FromString(TEXT("Backpack"))));
	Stack(Right, MakeSlot("Real33D.Inventory.Slot.left_hand",
		FText::FromString(TEXT("Shield"))));
	Stack(Right, MakeSlot("Real33D.Inventory.Slot.hip",
		FText::FromString(TEXT("Tools"))));
	Stack(Right, MakeReadout(FText::FromString(TEXT("Cap")), CapacityValue));

	// The combat column the 2D puts down the right-hand edge.
	TSharedRef<SVerticalBox> Combat = SNew(SVerticalBox);
	const auto Toggle = [&](const FName& Brush, const TCHAR* Label)
	{
		Combat->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
			[
				MakeCombatButton(Brush, FText::FromString(Label))
			];
	};
	Toggle("Real33D.Inventory.Attack", TEXT("Offensive (not available)"));
	Toggle("Real33D.Inventory.Balanced", TEXT("Balanced (not available)"));
	Toggle("Real33D.Inventory.Defend", TEXT("Defensive (not available)"));
	Toggle("Real33D.Inventory.Stand", TEXT("Stand while fighting (not available)"));
	Toggle("Real33D.Inventory.Follow", TEXT("Chase opponent (not available)"));

	ChildSlot
	.Padding(FMargin(PanelInset, TopInset, PanelInset, 0.0f))
	[
		SNew(SHorizontalBox)
		// The outer columns start 15px lower than the middle one. That is the
		// classic cross: inventory.otui anchors the helmet to `changeSize.top`
		// but the amulet and the backpack to `changeSize.bottom` plus 3, and
		// the 12px button between them is where the offset comes from. Lining
		// all three columns up flat loses the shape the layout is known by.
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0.0f, ColumnStagger, SlotGap, 0.0f))
		[
			Left
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0.0f, 0.0f, SlotGap, 0.0f))
		[
			Middle
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0.0f, ColumnStagger, 0.0f, 0.0f))
		[
			Right
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		[
			Combat
		]
	];

	(void)Style;
}

void SReal33DInventoryPanel::SetVitals(const FReal33DPlayerVitals& Vitals)
{
	if (bHasDrawnOnce
		&& Last.bKnown == Vitals.bKnown
		&& Last.SoulPoints == Vitals.SoulPoints
		&& Last.Capacity == Vitals.Capacity)
	{
		return;
	}
	Last = Vitals;
	bHasDrawnOnce = true;

	const FText Unknown = FText::FromString(TEXT("--"));
	if (SoulValue.IsValid())
	{
		SoulValue->SetText(Vitals.bKnown
			? FText::FromString(FString::Printf(TEXT("%d"),
				static_cast<int32>(Vitals.SoulPoints)))
			: Unknown);
	}
	if (CapacityValue.IsValid())
	{
		CapacityValue->SetText(Vitals.bKnown
			? FText::FromString(FString::Printf(TEXT("%d"),
				static_cast<int32>(Vitals.Capacity)))
			: Unknown);
	}
}

// ------------------------------------------------------------ condition strip

void SReal33DConditionStrip::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const float Size = FReal33DUIStyle::ConditionIconSize;

	static const TCHAR* const Names[FlagCount] = {
		TEXT("Poisoned"), TEXT("Burning"), TEXT("Electrified"), TEXT("Drunk"),
		TEXT("ManaShield"), TEXT("Slowed"), TEXT("Hasted"), TEXT("LogoutBlocked") };
	static const TCHAR* const Tips[FlagCount] = {
		TEXT("You are poisoned"), TEXT("You are burning"),
		TEXT("You are electrified"), TEXT("You are drunk"),
		TEXT("You are protected by a magic shield"), TEXT("You are slowed"),
		TEXT("You are hasted"), TEXT("You may not logout during a fight") };

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < FlagCount; ++Index)
	{
		Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(Index == 0 ? 0.0f : 2.0f, 0.0f, 0.0f, 0.0f))
			[
				SAssignNew(Icons[Index], SBox)
				.WidthOverride(Size)
				.HeightOverride(Size)
				.ToolTipText(FText::FromString(Tips[Index]))
				// Hidden rather than collapsed, so the icons keep their places
				// as conditions come and go instead of shuffling left and right.
				.Visibility(EVisibility::Hidden)
				[
					SNew(SImage).Image(Style.GetBrush(
						FName(*FString::Printf(TEXT("Real33D.Condition.%s"), Names[Index]))))
				]
			];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
		.Padding(FMargin(2.0f))
		[
			Row
		]
	];
}

void SReal33DConditionStrip::SetConditions(const FReal33DConditions& Conditions)
{
	if (bHasDrawnOnce && Last.bKnown == Conditions.bKnown
		&& Last.Flags == Conditions.Flags)
	{
		return;
	}
	Last = Conditions;
	bHasDrawnOnce = true;

	for (int32 Index = 0; Index < FlagCount; ++Index)
	{
		if (!Icons[Index].IsValid())
		{
			continue;
		}
		// Bit n of the byte is icon n: the order of FReal33DConditions::EFlag is
		// the order of the strip, pinned to Fusion32's own bit values by a
		// static_assert in the bridge.
		const uint8 Bit = static_cast<uint8>(1u << Index);
		const bool bRaised = Conditions.bKnown && (Conditions.Flags & Bit) != 0;
		Icons[Index]->SetVisibility(bRaised ? EVisibility::Visible : EVisibility::Hidden);
	}
}
