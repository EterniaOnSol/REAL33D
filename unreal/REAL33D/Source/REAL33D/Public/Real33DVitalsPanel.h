#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SProgressBar;
class STextBlock;

/**
 * Hitpoints, mana and level, drawn with the REAL33D 2D client's own art.
 *
 * The layout is `modules/game_healthinfo/healthinfo.otui` from that client,
 * transcribed: each row is an icon, then a bar made of a fixed trough with a
 * fill clipped to the current value, then the numbers. The spacing, the bar
 * geometry and the 9-slice borders are that file's, not new choices made here.
 *
 * The readout is `current / maximum` where the 2D shows only `current`. Both
 * halves of the pair are what a wire-level fault shows up in -- a max of 65531
 * is visible in the text long before it is visible in a bar -- and this client
 * is the one the protocol is certified against.
 *
 * The bar fill is clamped to the trough because a bar cannot draw past its own
 * ends. The numbers are never clamped: they are printed exactly as the server
 * sent them, so a bad value stays legible instead of being hidden by the UI.
 */
class SReal33DVitalsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DVitalsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Redraws for `Vitals`. Cheap to call every frame: no-ops when unchanged. */
	void SetVitals(const FReal33DPlayerVitals& Vitals);

private:
	/** One icon/bar/number row. Hands back the two widgets that change. */
	TSharedRef<SWidget> MakeRow(
		const FName& SymbolBrush,
		const FName& BarStyle,
		TSharedPtr<SProgressBar>& OutFill,
		TSharedPtr<STextBlock>& OutLabel);

	static float Fraction(uint16 Current, uint16 Maximum);

	TSharedPtr<SProgressBar> HitpointsFill;
	TSharedPtr<SProgressBar> ManaFill;
	TSharedPtr<STextBlock> HitpointsLabel;
	TSharedPtr<STextBlock> ManaLabel;
	TSharedPtr<STextBlock> LevelLabel;

	FReal33DPlayerVitals Last;
	bool bHasDrawnOnce = false;
};
