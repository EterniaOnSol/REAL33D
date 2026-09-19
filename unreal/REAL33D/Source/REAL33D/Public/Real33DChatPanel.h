#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;
class SVerticalBox;
class SReal33DChatPanel;

/** Fired when the player starts or stops typing. True means typing. */
DECLARE_DELEGATE_OneParam(FReal33DOnTypingChanged, bool);

/**
 * The text box, subclassed for one reason: to say when it takes focus.
 *
 * `SEditableTextBox` forwards keyboard focus to an inner widget, so watching
 * for focus loss from the outside is unreliable. Focus *arrival* is not: it is
 * `OnFocusReceived` on this widget, and it fires both when the player clicks
 * the box and when the controller focuses it deliberately. That single explicit
 * signal is what opens the input gate, so the gate cannot be left closed while
 * the player is already typing.
 */
class SReal33DChatInput : public SEditableTextBox
{
public:
	SLATE_BEGIN_ARGS(SReal33DChatInput) {}
		SLATE_ARGUMENT(SReal33DChatPanel*, OwningPanel)
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry,
		const FFocusEvent& InFocusEvent) override;

private:
	/** The panel outlives this widget: it owns it. Raw is correct here. */
	SReal33DChatPanel* Panel = nullptr;
};

/**
 * The player-facing chat area.
 *
 * What the player reads and what the developer reads are different surfaces.
 * This one carries speech and the server's own messages, and never a protocol
 * diagnostic; the counters overlay carries the diagnostics and never a message.
 * Mixing them is the defect `UNREAL-CHAT-AREA-001` exists to close: a distant
 * yell was being drawn with AddOnScreenDebugMessage underneath the frame and
 * tile counters, where the operator could not read it.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 *
 * Nothing is rendered optimistically. Sending posts an intent to the bridge and
 * stops; the line appears when Fusion32 broadcasts it back, like every other
 * thing this client believes about the world.
 */
class SReal33DChatPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DChatPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
		SLATE_EVENT(FReal33DOnTypingChanged, OnTypingChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime,
		const float DeltaTime) override;

	/**
	 * Opens the input gate and, unless the box already has focus, asks for it.
	 *
	 * Explicit rather than inferred from Slate's focus state, so "is the player
	 * typing" has exactly one answer and one owner. Idempotent.
	 */
	void BeginTyping();

	/** Closes the gate. Idempotent, and safe to call from any exit path. */
	void EndTyping();

	bool IsTyping() const { return bTyping; }

	/** The widget the controller must focus to put the caret in the box. */
	TSharedPtr<SWidget> GetInputWidget() const;

private:
	void HandleTextCommitted(const FText& Text, ETextCommit::Type Cause);
	FReply HandleSendClicked();
	void Send();
	void RebuildTranscript();

	ECheckBoxState IsModeChosen(EReal33DTalkMode Mode) const;
	void ChooseMode(ECheckBoxState State, EReal33DTalkMode Mode);
	TSharedRef<SWidget> BuildModeSelector();

	TWeakObjectPtr<UReal33DBridge> Bridge;
	FReal33DOnTypingChanged OnTypingChanged;

	TSharedPtr<SReal33DChatInput> Input;
	TSharedPtr<SScrollBox> History;
	TSharedPtr<SVerticalBox> Lines;

	/**
	 * The talk mode persists until the player changes it.
	 *
	 *     TALK_MODE_PERSISTENCE = REAL33D_UI_BEHAVIOUR
	 *
	 * Not 7.72 parity. Both a persistent and a per-message mode produce
	 * identical CL_CMD_TALK bytes, so the server cannot distinguish them and
	 * source cannot settle it. The operator reports the original worked this
	 * way; that is third-party recollection, recorded as such.
	 */
	EReal33DTalkMode Mode = EReal33DTalkMode::Say;

	/** The gate. One flag, one owner, read by the controller. */
	bool bTyping = false;

	/** Last transcript revision drawn, so the list rebuilds only on a change. */
	uint64 DrawnRevision = 0;
	bool bEverDrawn = false;
};
