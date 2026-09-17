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

private:
	void WalkNorth();
	void WalkEast();
	void WalkSouth();
	void WalkWest();
	void DumpEvidence();

	void Request(uint8 Direction);
};
