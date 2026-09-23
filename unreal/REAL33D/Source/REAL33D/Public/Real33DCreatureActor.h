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

	/**
	 * Colours the name by how hurt the creature is.
	 *
	 * The server sends a health percentage in the creature descriptor and in
	 * SV_CMD_CREATURE_HEALTH; what colour that becomes is client presentation.
	 * Fusion32 defines no colours and no thresholds, so the bands here are the
	 * operator's specification, not 7.72 parity. See docs/UNREAL_CHAT.md.
	 *
	 * Game thread only.
	 */
	void SetHealthPercent(uint8 Percent);

	/** Draws the WorldState-owned attack/follow marker on this actor. */
	void SetCombatFeedback(bool bAttacked, bool bFollowed);

	uint32 GetCreatureId() const { return CreatureId; }
	bool IsLocalPlayer() const { return bIsLocalPlayer; }
	const Real33D::FMapPosition& GetLogicalPosition() const { return LogicalPosition; }

	/** The name Fusion32 introduced this creature with; empty if it sent none. */
	const FString& GetCreatureName() const { return CreatureName; }

	/** 0..100 as the server last reported it. The battle list draws this. */
	uint8 GetHealthPercent() const { return HealthPercent; }

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body = nullptr;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> NameTag = nullptr;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> SpeechTag = nullptr;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> TargetTag = nullptr;

	/** When the current speech stops being shown. Zero means nothing is shown. */
	double SpeechExpiresAt = 0.0;

	Real33D::FMapPosition LogicalPosition;

	/** Kept so the battle list can name a row without re-reading the name tag. */
	FString CreatureName;

	/** Where the body is drawn while it catches up with the logical position. */
	FVector DrawnLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector BodyBaseOffset = FVector::ZeroVector;
	double WalkPhase = 0.0;

	uint32 CreatureId = 0;
	bool bIsLocalPlayer = false;
	bool bPlaced = false;
	uint8 HealthPercent = 100;
};
