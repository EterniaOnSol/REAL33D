#include "Real33DVitalsPanel.h"

#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// healthinfo.otui: icon, 4px, bar, 10px, number.
	constexpr float IconToBarGap = 4.0f;
	constexpr float BarToTextGap = 10.0f;
	constexpr float RowGap = 4.0f;
	// 44, which is the `width` healthinfo.otui gives the readout label. It was
	// 64 here, and with the bar also pinned at its native 94 the row came to
	// 184 inside a 176px column: the numbers were clipped off the edge.
	constexpr float TextWidth = 44.0f;
	constexpr int32 TextSize = 9;

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
	const FName& BarStyle,
	TSharedPtr<SProgressBar>& OutFill,
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
		.FillWidth(1.0f)
		.Padding(IconToBarGap, 0.0f, BarToTextGap, 0.0f)
		.VAlign(VAlign_Center)
		[
			// The bar takes whatever width the column leaves between the icon
			// and the numbers, exactly as `total` does in healthinfo.otui where
			// it is anchored to both. The 9-sliced fill stretches inside it, so
			// the rounded caps survive at any width.
			SNew(SBox)
			.HeightOverride(FReal33DUIStyle::BarHeight)
			[
				SAssignNew(OutFill, SProgressBar)
				.Style(&Style.GetWidgetStyle<FProgressBarStyle>(BarStyle))
				.Percent(0.0f)
				.BorderPadding(FVector2D::ZeroVector)
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
			MakeRow("Real33D.HealthMana.HitpointsSymbol", "Real33D.Bar.Hitpoints",
				HitpointsFill, HitpointsLabel)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, RowGap, 0.0f, 0.0f)
		[
			MakeRow("Real33D.HealthMana.ManaSymbol", "Real33D.Bar.Mana",
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
		HitpointsFill->SetPercent(0.0f);
		ManaFill->SetPercent(0.0f);
		HitpointsLabel->SetText(FText::FromString(TEXT("-- / --")));
		ManaLabel->SetText(FText::FromString(TEXT("-- / --")));
		LevelLabel->SetText(FText::FromString(TEXT("Level --")));
		return;
	}

	HitpointsFill->SetPercent(Fraction(Vitals.Hitpoints, Vitals.MaxHitpoints));
	ManaFill->SetPercent(Fraction(Vitals.Mana, Vitals.MaxMana));

	HitpointsLabel->SetText(Pair(Vitals.Hitpoints, Vitals.MaxHitpoints));
	ManaLabel->SetText(Pair(Vitals.Mana, Vitals.MaxMana));
	LevelLabel->SetText(FText::FromString(
		FString::Printf(TEXT("Level %d"), static_cast<int32>(Vitals.Level))));
}
