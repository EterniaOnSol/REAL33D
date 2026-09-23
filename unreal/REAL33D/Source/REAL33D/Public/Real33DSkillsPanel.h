#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SProgressBar;
class STextBlock;

/**
 * The character sheet, transcribed from `game_skills/skills.otui`.
 *
 * Every row is live. SV_CMD_PLAYER_DATA and SV_CMD_PLAYER_SKILLS are both
 * decoded by ClientCore and both stored in WorldState, so level, experience,
 * hitpoints, mana, soul, capacity, magic level and the seven fighting skills
 * are all server-owned values this panel only formats.
 *
 * It stops at the seven skills. The 2D client can draw criticals, leech,
 * momentum and the rest, but Fusion32 has no command that carries any of them,
 * so a row for one could only ever show a number this client made up. The 2D's
 * ability to display a field is not evidence the server sends it.
 *
 * The percent bars are the `SkillPercentPanel` of that file: a 5px bar under
 * the row, filled to the percentage the server reports towards the next level.
 */
class SReal33DSkillsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DSkillsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Redraws for the values given. Cheap to call every frame. */
	void SetValues(const FReal33DPlayerVitals& Vitals, const FReal33DPlayerSkills& Skills);

	/** The height the rows need, so the window can be sized to its contents. */
	static float DesiredContentHeight();

private:
	/** One name/value row, optionally with a percent bar under it. */
	TSharedRef<SWidget> MakeRow(const FText& Name, int32 Index, bool bWithBar);

	static constexpr int32 RowCount = 14;

	TSharedPtr<STextBlock> Values[RowCount];
	TSharedPtr<SProgressBar> Bars[RowCount];

	FReal33DPlayerVitals LastVitals;
	FReal33DPlayerSkills LastSkills;
	bool bHasDrawnOnce = false;
};
