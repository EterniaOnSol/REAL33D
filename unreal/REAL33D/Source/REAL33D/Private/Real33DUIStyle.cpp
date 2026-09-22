#include "Real33DUIStyle.h"

#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FReal33DUIStyle::Instance = nullptr;

namespace
{
	// healthinfo.otui gives the 9-slice borders in pixels; FSlateBoxBrush wants
	// them as fractions of the image. Both bar images are 94 wide.
	constexpr float BarPixels = 94.0f;

	FMargin BorderMargin(float LeftPixels, float RightPixels)
	{
		return FMargin(LeftPixels / BarPixels, 0.0f, RightPixels / BarPixels, 0.0f);
	}
}

void FReal33DUIStyle::Initialize()
{
	if (Instance.IsValid())
	{
		return;
	}
	Instance = Create();
	FSlateStyleRegistry::RegisterSlateStyle(*Instance);
}

void FReal33DUIStyle::Shutdown()
{
	if (!Instance.IsValid())
	{
		return;
	}
	FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);
	Instance.Reset();
}

const ISlateStyle& FReal33DUIStyle::Get()
{
	check(Instance.IsValid());
	return *Instance;
}

FName FReal33DUIStyle::GetStyleSetName()
{
	static const FName Name(TEXT("Real33DUIStyle"));
	return Name;
}

TSharedRef<FSlateStyleSet> FReal33DUIStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>(GetStyleSetName());
	Style->SetContentRoot(FPaths::ProjectDir() / TEXT("Resources/UI"));

	const FVector2D SymbolSize(SymbolWidth, SymbolHeight);
	const FVector2D BarSize(BarWidth, BarHeight);

	// The two icons. Plain image brushes: they are never stretched.
	Style->Set("Real33D.HealthMana.HitpointsSymbol", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("HealthMana/hitpoints_symbol"), TEXT(".png")),
		SymbolSize));
	Style->Set("Real33D.HealthMana.ManaSymbol", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("HealthMana/mana_symbol"), TEXT(".png")),
		SymbolSize));

	// The empty trough both bars sit in. 9-slice 6/6, per healthinfo.otui.
	Style->Set("Real33D.HealthMana.BarBorder", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("HealthMana/hitpoints_manapoints_bar_border"), TEXT(".png")),
		BarSize, BorderMargin(6.0f, 6.0f)));

	// The fills. 9-slice 5/7: these are stretched to the current value, so the
	// margins are what keeps the rounded caps from smearing.
	Style->Set("Real33D.HealthMana.HitpointsFill", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("HealthMana/hitpoints_bar_filled"), TEXT(".png")),
		BarSize, BorderMargin(5.0f, 7.0f)));
	Style->Set("Real33D.HealthMana.ManaFill", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("HealthMana/mana_bar_filled"), TEXT(".png")),
		BarSize, BorderMargin(5.0f, 7.0f)));

	// #c0c0c0ff, the label colour healthinfo.otui uses for both readouts.
	Style->Set("Real33D.HealthMana.TextColor",
		FSlateColor(FLinearColor(FColor(0xC0, 0xC0, 0xC0, 0xFF))));

	return Style;
}
