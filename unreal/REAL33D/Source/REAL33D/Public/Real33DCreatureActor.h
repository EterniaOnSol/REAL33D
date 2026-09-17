#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Real33DAssetRegistry.h"
#include "Real33DCoords.h"
#include "Real33DCreatureActor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * One creature, which may be this client's own player.
 *
 * The actor carries a logical position and a drawn position, and they are not
 * the same thing. The logical position only ever changes when WorldState says
 * so: it is a copy of the server's truth, never a guess. The drawn position
 * eases towards the logical one so a step reads as a step instead of a
 * teleport, and that easing is the only liberty taken here.
 *
 * Nothing in this class may move the logical position on its own. Input does
 * not reach it. See Real33DPlayerController for why.
 */
UCLASS()
class REAL33D_API AReal33DCreature : public AActor
{
	GENERATED_BODY()

public:
	AReal33DCreature();

	virtual void Tick(float DeltaSeconds) override;

	void Configure(uint32 InCreatureId, bool bInIsLocalPlayer, const FString& InName,
		const UReal33DAssetRegistry* Registry);

	/**
	 * Commits a server-confirmed position.
	 * @param bSnap places the drawn position there immediately, for an
	 *              appearance or a viewport jump where interpolation would show
	 *              a creature sliding across the map.
	 */
	void CommitPosition(const Real33D::FWorldOrigin& Origin,
		const Real33D::FMapPosition& Position, bool bSnap);

	void SetFacing(uint8 Direction);

	/**
	 * Shows a line of speech above this creature for a while.
	 *
	 * The text is owned by the creature's own component, so a creature leaving
	 * the viewport takes its speech with it: there is no separate registry that
	 * could outlive the actor and no pointer anyone else has to clear.
	 *
	 * Game thread only.
	 */
	void ShowSpeech(const FString& Text, float Seconds);

	uint32 GetCreatureId() const { return CreatureId; }
	bool IsLocalPlayer() const { return bIsLocalPlayer; }
	const Real33D::FMapPosition& GetLogicalPosition() const { return LogicalPosition; }

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body = nullptr;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> NameTag = nullptr;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> SpeechTag = nullptr;

	/** When the current speech stops being shown. Zero means nothing is shown. */
	double SpeechExpiresAt = 0.0;

	Real33D::FMapPosition LogicalPosition;

	/** Where the body is drawn while it catches up with the logical position. */
	FVector DrawnLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;

	uint32 CreatureId = 0;
	bool bIsLocalPlayer = false;
	bool bPlaced = false;
};
