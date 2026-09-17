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
	Diagnostic,

	// The outgoing walk path, reported back so the whole chain from a key press
	// to an authoritative position can be correlated in evidence rather than
	// inferred. A live session moved the local player eight fields while the
	// client had sent nothing, so "the player moved" proves nothing on its own.

	/** A walk command reached the wire. Carries InputId and RequestId. */
	WalkSent,
	/** Fusion32 answered a walk by moving us to the field it asked for. */
	WalkAccepted,
	/** Fusion32 refused a walk. Position is unchanged by definition. */
	WalkRejected,
	/** Fusion32 moved us for its own reasons: a push, or anything not ours. */
	ExternalRelocation,
	/** A walk was neither accepted nor refused before it expired. */
	WalkUnanswered,

	/**
	 * Someone spoke. Carries the decoded semantics, never protocol bytes.
	 *
	 * This milestone decodes talk so an ordinary session stops producing an
	 * unsupported opcode. Presentation of chat is out of scope: the event is
	 * logged and counted, and nothing draws it.
	 */
	Talk
};

/** Which shape of talk this was, mirroring the three forms Fusion32 emits. */
UENUM()
enum class EReal33DTalkLayout : uint8
{
	Positional,
	Channel,
	Plain
};

/**
 * Stands for "this event has no direction".
 *
 * Zero cannot mean that: enums.hh gives north the value 0, so a defaulted field
 * reads as a real heading. A push has no direction anyone requested, and saying
 * "north" about it is the same class of mistake as calling it an accepted step.
 */
inline constexpr uint8 kNoDirection = 0xFF;

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
	/** enums.hh direction, or kNoDirection when the event has none. */
	uint8 Direction = 0;
	bool bIsLocalPlayer = false;
	TArray<FReal33DThing> Things;
	FString Detail;
	/** Identifies the key press this event belongs to, 0 when it belongs to none. */
	uint32 InputId = 0;
	/** Identifies the walk command on the wire, assigned by the ledger. */
	uint32 RequestId = 0;

	// Talk. Populated only for EReal33DTalkKind::Talk; `Position` carries the
	// speech position when the form has one, and `Detail` carries the text.
	FString Speaker;
	EReal33DTalkLayout TalkLayout = EReal33DTalkLayout::Plain;
	/** The mode's name, resolved inside the bridge so Unreal never sees a number. */
	FString TalkMode;
	bool bHasChannel = false;
	int32 Channel = 0;
	/**
	 * How the speaker was identified, for evidence: `Resolved`, `NoMatch`,
	 * `Ambiguous` or `NotPositional`. Anything but `Resolved` means
	 * `CreatureId` is zero and the message belongs on the fallback surface,
	 * because showing it above a guessed creature would be worse than not
	 * showing it in the world at all.
	 */
	FString SpeakerResolution;
};

/** Counters the game thread may read for the on-screen diagnostic overlay. */
struct FReal33DStats
{
	int32 Frames = 0;
	int32 Commands = 0;
	int32 ResidualBytes = 0;
	int32 UnsupportedOpcodes = 0;
	int32 Anomalies = 0;
	/** Walk commands this client actually put on the wire. */
	int32 RequestedSteps = 0;
	/** Requests answered by a move to exactly the field the request asked for. */
	int32 AcceptedSelfWalks = 0;
	/** Requests Fusion32 refused, counted from the snapback it sends back. */
	int32 RejectedSteps = 0;
	/**
	 * Position changes that were not ours: a push from another creature, or
	 * anything else Fusion32 decided. Counted apart from accepted walks on
	 * purpose. A live run recorded eight of these against zero requests, and
	 * the counter then in use reported them as accepted steps, which read as
	 * proof of an input path that had never been exercised.
	 */
	int32 ExternalRelocations = 0;
	/** Requests that were neither accepted nor refused before expiry. */
	int32 UnansweredSteps = 0;
	/** Every local-player move, whatever its cause. The sum of the two above. */
	int32 LocalPlayerMoves = 0;
	/** Talk commands decoded. Counted to show chat no longer stops the walk. */
	int32 TalkMessages = 0;
	/** Talk shown above the creature that said it. */
	int32 TalkShownOnCreature = 0;
	/** Talk shown on the fallback surface because no speaker could be proven. */
	int32 TalkShownOnFallback = 0;
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
	 *
	 * Returns the input id that identifies this key press for the rest of its
	 * life, so evidence can join "a key was pressed" to "the server moved us"
	 * instead of assuming the second followed from the first.
	 */
	uint32 RequestWalk(uint8 Direction);

	FReal33DStats GetStats() const;

	/** Reads the connection config from the command line, or a sensible default. */
	static FReal33DConnectionConfig ConfigFromCommandLine();

private:
	FReal33DWorker* Worker = nullptr;
	FRunnableThread* Thread = nullptr;
	/** Game thread only. Numbers key presses so evidence can follow one. */
	uint32 NextInputId = 0;
};
