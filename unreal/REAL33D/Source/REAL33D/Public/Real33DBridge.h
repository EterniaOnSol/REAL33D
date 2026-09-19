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
	/**
	 * Something the client core could not fully consume. Carries prose only.
	 *
	 * For the developer. This must never reach a player-facing surface: a
	 * player reading "SV_CMD_CONTAINER_OPEN is not decoded" learns nothing and
	 * loses the message that mattered in the noise.
	 */
	Diagnostic,

	/**
	 * Something Fusion32 told this player directly, via SV_CMD_MESSAGE.
	 *
	 * A player message, not a protocol diagnostic. CTalk answers an illegal
	 * yell this way, so without it the client looks broken when the server is
	 * simply saying no. Carries the text in `Detail` and the server's own mode
	 * name in `TalkMode`, resolved inside the bridge so nothing above sees a
	 * number.
	 */
	ServerMessage,

	/**
	 * This client refused to send what the player typed, and is saying so.
	 *
	 * Neither a server message nor a diagnostic. The server never saw it, so
	 * attributing it to Fusion32 would be false; and the player caused it and
	 * can fix it, so hiding it on the developer surface would leave their
	 * message silently vanishing. Carries the reason in `Detail`.
	 */
	ClientNotice,

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
	 * `CreatureId` is the resolved speaker when the bridge could prove one, and
	 * zero otherwise; see `SpeakerResolution`.
	 */
	Talk,

	/**
	 * A creature's health changed, so its name colour must follow.
	 *
	 * Carries `CreatureId` and `HealthPercent`.
	 */
	CreatureHealth
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
 * How the player chose to speak. Semantic, never a protocol value.
 *
 * The UI picks one of these and the bridge is the only thing that knows what
 * byte each becomes, exactly as it is the only thing that knows what a walk
 * direction becomes. An earlier version carried the mode as a typed "#y "
 * prefix that the bridge parsed back out, which put a wire convention in the
 * one place that must not have one.
 *
 * Only the three positional modes are offered. The addressed and channel modes
 * `CTalk` also accepts need an addressee or a channel this client has no UI
 * for, and offering a mode that cannot carry its required field would produce
 * a command the server refuses.
 */
UENUM()
enum class EReal33DTalkMode : uint8
{
	Say,
	Whisper,
	Yell
};

/** The mode's player-facing name. Presentation only; no protocol meaning. */
REAL33D_API const TCHAR* Real33DTalkModeLabel(EReal33DTalkMode Mode);

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

	/** 0..100 as the server reports it. Meaningful for creature events. */
	uint8 HealthPercent = 100;
};

/** One line of the player-facing transcript, already formatted by ClientCore. */
struct FReal33DChatLine
{
	FString Line;
	/**
	 * True when this line is the server or the client addressing the player,
	 * rather than a creature speaking. Carried so the chat area can colour the
	 * two apart without parsing the formatted text back open.
	 */
	bool bSystemLine = false;
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
	/** Health changes applied to a creature's name colour. */
	int32 HealthUpdates = 0;
	/** Say commands this client put on the wire. */
	int32 SaysRequested = 0;
	/** Say commands ClientCore refused before sending, with the reason logged. */
	int32 SaysRefusedLocally = 0;
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
	/**
	 * Declared, not defaulted inline, because `Transcript` points at a type this
	 * header only forward-declares. Defined in the .cpp, where it is complete.
	 */
	virtual ~UReal33DBridge() override;

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

	/**
	 * Asks Fusion32 to speak, in the mode the player chose.
	 *
	 * An intent, exactly like a walk. Pressing Send is not proof the server
	 * accepted or broadcast anything: Fusion32 decides who hears it, and the
	 * authoritative talk it sends back is what this client draws. Nothing is
	 * displayed optimistically.
	 *
	 * The mode is semantic here and becomes a protocol value only inside the
	 * worker, which is the one place permitted to know one.
	 *
	 * Returns the id identifying this line for the rest of its life, so the
	 * request can be correlated with what comes back.
	 */
	uint32 RequestTalk(EReal33DTalkMode Mode, const FString& Text);

	/**
	 * The transcript of what the player should be able to read, oldest first.
	 *
	 * Fed by DrainEvents from speech and server messages, never from protocol
	 * diagnostics. Bounded and ordered by ClientCore's `ChatLog`, which owns
	 * those rules because they are testable without a renderer.
	 *
	 * Game thread only, like DrainEvents: one owner on one thread.
	 */
	void GetChatTranscript(TArray<FReal33DChatLine>& OutLines) const;

	/**
	 * Changes whenever the transcript does, so a widget can rebuild only then
	 * rather than every frame. Never reused, including across a reconnect.
	 */
	uint64 GetChatRevision() const;

	FReal33DStats GetStats() const;

	/** Reads the connection config from the command line, or a sensible default. */
	static FReal33DConnectionConfig ConfigFromCommandLine();

private:
	/** Files one drained event into the transcript, if it belongs there. */
	void NoteChat(const FReal33DEvent& Event);

	FReal33DWorker* Worker = nullptr;
	FRunnableThread* Thread = nullptr;
	/** Game thread only. Numbers key presses so evidence can follow one. */
	uint32 NextInputId = 0;

	/**
	 * ClientCore's ChatLog, held behind a pointer so this header stays free of
	 * anything under fusion32/. Created in Initialize, destroyed in
	 * Deinitialize, game thread only.
	 *
	 * A raw pointer rather than a TUniquePtr on purpose: a smart pointer would
	 * instantiate its deleter wherever this header is included, including UHT's
	 * generated constructor, and deleting an incomplete type there compiles to a
	 * delete that skips the destructor. With a raw pointer every line that knows
	 * what this is lives in the .cpp.
	 */
	struct FReal33DChatTranscript* Transcript = nullptr;
};
