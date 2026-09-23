#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

/**
 * An action bar, transcribed from `game_actionbar/otui/actionbar.otui`.
 *
 * The shell is complete: the 1px-bordered background strip, the 34x34 slots,
 * the paging arrows at each end, and the 36px column width the game interface
 * reserves for the vertical bars. That is what an action bar is, and it is
 * drawn here at the 2D's geometry.
 *
 * The slots are empty and the arrows are dead. An action button binds either a
 * spell or an object, and this client has neither: Fusion32 sends no spell
 * list, and the inventory commands that would name an object are decoded only
 * far enough to skip. There is also no cooldown, because nothing on the wire
 * carries one. Drawing a filled slot, a greyed cooldown sweep or an active
 * spell border would each be this client inventing a state the server never
 * described, so none of them is drawn.
 *
 * There is deliberately no object-use panel beside this one. An earlier
 * revision docked the `hotkeys_manager.otui` controls -- Select object, Use on
 * yourself, With crosshair and the rest -- into a side column. Those belong to
 * the 2D client's hotkey *configuration* dialog, not to a permanent HUD, and
 * REAL33D 3D uses objects the way Tibia does: an ordinary use through the
 * mouse, and a use-with that enters a temporary crosshair mode until the next
 * click picks a target. A panel of latching mode boxes is the wrong shape for
 * that, so it is gone rather than left sitting there waiting to be wired.
 */
class SReal33DActionBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DActionBar)
		: _SlotCount(10)
		, _Vertical(false)
	{}
		SLATE_ARGUMENT(int32, SlotCount)
		/** True for the left and right columns, false for the bottom bar. */
		SLATE_ARGUMENT(bool, Vertical)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** actionbar.otui: the bar is 36 tall when shown, slots plus padding. */
	static constexpr float BarThickness = 36.0f;
};
