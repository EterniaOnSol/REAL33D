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

	// One key per talk mode. The bridge also parses the classic "#y " and "#w "
	// prefixes, but "#" is unbound and needs a modifier on most layouts, so a
	// typed prefix left yell and whisper unreachable: the operator could not
	// enter the character at all. The key seeds the prefix instead.
	void ToggleChat();
	void ToggleYell();
	void ToggleWhisper();
	void OpenOrSend(const TCHAR* Prefix, const TCHAR* Label);
	void CancelChat();
	void Backspace();
	void TypeCharacter(TCHAR Glyph);
	void BindTypingKey(const FKey& Key, TCHAR Glyph);

	bool bComposing = false;
	FString Composing;
};
