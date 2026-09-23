#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class SScrollBox;

/**
 * The battle list, transcribed from `game_battle/battle.otui`.
 *
 * The rows are live. Every creature the server has described is already an
 * actor in this client's world, carrying the name Fusion32 introduced it with
 * and the health percentage Fusion32 last reported, so a row is a view of that
 * and nothing more. The list keeps no creature record of its own.
 *
 * The filter buttons above the rows are shells. Fusion32's creature descriptor
 * says nothing about vocation, party or guild -- `CreatureThing` in
 * `worldstate.h` is the whole of what arrives -- so this client cannot tell a
 * knight from a druid and a filter for one could only ever lie. They are drawn
 * because battle.otui draws them, and disabled because the data is not there.
 *
 * Ordering is nearest-first, which the 2D also defaults to, and is computed
 * from positions the server owns.
 */
class SReal33DBattlePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DBattlePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuilds the rows if, and only if, the list actually changed. */
	void SetEntries(const TArray<FReal33DBattleEntry>& Entries);

private:
	/** The 12 filter toggles battle.otui lays out in a 6-wide grid. */
	TSharedRef<SWidget> BuildFilters();

	/** One creature row: name over a health bar, as the 2D draws it. */
	TSharedRef<SWidget> MakeRow(const FReal33DBattleEntry& Entry);

	/** Colours a name by how hurt the creature is, as the world actor does. */
	static FLinearColor HealthColour(uint8 Percent);

	TSharedPtr<SVerticalBox> Rows;
	TSharedPtr<SScrollBox> Scroller;

	/** What is currently drawn, so an unchanged list is not rebuilt. */
	TArray<FReal33DBattleEntry> Drawn;
};
