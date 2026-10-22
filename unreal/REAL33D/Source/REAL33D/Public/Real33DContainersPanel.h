#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DPanelChrome.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

/**
 * Fired when a container's own header button is pressed.
 *
 * Carries the server's 0-based open-container number, never a panel index:
 * the two differ as soon as a container is closed out of the middle, and the
 * number is what `CCloseContainer` and `CUpContainer` index their table with.
 */
DECLARE_DELEGATE_OneParam(FReal33DOnContainerClosed, uint8);
DECLARE_DELEGATE_OneParam(FReal33DOnContainerUp, uint8);

/**
 * The open containers, transcribed from `game_containers/container.otui`.
 *
 * Live. Fusion32 describes a container with SV_CMD_CONTAINER and keeps it
 * current with SV_CMD_CREATE_IN_CONTAINER, SV_CMD_CHANGE_IN_CONTAINER and
 * SV_CMD_DELETE_IN_CONTAINER; all five are decoded and stored in WorldState,
 * so what is drawn here is the server's own contents in the server's own
 * order. Nothing is invented and nothing is cached: a container the server has
 * not described has no panel.
 *
 * One window per open container, in container-number order, each with its own
 * name and its own 34px grid at the 3px gutters and 6px padding that file
 * specifies. Nesting needs nothing extra: the server opens a nested container
 * as another numbered one and says it has a parent.
 *
 * Each header carries the two dedicated 7.72 container controls. Close is
 * always offered; the up arrow only when the server said this container has a
 * parent, because `CUpContainer` has nothing to answer for one that does not.
 * Both are intents and neither touches this panel: pressing them sends a
 * command and nothing more, and the windows move when, and only when, the
 * server's own SV_CMD_CONTAINER or SV_CMD_CLOSE_CONTAINER arrives.
 */
class SReal33DContainersPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DContainersPanel)
		: _Columns(4)
	{}
		/** container.otui flows 34px cells; four across is the classic bag. */
		SLATE_ARGUMENT(int32, Columns)
		/** Fired when an object is dropped on one of the cells. */
		SLATE_EVENT(FReal33DOnItemDropped, OnItemDropped)
		/** Right-click use, and shift-right-click use-with. */
		SLATE_EVENT(FReal33DOnSlotUsed, OnSlotUsed)
		/** Offered a left-click so a pending use-with can take its target. */
		SLATE_EVENT(FReal33DOnSlotPicked, OnSlotPicked)
		/** The header's close button. CL_CMD_CLOSE_CONTAINER. */
		SLATE_EVENT(FReal33DOnContainerClosed, OnContainerClosed)
		/** The header's up arrow, offered only when the server says there is a
		 *  parent. CL_CMD_UP_CONTAINER. */
		SLATE_EVENT(FReal33DOnContainerUp, OnContainerUp)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuilds the windows if, and only if, the containers actually changed. */
	void SetContainers(const TArray<FReal33DContainer>& Containers);

private:
	/** One container window: its header, its grid, and its footer. */
	TSharedRef<SWidget> MakeContainer(const FReal33DContainer& Container);

	/**
	 * One 14x14 header button, bound to a container NUMBER rather than to a
	 * position in this panel.
	 *
	 * The number is captured into the handler, so a window that moves when a
	 * container below it closes still asks about the container it is drawing.
	 */
	TSharedRef<SWidget> MakeHeaderButton(FName Brush, const FText& Tip,
		uint8 Number, bool bUp);

	FReply HandleClose(uint8 Number);
	FReply HandleUp(uint8 Number);

	/**
	 * Rolls one container's grid away, leaving its header.
	 *
	 * Purely local: a collapsed bag is still open on the server and still a
	 * legal drop target the moment it is rolled back out, so nothing is sent
	 * and no container state is touched. This is the same thing
	 * `SReal33DMiniWindow` does for a side panel, which is why it collapses to
	 * nothing rather than hiding: the bags below slide up.
	 */
	FReply HandleToggleCollapsed(uint8 Number);

	/**
	 * Which containers the player has rolled away, by server number.
	 *
	 * Kept across the rebuilds `SetContainers` does, because a container whose
	 * contents changed must not pop back open underneath the player's hands.
	 * Entries for containers that are no longer open are dropped, so reopening
	 * a bag gives it back expanded.
	 */
	TSet<uint8> Collapsed;

	int32 Columns = 4;
	TSharedPtr<SVerticalBox> Windows;
	FReal33DOnItemDropped OnItemDropped;
	FReal33DOnSlotUsed OnSlotUsed;
	FReal33DOnSlotPicked OnSlotPicked;
	FReal33DOnContainerClosed OnContainerClosed;
	FReal33DOnContainerUp OnContainerUp;

	/** What is currently drawn, so an unchanged set is not rebuilt. */
	TArray<FReal33DContainer> Drawn;

	/** The server's current set, kept so a local collapse can redraw without
	 *  waiting for the next frame's push. */
	TArray<FReal33DContainer> Live;
};
