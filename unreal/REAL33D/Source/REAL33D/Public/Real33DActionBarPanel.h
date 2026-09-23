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

/**
 * The generic object-use controls, from `game_hotkeys/hotkeys_manager.otui`.
 *
 * These are the item-use controls that file defines and nothing else: an item
 * preview, Select object, Clear object, and the four mutually exclusive use
 * modes -- on yourself, on target, with crosshair, at cursor position. The
 * widths, the 2px stacking and the preview placement are that file's.
 *
 * They are deliberately generic. The 2D client also ships shortcuts that
 * hard-code a health or mana potion; none of those is ported, because a control
 * that promises to drink a specific item is a claim about an inventory this
 * client cannot see.
 *
 * Every control is inert. Using an object needs a client command this client
 * does not send and an item identity it does not hold. The panel exists so the
 * shape of the feature is visible and so the wiring, when it lands, has a place
 * to land rather than a new design to argue about.
 */
class SReal33DHotkeyPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DHotkeyPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** The height the controls need, for the window that hosts them. */
	static float DesiredContentHeight();

private:
	/** One disabled push button at hotkeys_manager.otui's 128px width. */
	TSharedRef<SWidget> MakeButton(const FText& Label);

	/** One disabled use-mode box, drawn as the 2D's unchecked ButtonBox. */
	TSharedRef<SWidget> MakeModeBox(const FText& Label);
};
