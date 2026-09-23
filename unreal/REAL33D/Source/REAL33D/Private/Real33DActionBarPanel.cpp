#include "Real33DActionBarPanel.h"

#include "Real33DUIStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

namespace
{
	// actionbar.otui: slots are 34x34 with a 2px left margin, the bar pads 3 at
	// the top and 1 at the bottom, and the sliders are 17x17 pairs at each end.
	// Not named `Slot`: the engine's shared PCH declares a parameter of that
	// name in UnrealType.h, and a file-scope constant would shadow it there.
	constexpr float SlotEdge = FReal33DUIStyle::ActionSlotSize;
	constexpr float SlotGap = 2.0f;
	constexpr float BarPadTop = 3.0f;
	constexpr float BarPadBottom = 1.0f;
	constexpr float SliderSize = 17.0f;

	/** The sliders column: two stacked arrows, both in their disabled cell. */
	TSharedRef<SWidget> MakeSliders(const FName& Single, const FName& Double)
	{
		const ISlateStyle& Style = FReal33DUIStyle::Get();
		const auto Arrow = [&Style](const FName& Brush)
		{
			return SNew(SBox)
				.WidthOverride(SliderSize)
				.HeightOverride(SliderSize)
				[
					SNew(SImage).Image(Style.GetBrush(Brush))
				];
		};
		return SNew(SBox)
			.WidthOverride(SliderSize)
			.HeightOverride(SlotEdge)
			.ToolTipText(FText::FromString(
				TEXT("No further action buttons in this direction")))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Arrow(Single) ]
				+ SVerticalBox::Slot().AutoHeight()[ Arrow(Double) ]
			];
	}
}

void SReal33DActionBar::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// One empty slot. The 2D draws the same art when a button has nothing
	// assigned, so an unbound bar looks here exactly as it looks there.
	const auto MakeSlot = [&Style]()
	{
		return SNew(SBox)
			.WidthOverride(SlotEdge)
			.HeightOverride(SlotEdge)
			.ToolTipText(FText::FromString(
				TEXT("Action buttons need spells or objects, which are not decoded yet")))
			[
				SNew(SImage).Image(Style.GetBrush("Real33D.ActionBar.Slot"))
			];
	};

	TSharedRef<SWidget> Slots = InArgs._Vertical
		? StaticCastSharedRef<SWidget>(SNew(SVerticalBox))
		: StaticCastSharedRef<SWidget>(SNew(SHorizontalBox));

	for (int32 Index = 0; Index < InArgs._SlotCount; ++Index)
	{
		const FMargin Pad = InArgs._Vertical
			? FMargin(0.0f, 0.0f, 0.0f, SlotGap)
			: FMargin(0.0f, 0.0f, SlotGap, 0.0f);
		if (InArgs._Vertical)
		{
			StaticCastSharedRef<SVerticalBox>(Slots)->AddSlot()
				.AutoHeight().Padding(Pad)[ MakeSlot() ];
		}
		else
		{
			StaticCastSharedRef<SHorizontalBox>(Slots)->AddSlot()
				.AutoWidth().Padding(Pad)[ MakeSlot() ];
		}
	}

	// The vertical columns carry no sliders in the 2D -- gameinterface.otui
	// gives them a plain horizontalBox -- so only the bottom bar gets them.
	TSharedRef<SWidget> Body = InArgs._Vertical
		? Slots
		: StaticCastSharedRef<SWidget>(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			.Padding(FMargin(1.0f, 0.0f, 2.0f, 0.0f))
			[
				MakeSliders("Real33D.ActionBar.Previous", "Real33D.ActionBar.First")
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				Slots
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			.Padding(FMargin(2.0f, 0.0f, 1.0f, 0.0f))
			[
				MakeSliders("Real33D.ActionBar.Next", "Real33D.ActionBar.Last")
			]);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.ActionBar.Background"))
		.BorderBackgroundColor(FReal33DUIStyle::PanelTint())
		.Padding(FMargin(0.0f, BarPadTop, 0.0f, BarPadBottom))
		.HAlign(InArgs._Vertical ? HAlign_Center : HAlign_Fill)
		[
			Body
		]
	];
}
