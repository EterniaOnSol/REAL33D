#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "Real33DCoords.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Real33DBridge.generated.h"

/**
 * The boundary between Protocol772Core and Unreal.
 *
 * A worker thread owns the socket and the authoritative WorldState and runs
 * ClientCore exactly as the command-line harnesses do. It publishes immutable
 * semantic events into a single-producer single-consumer queue. The game thread
 * drains that queue and is the only thread that ever touches an Actor.
 *
 * Unreal never sees a packet, an opcode or a byte. It never decides whether a
 * move is legal. Input becomes an intent posted back to the worker, and the
 * presentation only commits once WorldState confirms it.
 */

UENUM()
enum class EReal33DEventKind : uint8
{
	LocalPlayerIdentified,
	AnchorMoved,
	TileUpserted,
	TileRemoved,
	CreatureAppeared,
	CreatureMoved,
	CreatureVanished,
	Connected,
	Disconnected,
	Failed,
	/** Something the client core could not fully consume. Carries prose only. */
	Diagnostic
};

/** One thing on a field, flattened so nothing points back into WorldState. */
struct FReal33DThing
{
	bool bIsCreature = false;
	uint16 TypeId = 0;
	uint32 CreatureId = 0;
	/**
	 * The server says this object stops a creature entering the field.
	 *
	 * Read from Fusion32's own `dat/objects.srv` UNPASS flag, not inferred.
	 * Presentation-relevant only: drawing every object the same way tells the
	 * player nothing about where they can walk, and an operator reported
	 * walking "through cubes" that were never walls.
	 */
	bool bBlocking = false;
};

/** An event crossing the thread boundary. Copied, never shared. */
struct FReal33DEvent
{
	EReal33DEventKind Kind = EReal33DEventKind::AnchorMoved;
	Real33D::FMapPosition Position;
	Real33D::FMapPosition PreviousPosition;
	uint32 CreatureId = 0;
	FString CreatureName;
	uint8 Direction = 0;
	bool bIsLocalPlayer = false;
	TArray<FReal33DThing> Things;
	FString Detail;
};

/** Counters the game thread may read for the on-screen diagnostic overlay. */
struct FReal33DStats
{
	int32 Frames = 0;
	int32 Commands = 0;
	int32 ResidualBytes = 0;
	int32 UnsupportedOpcodes = 0;
	int32 Anomalies = 0;
	/** Walk intents handed to Fusion32. Says nothing about the outcome. */
	int32 RequestedSteps = 0;
	/**
	 * Times Fusion32 moved the local player, whatever the cause.
	 *
	 * Deliberately not called "accepted steps". A live run recorded eight of
	 * these against zero requests: the server moves a player for its own
	 * reasons too, and calling that an accepted request would have read as
	 * proof of something the client never asked for.
	 */
	int32 LocalPlayerMoves = 0;
	/** Intents Fusion32 refused, counted from the snapback it sends back. */
	int32 RejectedSteps = 0;
	int32 Tiles = 0;
	int32 VisibleCreatures = 0;
	bool bViewportSynchronised = false;
	Real33D::FMapPosition Anchor;
};

/** Where to connect and as whom. Read at runtime, never committed. */
struct FReal33DConnectionConfig
{
	FString Host = TEXT("127.0.0.1");
	int32 LoginPort = 7171;
	/** Path to the sanitized runtime directory holding secrets/ and the object data. */
	FString RuntimeDir;
	/** Which synthetic account of that runtime to use: "A" or "B". */
	FString Account = TEXT("B");
	FString ObjectsSrvPath;

	bool IsValid() const { return !RuntimeDir.IsEmpty(); }
};

class FReal33DWorker;

UCLASS()
class REAL33D_API UReal33DBridge : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Starts the worker. Safe to call once; returns false if already running. */
	bool Connect(const FReal33DConnectionConfig& Config);
	void Disconnect();
	bool IsRunning() const;

	/** Drains everything the worker has published. Game thread only. */
	void DrainEvents(TArray<FReal33DEvent>& OutEvents);

	/**
	 * Posts a walk intent. This does not move anything: it asks Fusion32, and
	 * the Actor only follows once WorldState confirms it.
	 * Direction uses enums.hh: 0 north, 1 east, 2 south, 3 west.
	 */
	void RequestWalk(uint8 Direction);

	FReal33DStats GetStats() const;

	/** Reads the connection config from the command line, or a sensible default. */
	static FReal33DConnectionConfig ConfigFromCommandLine();

private:
	FReal33DWorker* Worker = nullptr;
	FRunnableThread* Thread = nullptr;
};
