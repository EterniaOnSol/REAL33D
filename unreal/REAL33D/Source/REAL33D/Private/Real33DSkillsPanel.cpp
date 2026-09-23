#include "Real33DSkillsPanel.h"

#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// skills.otui: a SkillButton is 21 tall with a bar and 15 without, rows sit
	// 2 apart, and the bar itself is 5 tall.
	constexpr float RowHeightWithBar = 21.0f;
	constexpr float RowHeightPlain = 15.0f;
	constexpr float RowGap = 2.0f;
	constexpr float BarHeight = 5.0f;
	constexpr float SidePadding = 5.0f;
	constexpr int32 TextSize = 9;

	/** The row order of skills.otui, trimmed to what 7.72 actually sends. */
	enum ERow : int32
	{
		RowLevel, RowExperience, RowHitpoints, RowMana, RowSoul, RowCapacity,
		RowMagic, RowFist, RowClub, RowSword, RowAxe, RowDistance,
		RowShielding, RowFishing
	};

	/** True for the rows the server also sends a percentage for. */
	bool HasBar(int32 Row)
	{
		return Row == RowLevel || Row >= RowMagic;
	}

	FText Number(int64 Value)
	{
		return FText::FromString(FString::Printf(TEXT("%lld"), Value));
	}
}

float SReal33DSkillsPanel::DesiredContentHeight()
{
	float Height = 0.0f;
	for (int32 Row = 0; Row < RowCount; ++Row)
	{
		Height += (HasBar(Row) ? RowHeightWithBar : RowHeightPlain) + RowGap;
	}
	return Height;
}

TSharedRef<SWidget> SReal33DSkillsPanel::MakeRow(const FText& Name, int32 Index, bool bWithBar)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	TSharedRef<SVerticalBox> Row = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(Name)
				.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(Values[Index], STextBlock)
				// Until SV_CMD_PLAYER_DATA or SV_CMD_PLAYER_SKILLS arrives there
				// is no value, and "--" says so. A zero here would be a claim.
				.Text(FText::FromString(TEXT("--")))
				.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			]
		];

	if (bWithBar)
	{
		Row->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 1.0f, 0.0f, 0.0f))
			[
				SNew(SBox)
				.HeightOverride(BarHeight)
				[
					// Takes the row's width rather than a number computed from
					// the panel's, so the bar is right whatever the column ends
					// up being. skills.otui fills these green and paints the
					// level bar red; both colours are that file's.
					SAssignNew(Bars[Index], SProgressBar)
					.Style(&FReal33DUIStyle::Get().GetWidgetStyle<FProgressBarStyle>(
						"Real33D.Bar.Thin"))
					.FillColorAndOpacity(Index == RowLevel
						? FLinearColor(0.78f, 0.13f, 0.13f, 1.0f)
						: FLinearColor(0.18f, 0.63f, 0.25f, 1.0f))
					.Percent(0.0f)
					.BorderPadding(FVector2D::ZeroVector)
				]
			];
	}

	return SNew(SBox)
		.HeightOverride(bWithBar ? RowHeightWithBar : RowHeightPlain)
		[
			Row
		];
}

void SReal33DSkillsPanel::Construct(const FArguments& InArgs)
{
	static const TCHAR* const Names[RowCount] = {
		TEXT("Level"), TEXT("Experience"), TEXT("Hit Points"), TEXT("Mana"),
		TEXT("Soul Points"), TEXT("Capacity"), TEXT("Magic Level"),
		TEXT("Fist Fighting"), TEXT("Club Fighting"), TEXT("Sword Fighting"),
		TEXT("Axe Fighting"), TEXT("Distance Fighting"), TEXT("Shielding"),
		TEXT("Fishing") };

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Row = 0; Row < RowCount; ++Row)
	{
		Rows->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, RowGap))
			[
				MakeRow(FText::FromString(Names[Row]), Row, HasBar(Row))
			];
	}

	ChildSlot
	.Padding(FMargin(SidePadding, 0.0f, SidePadding, 0.0f))
	[
		Rows
	];
}

void SReal33DSkillsPanel::SetValues(
	const FReal33DPlayerVitals& Vitals, const FReal33DPlayerSkills& Skills)
{
	if (bHasDrawnOnce
		&& LastVitals.bKnown == Vitals.bKnown
		&& LastVitals.Level == Vitals.Level
		&& LastVitals.LevelPercent == Vitals.LevelPercent
		&& LastVitals.Experience == Vitals.Experience
		&& LastVitals.Hitpoints == Vitals.Hitpoints
		&& LastVitals.MaxHitpoints == Vitals.MaxHitpoints
		&& LastVitals.Mana == Vitals.Mana
		&& LastVitals.MaxMana == Vitals.MaxMana
		&& LastVitals.SoulPoints == Vitals.SoulPoints
		&& LastVitals.Capacity == Vitals.Capacity
		&& LastVitals.MagicLevel == Vitals.MagicLevel
		&& LastVitals.MagicLevelPercent == Vitals.MagicLevelPercent
		&& LastSkills.bKnown == Skills.bKnown
		&& FMemory::Memcmp(&LastSkills, &Skills, sizeof(FReal33DPlayerSkills)) == 0)
	{
		return;
	}
	LastVitals = Vitals;
	LastSkills = Skills;
	bHasDrawnOnce = true;

	const auto Set = [this](int32 Row, const FText& Text, int32 Percent, bool bKnown)
	{
		if (Values[Row].IsValid())
		{
			Values[Row]->SetText(bKnown ? Text : FText::FromString(TEXT("--")));
		}
		if (Bars[Row].IsValid())
		{
			// Clamped because a percentage cannot overrun a bar.
			Bars[Row]->SetPercent(bKnown
				? FMath::Clamp(Percent / 100.0f, 0.0f, 1.0f) : 0.0f);
		}
	};

	Set(RowLevel, Number(Vitals.Level), Vitals.LevelPercent, Vitals.bKnown);
	Set(RowExperience, Number(Vitals.Experience), 0, Vitals.bKnown);
	Set(RowHitpoints, FText::FromString(FString::Printf(TEXT("%d"),
		static_cast<int32>(Vitals.Hitpoints))), 0, Vitals.bKnown);
	Set(RowMana, FText::FromString(FString::Printf(TEXT("%d"),
		static_cast<int32>(Vitals.Mana))), 0, Vitals.bKnown);
	Set(RowSoul, Number(Vitals.SoulPoints), 0, Vitals.bKnown);
	Set(RowCapacity, Number(Vitals.Capacity), 0, Vitals.bKnown);
	Set(RowMagic, Number(Vitals.MagicLevel), Vitals.MagicLevelPercent, Vitals.bKnown);

	const FReal33DSkill* const Seven[] = {
		&Skills.Fist, &Skills.Club, &Skills.Sword, &Skills.Axe,
		&Skills.Distance, &Skills.Shielding, &Skills.Fishing };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Seven); ++Index)
	{
		Set(RowFist + Index, Number(Seven[Index]->Level),
			Seven[Index]->Percent, Skills.bKnown);
	}
}
