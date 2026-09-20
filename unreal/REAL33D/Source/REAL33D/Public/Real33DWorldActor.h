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
	const UReal33DAssetRegistry* GetAssetRegistry() const { return Registry; }

	/** Writes the machine-readable evidence file for the acceptance run. */
	void WriteEvidence(const FString& Reason);

	// ---------------------------------------------------------------- camera
	//
	// The camera orbits the player: the actor is already moved onto the local
	// creature every frame, so yaw, pitch and distance are the whole state, and
	// the player stays centred whatever the operator does with them.
	//
	// This exists because speech drawn above a creature was unreadable from the
	// one fixed angle the camera used to have. The text component was never the
	// problem; not being able to look at it from anywhere else was.

	/** Swings the view around the player. Degrees. */
	void AddCameraOrbit(float DeltaYawDegrees, float DeltaPitchDegrees);

	/** Pulls the view back or pushes it in. Positive pulls back. */
	void AddCameraDistance(float Delta);

	/** Maps W/D/S/A to the nearest server cardinal direction at the current camera yaw. */
	uint8 CameraRelativeDirection(uint8 RelativeDirection) const;

private:
	/** Rebuilds the camera's relative transform from yaw, pitch and distance. */
	void ApplyCameraTransform();

	static constexpr float kCameraPitchMin = -85.0f;
	static constexpr float kCameraPitchMax = -5.0f;
	static constexpr float kCameraDistanceMin = 400.0f;
	static constexpr float kCameraDistanceMax = 3000.0f;

	float CameraYaw = 45.0f;
	float CameraPitch = -42.0f;
	float CameraDistance = 1173.5f;

	void HandleEvent(const FReal33DEvent& Event);

	/**
	 * Decides where a decoded talk becomes visible.
	 *
	 * Above the creature that said it when the bridge resolved a speaker;
	 * otherwise on a small on-screen fallback list, which still has to be
	 * readable without opening a log.
	 */
	void PresentSpeech(const FReal33DEvent& Event);

	/**
	 * Appends one line to the movement journal.
	 *
	 * The journal exists so the outgoing path can be read as a chain rather
	 * than assumed: a key press, the command that reached the wire, and the
	 * authoritative answer, joined by an input id. Position changes that belong
	 * to no request are written too, and marked as such.
	 */
	void JournalMovement(const FReal33DEvent& Event);
	void ClearWorld();
	void UpdateCamera(float DeltaSeconds);
	void UpdateFloorVisibility();
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
	bool bFloorVisibilityDirty = true;
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

	/** Where the movement journal is appended, decided once at BeginPlay. */
	FString JournalPath;

	/**
	 * How long a message stays on screen.
	 *
	 * NOT a proven 7.72 value. Fusion32 sends no duration and has no command to
	 * retract speech, so lifetime is not server or protocol owned; the classic
	 * client owns it, and this project has the client only as a binary, so the
	 * exact rule is unproven. See docs/UNREAL_CHAT.md. Overridable with
	 * -real33d-speech-seconds= so a measurement can replace it without a
	 * rebuild.
	 */
	float SpeechSeconds = 6.0f;

	int32 SpeechOnCreature = 0;
	/** Speech with no provable speaker, readable only in the chat area. */
	int32 SpeechInChatArea = 0;
	/** Messages Fusion32 addressed to this player, counted apart from speech. */
	int32 ServerMessagesReceived = 0;
	/** Times this client refused to send a line, counted apart from the above. */
	int32 ClientNoticesRaised = 0;
};
