#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

/**
 * The classic REAL33D2D window frame, transcribed from `30-miniwindow.otui`.
 *
 * Every panel in the side columns is one of these: a 9-sliced body, a 15px
 * header carrying a 12x12 icon, a title and the header buttons, and the
 * contents below. The geometry is that file's -- the 4px body border, the 2px
 * bevel under the header, the 3px inset the contents sit in -- so a panel
 * written against this frame lands where the 2D client puts it.
 *
 * The header buttons are drawn because the 2D draws them. They do nothing: this
 * client has no window manager to minimise or close into, and a button that
 * looked live and then did nothing would be worse than one that plainly is not.
 * Minimise is the exception and is wired, because collapsing a panel needs
 * nothing from the server.
 */
class SReal33DMiniWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DMiniWindow)
		: _Title()
		, _IconBrush(NAME_None)
		, _ContentHeight(150.0f)
		, _Inert(false)
	{}
		/** Shown in the header, in the 2D's own title colour. */
		SLATE_ARGUMENT(FText, Title)
		/** A 12x12 brush name from FReal33DUIStyle, or NAME_None for no icon. */
		SLATE_ARGUMENT(FName, IconBrush)
		/** Height of the contents area below the header, in pixels. */
		SLATE_ARGUMENT(float, ContentHeight)
		/**
		 * True when the panel shows a surface this client cannot fill yet.
		 *
		 * Draws the 2D's own dither over the contents and greys the title, which
		 * is exactly what that client does to a disabled widget. It is a
		 * statement that the data does not exist, not that the panel is broken.
		 */
		SLATE_ARGUMENT(bool, Inert)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply ToggleCollapsed();

	/** One 12x12 header button. Inert unless it is the minimise one. */
	TSharedRef<SWidget> MakeHeaderButton(const FName& Brush, bool bWired);

	TSharedPtr<SWidget> Body;
	float ContentHeight = 150.0f;
	bool bCollapsed = false;
};

/**
 * One 34x34 slot, as `10-items.otui` draws an item and `inventory.otui` an
 * equipment square.
 *
 * Always empty. Fusion32 sends SV_CMD_SET_INVENTORY and this client's
 * ClientCore decodes it only far enough to walk past it -- `player_state.h`
 * says so in as many words -- so there is no item to draw. The slot shows its
 * own background and, for equipment, the 32x32 placeholder the 2D shows for an
 * empty slot. Nothing here invents contents.
 */
class SReal33DSlot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DSlot)
		: _SlotBrush("Real33D.Chrome.ItemSlot")
		, _PlaceholderBrush(NAME_None)
		, _Tooltip()
	{}
		SLATE_ARGUMENT(FName, SlotBrush)
		/** The 32x32 art naming which body part this slot is, if it is one. */
		SLATE_ARGUMENT(FName, PlaceholderBrush)
		SLATE_ARGUMENT(FText, Tooltip)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
