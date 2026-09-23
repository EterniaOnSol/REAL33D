#include "Real33DBattlePanel.h"

#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// battle.otui: a 6-wide grid of 20x20 filters, 2 apart, 130 wide overall.
	constexpr float FilterSize = 20.0f;
	constexpr float FilterGap = 2.0f;
	constexpr int32 FilterColumns = 6;

	// A battle row in the 2D is a name over a 5px health bar.
	constexpr float RowHeight = 20.0f;
	constexpr float HealthBarHeight = 5.0f;
	constexpr int32 TextSize = 8;
}

FLinearColor SReal33DBattlePanel::HealthColour(uint8 Percent)
{
	// The same bands Real33DCreatureActor colours a name tag with, so a
	// creature reads the same in the world and in the list. Fusion32 defines no
	// colours; these are this project's, and are stated as such there too.
	if (Percent > 92) return FLinearColor(0.00f, 0.75f, 0.00f, 1.0f);
	if (Percent > 60) return FLinearColor(0.50f, 0.78f, 0.00f, 1.0f);
	if (Percent > 30) return FLinearColor(0.85f, 0.75f, 0.00f, 1.0f);
	if (Percent > 8)  return FLinearColor(0.90f, 0.45f, 0.00f, 1.0f);
	return FLinearColor(0.85f, 0.10f, 0.10f, 1.0f);
}

TSharedRef<SWidget> SReal33DBattlePanel::BuildFilters()
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// battle.otui lists twelve. Only the four this client could ever tell apart
	// carry their own art; the rest reuse the empty button, exactly as that
	// file does for a filter with no icon of its own.
	static const TCHAR* const Icons[] = {
		TEXT("Real33D.Battle.Players"), TEXT("Real33D.Battle.Monsters"),
		TEXT("Real33D.Battle.NPCs"), TEXT("Real33D.Battle.Skulls"),
		TEXT("Real33D.Battle.Party"), TEXT("Real33D.Battle.Icon") };
	static const TCHAR* const Tips[] = {
		TEXT("Hide players"), TEXT("Hide monsters"), TEXT("Hide NPCs"),
		TEXT("Hide non-skull players"), TEXT("Hide party members"),
		TEXT("Hide own guild members") };

	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	TSharedPtr<SHorizontalBox> Row;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Icons); ++Index)
	{
		if (Index % FilterColumns == 0)
		{
			Grid->AddSlot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, FilterGap))
				[
					SAssignNew(Row, SHorizontalBox)
				];
		}
		Row->AddSlot()
			.AutoWidth()
			.Padding(FMargin(0.0f, 0.0f, FilterGap, 0.0f))
			[
				SNew(SBox)
				.WidthOverride(FilterSize)
				.HeightOverride(FilterSize)
				.ToolTipText(FText::FromString(FString::Printf(
					TEXT("%s (not available)"), Tips[Index])))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage).Image(Style.GetBrush("Real33D.Battle.FilterIdle"))
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SImage).Image(Style.GetBrush(FName(Icons[Index])))
					]
					+ SOverlay::Slot()
					[
						// Fusion32's creature descriptor carries no vocation,
						// party or guild, so none of these could be honoured.
						SNew(SImage)
						.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
						.Visibility(EVisibility::HitTestInvisible)
					]
				]
			];
	}
	return Grid;
}

TSharedRef<SWidget> SReal33DBattlePanel::MakeRow(const FReal33DBattleEntry& Entry)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SBox)
		.HeightOverride(RowHeight)
		.ToolTipText(FText::FromString(FString::Printf(
			TEXT("%s  -  %d%%"), *Entry.Name, static_cast<int32>(Entry.HealthPercent))))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Entry.Name))
				.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				.Clipping(EWidgetClipping::ClipToBounds)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 1.0f, 0.0f, 0.0f))
			[
				SNew(SBox)
				.HeightOverride(HealthBarHeight)
				[
					// Fills the row rather than a width worked out from the
					// panel's, so the bar is correct at any column width.
					SNew(SProgressBar)
					.Style(&Style.GetWidgetStyle<FProgressBarStyle>("Real33D.Bar.Thin"))
					.FillColorAndOpacity(HealthColour(Entry.HealthPercent))
					.Percent(FMath::Clamp(Entry.HealthPercent / 100.0f, 0.0f, 1.0f))
					.BorderPadding(FVector2D::ZeroVector)
				]
			]
		];
}

void SReal33DBattlePanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.0f, 2.0f, 0.0f, 4.0f))
		[
			BuildFilters()
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(Scroller, SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(Rows, SVerticalBox)
			]
		]
	];
}

void SReal33DBattlePanel::SetEntries(const TArray<FReal33DBattleEntry>& Entries)
{
	// Rebuilt only on a real change. The list is read every frame off actors
	// that move every frame, so comparing first is what keeps this from
	// re-creating a dozen widgets sixty times a second.
	bool bSame = Entries.Num() == Drawn.Num();
	for (int32 Index = 0; bSame && Index < Entries.Num(); ++Index)
	{
		bSame = Entries[Index].CreatureId == Drawn[Index].CreatureId
			&& Entries[Index].HealthPercent == Drawn[Index].HealthPercent
			&& Entries[Index].Name == Drawn[Index].Name;
	}
	if (bSame)
	{
		return;
	}
	Drawn = Entries;

	if (!Rows.IsValid())
	{
		return;
	}
	Rows->ClearChildren();

	if (Entries.Num() == 0)
	{
		// An empty list is a fact about the world, not a failure, and saying so
		// is better than a blank panel that reads as a broken one.
		Rows->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("No creatures in view")))
				.ColorAndOpacity(FReal33DUIStyle::Get().GetSlateColor("Real33D.Text.Disabled"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			];
		return;
	}

	for (const FReal33DBattleEntry& Entry : Entries)
	{
		Rows->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 3.0f))
			[
				MakeRow(Entry)
			];
	}
}
