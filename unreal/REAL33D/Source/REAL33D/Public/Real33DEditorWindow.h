#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SMultiLineEditableTextBox;
class STextBlock;

/**
 * The book, sign and house-list window.
 *
 * One widget for both commands, because Fusion32 treats them as one window:
 * SV_CMD_EDIT_TEXT and SV_CMD_EDIT_LIST each replace whatever was open, and
 * neither has a close or a cancel on the wire.
 *
 * NOT A REGISTRY SIDE PANEL, AND NOT A FLOATING WINDOW EITHER. It is an
 * isolated modal centred over the map, which is what the 2D client puts on
 * screen for a book. It is still *registered* in `FReal33DPanelRegistry`
 * under `Real33D::PanelIds::TextEditor`, so showing and hiding it goes
 * through the same one mechanism every other panel uses and the HUD gains no
 * second way to open something. The registry only cares about visibility; it
 * does not care that this widget lives in a centre overlay rather than in a
 * 176px column.
 *
 * WHAT THIS WINDOW MUST NOT DO, and the reason the rule is written here:
 *
 *   * it must not decide whether the object can be written. SendEditText sets
 *     MaxLength to the type's limit minus one for a writable object and to
 *     strlen(Text) for everything else, so the two cases are not
 *     distinguishable from the wire, and CEditText re-checks the WRITE and
 *     WRITEONCE flags itself. The box is therefore always editable and
 *     Fusion32 answers;
 *   * it must not report that an edit was saved. operate.cc::EditText writes
 *     the text and the editor name and sends nothing back. Only a refusal
 *     answers, as an ordinary SV_CMD_MESSAGE through SendResult. So Submit
 *     says the text was handed over, never that it was kept, and the only
 *     proof of acceptance is reading the object again.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 */
class SReal33DEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DEditorWindow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Takes this frame's editor state from the bridge. Called once a frame by
	 * `SReal33DHUD::Refresh`, like every other panel.
	 *
	 * NOT A Tick OVERRIDE, and that is the whole point. Slate does not tick a
	 * widget whose visibility is Collapsed, and this window is collapsed
	 * whenever no editor is open -- so a self-ticking version could never
	 * notice that Fusion32 had opened one and could never un-collapse itself.
	 * A live run proved it: fifteen SV_CMD_EDIT_TEXT commands decoded and not
	 * one window drawn. Driving it from the HUD also puts it back under this
	 * HUD's own rule, that no panel reaches for state on its own.
	 */
	void Refresh();

	/** True while a window from Fusion32 is on screen. */
	bool IsShowing() const { return bShowing; }

	// --------------------------------------------------------- display rules
	//
	// Static and free of Slate, so each is testable without a renderer, a
	// bridge or a server.

	/**
	 * The window's heading.
	 *
	 * A list is named by its kind, which the server's own type byte decides.
	 * A text window carries no name on the wire at all -- SendEditText sends
	 * an object id and a type id and no string for either -- so it is headed
	 * generically rather than with an item name this client would have to
	 * invent. The ids go in the detail line, where they are useful evidence.
	 */
	static FText Title(const FReal33DEditor& Editor);

	/**
	 * The attribution line.
	 *
	 * Three different answers, never one: a named editor, an explicit
	 * "nobody has written on this" for the empty string the server really
	 * sent, and nothing at all for a list, which carries no editor field.
	 * Printing "written by" with an empty name, or hiding the empty case,
	 * would both make a fact the server stated unreadable.
	 */
	static FText Attribution(const FReal33DEditor& Editor);

	/**
	 * The line describing what the server will accept back.
	 *
	 * Deliberately phrased as the server's limit rather than as this window's
	 * capacity, and it never claims the object is writable: for a read-only
	 * object SendEditText reports `strlen(Text)` here, which is a real number
	 * that happens not to mean a limit at all.
	 */
	static FText LengthNote(const FReal33DEditor& Editor);

	/**
	 * Whether Submit does anything.
	 *
	 * False only when there is nothing to submit *against*: a closed window,
	 * a text window with no object id, or a list whose type Fusion32 never
	 * named. Never false because this client thinks the object is read-only
	 * -- that is the server's ruling and the whole point of the window.
	 */
	static bool CanSubmit(const FReal33DEditor& Editor);

private:
	void Rebuild();
	FReply HandleSubmit();
	FReply HandleClose();

	TWeakObjectPtr<UReal33DBridge> Bridge;

	TSharedPtr<SMultiLineEditableTextBox> Body;
	TSharedPtr<STextBlock> Heading;
	TSharedPtr<STextBlock> AttributionText;
	TSharedPtr<STextBlock> LengthText;
	TSharedPtr<STextBlock> DetailText;
	/** Says what happened to the last submission, in the server's terms. */
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<class SButton> SubmitButton;

	/** What is drawn, so the box is not rebuilt under the player's caret. */
	uint64 DrawnRevision = 0;
	bool bEverDrawn = false;
	bool bShowing = false;
};
