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
 * borders from the corresponding `.otui`, converted from pixels to the
 * normalised fractions `FSlateBoxBrush` expects. Where the 2D uses
 * `image-clip` to pick one cell out of a sprite sheet, the brush here carries
 * the same rectangle as a UV region, so a single PNG still serves every state
 * and no art is sliced up on disk.
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

	// ------------------------------------------------- shared classic geometry
	//
	// Every value below is read out of the REAL33D2D `.otui` that owns the
	// panel in question, not chosen here. They live together so a panel and the
	// art it lays out cannot drift apart.

	/** GameSidePanel width, from gameinterface.otui. */
	static constexpr float SidePanelWidth = 176.0f;
	/** MiniWindow default width, from 30-miniwindow.otui. */
	static constexpr float MiniWindowWidth = 192.0f;
	/** MiniWindow header height, and the height it collapses to. */
	static constexpr float MiniWindowHeaderHeight = 15.0f;
	/** The 12x12 header buttons, drawn from a sheet of 14x14 cells. */
	static constexpr float MiniWindowButtonSize = 12.0f;
	/** Item and equipment slots are 34x34 with a 3px 9-slice border. */
	static constexpr float SlotSize = 34.0f;
	/** The 32x32 placeholder drawn centred inside an empty equipment slot. */
	static constexpr float SlotIconSize = 32.0f;
	/** Container grid: 34px cells, 3px apart, from container.otui. */
	static constexpr float SlotSpacing = 3.0f;
	/** Action bar slots are the same 34x34 as an item slot. */
	static constexpr float ActionSlotSize = 34.0f;
	/** The left/right action columns, from gameinterface.otui. */
	static constexpr float ActionColumnWidth = 36.0f;
	/** One condition icon in player-state-flags.png. */
	static constexpr float ConditionIconSize = 9.0f;
	/** The 20x20 control buttons in the main panel. */
	static constexpr float ControlButtonSize = 20.0f;
	/** Minimap surface, from minimap.otui: a 115x111 frame inset by 1. */
	static constexpr float MinimapFrameWidth = 115.0f;
	static constexpr float MinimapFrameHeight = 111.0f;

	/**
	 * How solid a panel's background is drawn.
	 *
	 * 0.6, which is the `opacity` `30-miniwindow.otui` gives the background of
	 * its own `PhantomMiniWindow` -- so this is the 2D client's number, not a
	 * new choice. It matters more here than it does there: the 3D scene is a
	 * camera into the world rather than a fixed tile grid, and a solid column
	 * of panels down each flank walls off a large part of what the player is
	 * looking at.
	 *
	 * Applied to backgrounds and chrome only. Text, icons, bars and item slots
	 * stay fully opaque: a readout that has to be legible against a moving
	 * scene cannot be faded, and a half-transparent number is a worse readout
	 * than a solid one on a faded panel.
	 */
	static constexpr float PanelOpacity = 0.6f;

	/** The background tint that opacity becomes, for SBorder and SImage. */
	static FLinearColor PanelTint() { return FLinearColor(1.0f, 1.0f, 1.0f, PanelOpacity); }

	/**
	 * How strongly the dither is laid over a control that is inert.
	 *
	 * Half. The 2D uses `ditherpattern` to grey a widget out, and a grey-out
	 * has to leave the icon legible -- the player still needs to know which
	 * control it is they cannot use. Drawn at full strength the checkerboard
	 * simply erased the 20x20 buttons underneath it.
	 */
	static FLinearColor DitherTint() { return FLinearColor(1.0f, 1.0f, 1.0f, 0.5f); }

private:
	static TSharedRef<FSlateStyleSet> Create();
	static TSharedPtr<FSlateStyleSet> Instance;
};
