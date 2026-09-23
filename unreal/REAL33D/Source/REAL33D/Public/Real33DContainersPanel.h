#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DPanelChrome.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

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
 * specifies. Nesting needs nothing extra -- the server opens a nested
 * container as another numbered one and says it has a parent -- so the "go up"
 * arrow is shown for those, disabled until CL_CMD_UP_CONTAINER is wired.
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
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuilds the windows if, and only if, the containers actually changed. */
	void SetContainers(const TArray<FReal33DContainer>& Containers);

private:
	/** One container window: its header, its grid, and its footer. */
	TSharedRef<SWidget> MakeContainer(const FReal33DContainer& Container);

	int32 Columns = 4;
	TSharedPtr<SVerticalBox> Windows;
	FReal33DOnItemDropped OnItemDropped;
	FReal33DOnSlotUsed OnSlotUsed;
	FReal33DOnSlotPicked OnSlotPicked;

	/** What is currently drawn, so an unchanged set is not rebuilt. */
	TArray<FReal33DContainer> Drawn;
};
