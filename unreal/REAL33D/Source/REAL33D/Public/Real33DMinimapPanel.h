#pragma once

#include "CoreMinimal.h"
#include "Real33DCoords.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;

/**
 * The minimap panel, transcribed from `game_minimap/minimap.otui`.
 *
 * The frame, the compass rose, the floor indicator and the zoom buttons are all
 * the 2D's own art at its own 115x111 geometry, so the panel occupies exactly
 * the space it occupies there.
 *
 * The map surface itself is empty, and deliberately. An automap is a picture of
 * ground the player has walked, accumulated and persisted by the client; this
 * client keeps no such record, and Fusion32 sends nothing resembling one.
 * Painting the current viewport's tiles there instead would be a different
 * thing wearing a minimap's frame -- it would show only what is already on
 * screen and would blank the moment the player moved.
 *
 * What is live is the position: x, y and floor come straight from the server's
 * own viewport anchor, so the panel tells the player exactly where they are
 * even while it cannot yet draw where they have been.
 */
class SReal33DMinimapPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DMinimapPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Updates the coordinate readout. Cheap to call every frame. */
	void SetPosition(const Real33D::FMapPosition& Position, bool bKnown);

	/** minimap.otui gives the panel a fixed 116px body. */
	static constexpr float PanelHeight = 116.0f;

private:
	/** One 20x20 automap button, drawn idle and inert. */
	TSharedRef<SWidget> MakeButton(const FName& Brush, const FText& Tooltip);

	TSharedPtr<STextBlock> PositionLabel;
	TSharedPtr<STextBlock> FloorLabel;

	Real33D::FMapPosition Last;
	bool bLastKnown = false;
	bool bHasDrawnOnce = false;
};
