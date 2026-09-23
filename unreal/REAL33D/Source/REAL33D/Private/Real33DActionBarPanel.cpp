#include "Real33DActionBarPanel.h"

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
	// actionbar.otui: slots are 34x34 with a 2px left margin, the bar pads 3 at
	// the top and 1 at the bottom, and the sliders are 17x17 pairs at each end.
	// Not named `Slot`: the engine's shared PCH declares a parameter of that
	// name in UnrealType.h, and a file-scope constant would shadow it there.
	constexpr float SlotEdge = FReal33DUIStyle::ActionSlotSize;
	constexpr float SlotGap = 2.0f;
	constexpr float BarPadTop = 3.0f;
	constexpr float BarPadBottom = 1.0f;
	constexpr float SliderSize = 17.0f;

	// hotkeys_manager.otui: buttons stacked 2 apart, with the preview 10 to
	// their left. That file's 128px width is not kept -- see MakeButton.
	constexpr float ControlHeight = 20.0f;
	constexpr float ControlGap = 2.0f;
	constexpr float PreviewGap = 10.0f;
	constexpr int32 TextSize = 8;

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

// ---------------------------------------------------------- item-use controls

float SReal33DHotkeyPanel::DesiredContentHeight()
{
	// Preview row, then Select/Clear, then the four use modes, then the caption.
	return SlotEdge + PreviewGap + 6.0f * (ControlHeight + ControlGap) + 18.0f;
}

TSharedRef<SWidget> SReal33DHotkeyPanel::MakeButton(const FText& Label)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// Height fixed, width free. hotkeys_manager.otui gives these 128 inside a
	// 340-wide dialog; in a 176 side column 128 plus the 34 preview and the
	// 10 gap overflows, so the width comes from the column instead.
	return SNew(SBox)
		.HeightOverride(ControlHeight)
		.ToolTipText(FText::FromString(
			TEXT("Object use is not implemented in this client yet")))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(2.0f))
				[
					SNew(STextBlock)
					.Text(Label)
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Disabled"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				]
			]
		];
}

TSharedRef<SWidget> SReal33DHotkeyPanel::MakeModeBox(const FText& Label)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// hotkeys_manager.otui uses a ButtonBox for the four use modes: a button
	// that latches. All four are drawn unlatched, because nothing can select
	// one and a latched box would assert a mode the player never chose.
	// Height fixed, width free. hotkeys_manager.otui gives these 128 inside a
	// 340-wide dialog; in a 176 side column 128 plus the 34 preview and the
	// 10 gap overflows, so the width comes from the column instead.
	return SNew(SBox)
		.HeightOverride(ControlHeight)
		.ToolTipText(FText::FromString(
			TEXT("Object use is not implemented in this client yet")))
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 5.0f, 0.0f))
				[
					SNew(SBox)
					.WidthOverride(8.0f)
					.HeightOverride(8.0f)
					[
						SNew(SImage)
						.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.ColorAndOpacity(FLinearColor(0.16f, 0.16f, 0.16f, 1.0f))
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Label)
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Disabled"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				]
			]
		];
}

void SReal33DHotkeyPanel::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);

	// The preview and the two object buttons, laid out as hotkeys_manager.otui
	// lays them: the item square on the left, the buttons stacked beside it.
	Column->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, ControlGap))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0.0f, 0.0f, PreviewGap, 0.0f))
			[
				SNew(SReal33DSlot)
				.SlotBrush("Real33D.Chrome.ItemSlot")
				.Tooltip(FText::FromString(TEXT("No object selected")))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, ControlGap))
				[
					MakeButton(FText::FromString(TEXT("Select object")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeButton(FText::FromString(TEXT("Clear object")))
				]
			]
		];

	static const TCHAR* const Modes[] = {
		TEXT("Use on yourself"), TEXT("Use on target"),
		TEXT("With crosshair"), TEXT("Use at cursor position") };
	for (const TCHAR* const Mode : Modes)
	{
		Column->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, ControlGap))
			[
				MakeModeBox(FText::FromString(Mode))
			];
	}

	Column->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Object use is not decoded yet")))
			.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Disabled"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			.AutoWrapText(true)
		];

	ChildSlot
	.Padding(FMargin(5.0f, 3.0f, 5.0f, 0.0f))
	[
		Column
	];
}
