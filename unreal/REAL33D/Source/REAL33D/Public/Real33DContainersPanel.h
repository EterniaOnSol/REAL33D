#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

/**
 * A container window, transcribed from `game_containers/container.otui`.
 *
 * Inert, and there is no half-measure available. Fusion32 opens a container
 * with SV_CMD_CONTAINER_OPEN and this client's ClientCore does not decode that
 * command at all -- the bridge files it as a Diagnostic and moves on. So there
 * is no container, no name, no capacity and no contents: not "empty", but
 * unknown.
 *
 * The window is still drawn, at the 34px grid with 3px gutters and the 6px
 * padding that file specifies, because the side column has to reserve the space
 * a container will take and because the player should be able to see that this
 * client has a place for one. The paging controls come from the same file and
 * are disabled for the same reason.
 */
class SReal33DContainersPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DContainersPanel)
		: _Columns(4)
		, _Rows(2)
	{}
		/** container.otui flows 34px cells; four across is the classic backpack. */
		SLATE_ARGUMENT(int32, Columns)
		SLATE_ARGUMENT(int32, Rows)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** The height a grid of this many rows needs, including the paging strip. */
	static float ContentHeightFor(int32 Rows);
};
