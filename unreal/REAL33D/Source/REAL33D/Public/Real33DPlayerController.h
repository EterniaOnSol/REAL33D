#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Real33DPlayerController.generated.h"

/**
 * Turns key presses into intents and nothing else.
 *
 * There is deliberately no path from a key to an Actor's position here. A press
 * becomes a walk request on the bridge, the bridge hands it to Protocol772Core,
 * Protocol772Core asks Fusion32, and only when Fusion32 answers does WorldState
 * change and the creature actor follow. If Fusion32 refuses the step nothing at
 * all happens on screen, which is the correct outcome: the player did not move.
 *
 * Bindings are made in code with BindKey so the project carries no binary input
 * assets. Direction values are the server's own, from enums.hh.
 */
UCLASS()
class REAL33D_API AReal33DPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AReal33DPlayerController();

	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void WalkNorth();
	void WalkEast();
	void WalkSouth();
	void WalkWest();
	void DumpEvidence();

	void Request(uint8 Direction);

	// ------------------------------------------------------------ chat input
	//
	// Enter opens the line, Enter again sends it, Escape abandons it. While the
	// line is open the walk keys are inert: a player typing "was" must not walk
	// west, north and south. That is checked at the top of Request rather than
	// by unbinding, so there is one place where the rule lives and no window in
	// which the bindings are half-swapped.

	// The talk mode persists until changed, which is how the operator describes
	// the original: a speaker control in the chat panel sets a mode and it stays
	// set. F2 cycles it. Everything typed afterwards goes out that way, rather
	// than the mode being re-chosen for every line.
	//
	// The mode is carried to the bridge as the classic "#y " / "#w " prefix, so
	// the key and the typed convention share one path. A typed prefix alone was
	// not enough: "#" is unbound here and needs a modifier on most layouts, so
	// yell and whisper were unreachable entirely.
	void ToggleChat();
	void CycleTalkMode();
	void OpenOrSend();
	const TCHAR* TalkModePrefix() const;
	const TCHAR* TalkModeLabel() const;
	void CancelChat();
	void Backspace();
	void TypeCharacter(TCHAR Glyph);
	void BindTypingKey(const FKey& Key, TCHAR Glyph);

	bool bComposing = false;
	FString Composing;

	/** 0 say, 1 whisper, 2 yell. Survives sending, like the classic control. */
	uint8 TalkMode = 0;
};
