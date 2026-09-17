#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Real33DGameMode.generated.h"

/**
 * Builds the scene from code so the project carries no binary map.
 *
 * The slice runs on an empty engine map: this spawns the world presenter and
 * the two lights it needs, which keeps everything that defines the run
 * reviewable as text and reproducible from a clean checkout.
 */
UCLASS()
class REAL33D_API AReal33DGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AReal33DGameMode();

	virtual void StartPlay() override;
};
