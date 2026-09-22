#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ISlateStyle;

/**
 * The Slate style carrying REAL33D's UI art.
 *
 * The images are the ones the REAL33D 2D client draws, copied verbatim from
 * `data/images/` in that repository into `Resources/UI/` here, so both clients
 * present the same face to the player rather than two designs that happen to
 * show the same numbers.
 *
 * They are loaded as PNG files from `Resources/` rather than imported as
 * `.uasset` textures. That keeps the promise the rest of this module makes --
 * presentation is built in C++, with no cooked asset to regenerate whenever the
 * art changes -- and it means updating a bar is a file copy, not an editor
 * session. It is the same mechanism the engine's own editor styles use.
 *
 * Brush sizes are the images' native pixel dimensions, so the art is drawn at
 * 1:1 exactly as the 2D client draws it. Box-brush margins are the 9-slice
 * borders from `healthinfo.otui`, converted from pixels to the normalised
 * fractions `FSlateBoxBrush` expects.
 */
class FReal33DUIStyle
{
public:
	/** Builds and registers the style. Safe to call more than once. */
	static void Initialize();

	/** Unregisters and destroys the style. Safe to call when not initialised. */
	static void Shutdown();

	static const ISlateStyle& Get();
	static FName GetStyleSetName();

	// Native pixel geometry of the health/mana art, shared with the widgets so
	// a layout cannot drift from the images it is laying out.
	static constexpr float SymbolWidth = 12.0f;
	static constexpr float SymbolHeight = 11.0f;
	static constexpr float BarWidth = 94.0f;
	static constexpr float BarHeight = 11.0f;

private:
	static TSharedRef<FSlateStyleSet> Create();
	static TSharedPtr<FSlateStyleSet> Instance;
};
