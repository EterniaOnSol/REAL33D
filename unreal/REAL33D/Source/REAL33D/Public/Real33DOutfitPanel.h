#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DOutfitSelection.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;
class SButton;

/**
 * The 7.72 outfit chooser.
 *
 * Registered in `FReal33DPanelRegistry` under `Real33D::PanelIds::Outfit`, so
 * showing and hiding it goes through the one mechanism every other surface
 * uses. A centred modal like the text editor and the trade window.
 *
 * FUSION32 OPENS THIS WINDOW AND NOTHING ELSE MAY. The band of selectable ids
 * is computed by the server from the character's sex and premium right, and the
 * 7.72 *game* protocol never carries either: the sex byte lives in the
 * querymanager's character record and never reaches this connection. So a
 * client has no way to work the band out, and one that opened a chooser before
 * SV_CMD_OUTFIT arrived would be showing a range it invented. The context-menu
 * entry therefore sends CL_CMD_GET_OUTFIT and opens nothing; this window
 * appears when, and only when, the server answers.
 *
 * It also appears unprompted on a character's very first login, because
 * `crplayer.cc` line 221 offers the chooser then. That is the same event and is
 * not treated as a special case.
 *
 * FOUR RULES THIS WINDOW EXISTS TO KEEP:
 *
 *   * the band is the server's. Only `First..Last` from the SV_CMD_OUTFIT that
 *     opened this window may be offered, every control is clamped to it, and a
 *     band naming nothing disables Submit rather than falling back to a guess;
 *   * the four colour slots are named from the classic client, not from
 *     Fusion32, which never names them. The proof is in
 *     `Real33DOutfitSelection.h` and `tests/verify_outfit_color_mapping.py`;
 *   * nothing acknowledges a submission. `CSetOutfit` returns void: no
 *     confirmation, no second SV_CMD_OUTFIT, and no error when it drops the
 *     selection. So this window must never report that an outfit was applied,
 *     and it must not wait for anything;
 *   * it closes itself on Submit. The one effect a submission has is
 *     `AnnounceChangedCreature(CREATURE_OUTFIT_CHANGED)`, and that is
 *     conditional on the player already wearing their own outfit -- a player
 *     who is invisible or under an illusion will not see a 142 at all. A
 *     window that stayed open until one arrived could hang for ever. Closing
 *     on OK is also what the classic client does, at `0x0047138B`,
 *     immediately after its own send.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 */
class SReal33DOutfitPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DOutfitPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Takes this frame's chooser from the bridge. Called once a frame by
	 * `SReal33DHUD::Refresh`, never from this widget's own Tick: Slate does not
	 * tick a collapsed widget, and this one is collapsed until Fusion32 offers
	 * a chooser, so a self-driven version could miss the very event that is
	 * supposed to open it. That exact mistake cost a live session in
	 * `UNREAL-EDITORS-UI-001`.
	 */
	void Refresh();

	/** True while a chooser from Fusion32 is on screen. */
	bool IsShowing() const { return bShowing; }

	/** What the player currently has selected. For tests and evidence. */
	const Real33D::OutfitDraft& GetDraft() const { return Draft; }

	// --------------------------------------------------------- display rules
	//
	// Static and free of Slate, so each is testable without a renderer.

	/** The window's heading, in the classic client's own words. */
	static FText Title();

	/**
	 * The band, stated as the server granted it.
	 *
	 * Names the actual ids rather than only a count, because the ids are what
	 * the evidence for a live run has to match against SV_CMD_OUTFIT. An
	 * inverted or empty band is reported as granting nothing rather than
	 * silently reordered.
	 */
	static FText RangeText(const FReal33DOutfitDialog& Dialog);

	/** The selected outfit and its position within the band. */
	static FText OutfitText(const Real33D::OutfitDraft& Draft);

	/** One colour row: the proven slot name and the byte that will be sent. */
	static FText SlotText(const Real33D::OutfitDraft& Draft, Real33D::OutfitSlot Slot);

	/**
	 * What this window is entitled to claim, which is never "applied".
	 *
	 * Reports the state of the chooser and what a submission will and will not
	 * produce. There is no success state to report, by design.
	 */
	static FText StatusText(const FReal33DOutfitDialog& Dialog,
		const Real33D::OutfitDraft& Draft);

	/**
	 * The note shown when the outfit the server reported is an object disguise.
	 *
	 * Empty otherwise. CL_CMD_SET_OUTFIT is seven fixed bytes -- a word and
	 * four colours, with no `OutfitID == 0` branch in `CSetOutfit` -- so an
	 * object disguise has nowhere to go on the wire and cannot be submitted
	 * back. The player is told that rather than shown a disguise silently
	 * rewritten into an outfit id.
	 */
	static FText DisguiseNote(const FReal33DOutfitDialog& Dialog);

	/** Whether Submit does anything. The band and the colour bound, as above. */
	static bool CanSubmit(const FReal33DOutfitDialog& Dialog,
		const Real33D::OutfitDraft& Draft);

private:
	/** Adopts the server's state into the draft and redraws every control. */
	void Rebuild();
	/** Redraws the labels and the enabled states from the current draft. */
	void RefreshControls();

	TSharedRef<SWidget> BuildColorRow(Real33D::OutfitSlot Slot);

	FReply HandleStepOutfit(int32 Delta);
	FReply HandleStepColor(Real33D::OutfitSlot Slot, int32 Delta);
	FReply HandleSubmit();
	FReply HandleClose();

	TWeakObjectPtr<UReal33DBridge> Bridge;

	/**
	 * What the player has chosen so far, inside the band the server sent.
	 *
	 * Presentation state, not a model of the character. The authoritative
	 * appearance arrives as SV_CMD_CREATURE_OUTFIT and lives in WorldState;
	 * nothing here is ever read back as truth about the player.
	 */
	Real33D::OutfitDraft Draft;

	TSharedPtr<STextBlock> Heading;
	TSharedPtr<STextBlock> RangeLabel;
	TSharedPtr<STextBlock> OutfitLabel;
	TSharedPtr<STextBlock> DisguiseLabel;
	TSharedPtr<STextBlock> Status;
	TSharedPtr<SButton> SubmitButton;
	TSharedPtr<SButton> PreviousOutfitButton;
	TSharedPtr<SButton> NextOutfitButton;

	/**
	 * One colour row: a plain label and nothing else held.
	 *
	 * The arrows are not kept, because nothing needs to reach them again: they
	 * are enabled whenever the chooser is open, which is whenever this widget
	 * is visible at all.
	 */
	struct FColorRow
	{
		TSharedPtr<STextBlock> Label;
	};
	FColorRow Rows[Real33D::kOutfitSlotCount];

	uint64 DrawnRevision = 0;
	bool bEverDrawn = false;
	bool bShowing = false;
};
