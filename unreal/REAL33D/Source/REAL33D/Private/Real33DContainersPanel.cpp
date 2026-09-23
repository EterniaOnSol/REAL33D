#include "Real33DContainersPanel.h"

#include "Real33DPanelChrome.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// container.otui: cell-size 34 34, cell-spacing 3, padding 6, and a 20px
	// paging strip along the bottom.
	constexpr float Cell = FReal33DUIStyle::SlotSize;
	constexpr float Gap = FReal33DUIStyle::SlotSpacing;
	constexpr float Padding = 6.0f;
	constexpr float PageStripHeight = 20.0f;
	constexpr float PageButtonSize = 18.0f;
	constexpr int32 TextSize = 8;
}

float SReal33DContainersPanel::ContentHeightFor(int32 Rows)
{
	return Padding * 2.0f + Rows * Cell + FMath::Max(0, Rows - 1) * Gap
		+ PageStripHeight;
}

void SReal33DContainersPanel::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	for (int32 Row = 0; Row < InArgs._Rows; ++Row)
	{
		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);
		for (int32 Column = 0; Column < InArgs._Columns; ++Column)
		{
			Line->AddSlot()
				.AutoWidth()
				.Padding(FMargin(0.0f, 0.0f, Column + 1 < InArgs._Columns ? Gap : 0.0f, 0.0f))
				[
					SNew(SReal33DSlot)
					.SlotBrush("Real33D.Chrome.ItemSlot")
					.Tooltip(FText::FromString(TEXT("Containers are not decoded yet")))
				];
		}
		Grid->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, Row + 1 < InArgs._Rows ? Gap : 0.0f))
			[
				Line
			];
	}

	// The paging strip. container.otui hides it until a container needs more
	// than one page; it is shown here, disabled, because it is part of what the
	// window is and hiding it would make the shell look smaller than the real
	// thing will be.
	const auto PageButton = [&Style](const FName& Brush)
	{
		return SNew(SBox)
			.WidthOverride(PageButtonSize)
			.HeightOverride(PageButtonSize)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage).Image(Style.GetBrush(Brush))
				]
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
					.Visibility(EVisibility::HitTestInvisible)
				]
			];
	};

	ChildSlot
	.Padding(FMargin(Padding))
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			Grid
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
		[
			SNew(SBox)
			.HeightOverride(PageStripHeight)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					PageButton("Real33D.Chrome.PageLeft")
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("No container open")))
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Disabled"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					PageButton("Real33D.Chrome.PageRight")
				]
			]
		]
	];
}
