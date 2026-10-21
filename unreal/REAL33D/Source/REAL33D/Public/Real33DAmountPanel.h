#pragma once

#include "CoreMinimal.h"
#include "Real33DStackMove.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;
class SButton;

/** Fired when the player settles on an amount. Carries 1..stack count. */
DECLARE_DELEGATE_OneParam(FReal33DOnAmountChosen, uint32);

/** Fired when the player abandons the question. NOTHING is sent for it. */
DECLARE_DELEGATE(FReal33DOnAmountAbandoned);

/**
 * "How many?", asked once, for one move the player has already aimed.
 *
 * THE WINDOW TITLE IS THE CLASSIC CLIENT'S OWN. `Move Objects` is at
 * `0x00563C00` in the 7.72 image `docs/CLASSIC_CLIENT_772.md` records -- the
 * same window-title table, four strings earlier, that gave `Select Outfit` its
 * `0x00563C78` for the outfit milestone, and the same image hash. So that this
 * client asks the question at all, and what it calls the question, are not
 * inventions. The body wording and the stepper layout are this client's.
 *
 * Registered in `FReal33DPanelRegistry` under `Real33D::PanelIds::MoveAmount`,
 * so visibility keeps one owner, like the text editor, the trade window and the
 * outfit chooser. A centred modal like those three.
 *
 * TWO RULES THIS WINDOW EXISTS TO KEEP:
 *
 *   * it is the only thing between a Ctrl+drag and the wire, and until it is
 *     answered NOTHING has been sent. Cancelling sends nothing at all: 7.72 has
 *     no command that withdraws a move, because no move was ever offered, so
 *     Fusion32 is told nothing and keeps no record of the question;
 *   * every control is clamped to 1..the stack count the server reported, by
 *     `Real33DStackMove.h`, which is the same function the plain and Shift
 *     drags go through. An amount this window cannot produce is one CMoveObject
 *     would have refused.
 *
 * It does not mutate inventory, container or world state, and it does not wait
 * for anything: the authoritative counts arrive as the ordinary inventory and
 * container commands after the move, exactly as for an unmodified drag.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 */
class SReal33DAmountPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DAmountPanel) {}
		SLATE_EVENT(FReal33DOnAmountChosen, OnChosen)
		SLATE_EVENT(FReal33DOnAmountAbandoned, OnAbandoned)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Opens the question for a stack of `Max`, described by `Subject`.
	 *
	 * Opens ON `Max`, so a player who answers immediately gets what an
	 * unmodified drag would have given them. `Max` is clamped exactly as every
	 * other amount here is, so a zero or an over-byte count cannot open a
	 * window that could answer one.
	 *
	 * Asking again while a question is open replaces it. The caller is expected
	 * to have abandoned its own earlier pending move first; this window only
	 * ever describes one.
	 */
	void Ask(uint32 Max, const FText& Subject);

	/**
	 * Closes the question and reports that it was abandoned.
	 *
	 * Safe when nothing is open, in which case it does nothing and reports
	 * nothing. Bound to the same Escape the other modals use.
	 */
	void Abandon();

	/** True while a question is on screen. */
	bool IsShowing() const { return bShowing; }

	/** The amount currently showing. For tests and evidence. */
	uint32 GetAmount() const { return Amount; }

	/** The stack count this question is bounded by. */
	uint32 GetMax() const { return Max; }

	// --------------------------------------------------------- display rules
	//
	// Static and free of Slate, so each is testable without a renderer.

	/** The classic client's own title for this window. */
	static FText Title();

	/** The answer and the bound it sits in. */
	static FText AmountText(uint32 InAmount, uint32 InMax);

	/** What this window is entitled to claim, which is never "moved". */
	static FText NoteText();

private:
	FReply HandleStep(int32 Delta);
	FReply HandleAll();
	FReply HandleOne();
	FReply HandleOk();
	FReply HandleCancel();

	/** Redraws the labels and the enabled states from the current answer. */
	void RefreshControls();

	/** One stepper, with its own tooltip naming what it does. */
	TSharedRef<SWidget> MakeStep(const FText& Caption, const FText& Tip, int32 Delta);

	FReal33DOnAmountChosen OnChosen;
	FReal33DOnAmountAbandoned OnAbandoned;

	TSharedPtr<STextBlock> SubjectLabel;
	TSharedPtr<STextBlock> AmountLabel;
	TSharedPtr<SButton> OkButton;

	/**
	 * Presentation state, not a model of the stack. The authoritative count
	 * lives in WorldState and arrives from Fusion32; this is only the answer
	 * the player is composing.
	 */
	uint32 Amount = Real33D::kMinStackCount;
	uint32 Max = Real33D::kMinStackCount;
	bool bShowing = false;
};
