#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Real33DBridge.h"
#include "Real33DCoords.h"
#include "Real33DWorldActor.generated.h"

class AReal33DCreature;
class AReal33DTile;
class UReal33DAssetRegistry;

/**
 * The game-thread consumer. The only class that spawns or destroys anything.
 *
 * Every frame it drains the semantic events the bridge published and makes the
 * scene match them. It holds no map of its own beyond the actors it has
 * spawned, asks no questions of the network, and decides nothing: if an actor
 * exists here, WorldState said the thing exists; if it moved, WorldState said
 * it moved.
 */
UCLASS()
class REAL33D_API AReal33DWorld : public AActor
{
	GENERATED_BODY()

public:
	AReal33DWorld();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** The creature this client controls, once Fusion32 has identified it. */
	AReal33DCreature* GetLocalPlayer() const;

	/** Writes the machine-readable evidence file for the acceptance run. */
	void WriteEvidence(const FString& Reason);

private:
	void HandleEvent(const FReal33DEvent& Event);
	void ClearWorld();
	void UpdateCamera(float DeltaSeconds);
	void DrawOverlay();

	UPROPERTY()
	TObjectPtr<UReal33DAssetRegistry> Registry = nullptr;

	UPROPERTY()
	TMap<FIntVector, TObjectPtr<AReal33DTile>> Tiles;

	UPROPERTY()
	TMap<uint32, TObjectPtr<AReal33DCreature>> Creatures;

	UPROPERTY()
	TObjectPtr<class UCameraComponent> Camera = nullptr;

	UPROPERTY()
	TObjectPtr<USceneComponent> CameraRoot = nullptr;

	Real33D::FWorldOrigin Origin;

	uint32 LocalCreatureId = 0;
	bool bConnected = false;

	/** Last thing the client core said it could not consume, verbatim. */
	FString LastDiagnostic;

	/** Kept so a dropped session can be retried without re-reading the command line. */
	FReal33DConnectionConfig Config;
	double DisconnectedAt = 0.0;
	int32 ReconnectAttempts = 0;

	/** Counters over the whole session, for the evidence file. */
	int32 TilesSpawned = 0;
	int32 TilesRemoved = 0;
	int32 CreaturesAppeared = 0;
	int32 CreaturesVanished = 0;
	int32 CreatureMoves = 0;
	int32 DuplicateSpawnAttempts = 0;
	int32 OrphanEvents = 0;
	double FirstFrameTime = 0.0;
};
