#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Real33DBridge.h"
#include "Real33DCoords.h"
#include "Real33DStaticSectorActor.h"
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
	const FReal33DPlayerVitals& GetPlayerVitals() const { return PlayerVitals; }
	const FReal33DPlayerSkills& GetPlayerSkills() const { return PlayerSkills; }
	const FReal33DConditions& GetConditions() const { return PlayerConditions; }
	const FReal33DInventory& GetInventory() const { return PlayerInventory; }
	const FReal33DCombat& GetCombat() const { return Combat; }

	/** The open containers, in the server's own container-number order. */
	const TArray<FReal33DContainer>& GetContainers() const { return OpenContainers; }

	/**
	 * The creatures currently on screen, nearest first.
	 *
	 * Read straight off the actors this class already owns, which are the game
	 * thread's only mirror of what WorldState said exists. The battle list is a
	 * view of that mirror and keeps no creature record of its own: a second
	 * list would be a second answer to "who is here", and the two would
	 * disagree the first time a creature left the viewport.
	 *
	 * Game thread only.
	 */
	void GetBattleList(TArray<FReal33DBattleEntry>& OutEntries) const;

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
	void RefreshCombatFeedback();
	void ClearWorld();
	void UpdateCamera(float DeltaSeconds);
	void UpdateFloorVisibility();
	void DrawOverlay();
	void InitialiseWideWorld();
	void UpdateWideWorld();
	void RequestStaticSector(const FIntVector& SectorKey);
	void InstallStaticSector(const FIntVector& SectorKey,
		const TArray<FReal33DStaticItem>& Items, const FString& Error);

	UPROPERTY()
	TObjectPtr<UReal33DAssetRegistry> Registry = nullptr;

	UPROPERTY()
	TMap<FIntVector, TObjectPtr<AReal33DTile>> Tiles;

	UPROPERTY()
	TMap<FIntVector, TObjectPtr<AReal33DStaticSector>> StaticSectors;

	TMap<FIntVector, TArray<FReal33DStaticItem>> StaticSectorCache;
	TSet<FIntVector> PendingStaticSectors;
	FString WideWorldCacheDirectory;
	FString WideWorldPreviewDirectory;
	Real33D::FMapPosition WideWorldAnchor;
	int32 WideWorldVisualRadius = 64;
	int32 WideWorldLoads = 0;
	int32 WideWorldCacheHits = 0;
	int32 WideWorldUnloads = 0;
	int32 WideWorldV08Resolved = 0;
	int32 WideWorldClassicFallback = 0;
	int32 WideWorldMissingPhysical = 0;
	bool bWideWorldEnabled = false;
	bool bWideWorldAnchorSet = false;

	UPROPERTY()
	TMap<uint32, TObjectPtr<AReal33DCreature>> Creatures;

	UPROPERTY()
	TObjectPtr<class UCameraComponent> Camera = nullptr;

	UPROPERTY()
	TObjectPtr<USceneComponent> CameraRoot = nullptr;

	Real33D::FWorldOrigin Origin;

	FReal33DPlayerVitals PlayerVitals;
	FReal33DPlayerSkills PlayerSkills;
	FReal33DConditions PlayerConditions;

	/** What the player is wearing, as the server last described it. */
	FReal33DInventory PlayerInventory;

	/** Presentation copy of ClientCore/WorldState's one combat record. */
	FReal33DCombat Combat;

	/**
	 * The containers the player has open, kept sorted by container number.
	 *
	 * A closed one is removed rather than left with `bOpen` false: the panel
	 * list is built from this, and a closed container has no panel.
	 */
	TArray<FReal33DContainer> OpenContainers;

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
