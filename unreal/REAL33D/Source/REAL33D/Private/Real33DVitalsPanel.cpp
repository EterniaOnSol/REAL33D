#include "Real33DVitalsPanel.h"

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
	// healthinfo.otui: icon, 4px, bar, 10px, number.
	constexpr float IconToBarGap = 4.0f;
	constexpr float BarToTextGap = 10.0f;
	constexpr float RowGap = 4.0f;
	constexpr float TextWidth = 64.0f;
	constexpr int32 TextSize = 10;

	FText Pair(uint16 Current, uint16 Maximum)
	{
		return FText::FromString(FString::Printf(TEXT("%d / %d"),
			static_cast<int32>(Current), static_cast<int32>(Maximum)));
	}
}

float SReal33DVitalsPanel::Fraction(uint16 Current, uint16 Maximum)
{
	if (Maximum == 0)
	{
		return 0.0f;
	}
	return FMath::Clamp(
		static_cast<float>(Current) / static_cast<float>(Maximum), 0.0f, 1.0f);
}

TSharedRef<SWidget> SReal33DVitalsPanel::MakeRow(
	const FName& SymbolBrush,
	const FName& FillBrush,
	TSharedPtr<SBox>& OutFill,
	TSharedPtr<STextBlock>& OutLabel)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(FReal33DUIStyle::SymbolWidth)
			.HeightOverride(FReal33DUIStyle::SymbolHeight)
			[
				SNew(SImage).Image(Style.GetBrush(SymbolBrush))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(IconToBarGap, 0.0f, BarToTextGap, 0.0f)
		.VAlign(VAlign_Center)
		[
			// The trough is the full bar; the fill is a left-aligned box whose
			// width is the only thing that moves. Same construction as the 2D,
			// where `current` is anchored to `total.left` and resized.
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBox)
				.WidthOverride(FReal33DUIStyle::BarWidth)
				.HeightOverride(FReal33DUIStyle::BarHeight)
				[
					SNew(SImage).Image(Style.GetBrush("Real33D.HealthMana.BarBorder"))
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SAssignNew(OutFill, SBox)
				.WidthOverride(0.0f)
				.HeightOverride(FReal33DUIStyle::BarHeight)
				[
					SNew(SImage).Image(Style.GetBrush(FillBrush))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(TextWidth)
			[
				SAssignNew(OutLabel, STextBlock)
				.Text(FText::FromString(TEXT("-- / --")))
				.ColorAndOpacity(Style.GetSlateColor("Real33D.HealthMana.TextColor"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			]
		];
}

void SReal33DVitalsPanel::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeRow("Real33D.HealthMana.HitpointsSymbol",
				"Real33D.HealthMana.HitpointsFill",
				HitpointsFill, HitpointsLabel)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, RowGap, 0.0f, 0.0f)
		[
			MakeRow("Real33D.HealthMana.ManaSymbol",
				"Real33D.HealthMana.ManaFill",
				ManaFill, ManaLabel)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, RowGap, 0.0f, 0.0f)
		.HAlign(HAlign_Right)
		[
			SAssignNew(LevelLabel, STextBlock)
			.Text(FText::FromString(TEXT("Level --")))
			.ColorAndOpacity(Style.GetSlateColor("Real33D.HealthMana.TextColor"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
		]
	];
}

void SReal33DVitalsPanel::SetVitals(const FReal33DPlayerVitals& Vitals)
{
	if (bHasDrawnOnce
		&& Last.bKnown == Vitals.bKnown
		&& Last.Hitpoints == Vitals.Hitpoints
		&& Last.MaxHitpoints == Vitals.MaxHitpoints
		&& Last.Mana == Vitals.Mana
		&& Last.MaxMana == Vitals.MaxMana
		&& Last.Level == Vitals.Level)
	{
		return;
	}
	Last = Vitals;
	bHasDrawnOnce = true;

	if (!Vitals.bKnown)
	{
		// Not logged in, or the session dropped before any player data arrived.
		HitpointsFill->SetWidthOverride(0.0f);
		ManaFill->SetWidthOverride(0.0f);
		HitpointsLabel->SetText(FText::FromString(TEXT("-- / --")));
		ManaLabel->SetText(FText::FromString(TEXT("-- / --")));
		LevelLabel->SetText(FText::FromString(TEXT("Level --")));
		return;
	}

	HitpointsFill->SetWidthOverride(
		FReal33DUIStyle::BarWidth * Fraction(Vitals.Hitpoints, Vitals.MaxHitpoints));
	ManaFill->SetWidthOverride(
		FReal33DUIStyle::BarWidth * Fraction(Vitals.Mana, Vitals.MaxMana));

	HitpointsLabel->SetText(Pair(Vitals.Hitpoints, Vitals.MaxHitpoints));
	ManaLabel->SetText(Pair(Vitals.Mana, Vitals.MaxMana));
	LevelLabel->SetText(FText::FromString(
		FString::Printf(TEXT("Level %d"), static_cast<int32>(Vitals.Level))));
}
