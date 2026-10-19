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
	MinimapObserved,
	CreatureAppeared,
	CreatureMoved,
	CreatureVanished,
	CreatureAppearance,
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
	CreatureHealth,
	/** Server-owned health, mana and level for the local player's HUD. */
	PlayerVitals,

	/**
	 * The seven fighting skills, from SV_CMD_PLAYER_SKILLS.
	 *
	 * A separate command from SV_CMD_PLAYER_DATA and separately timed, so it
	 * gets its own event rather than being folded into the vitals: a client
	 * that had skills but no stats, or the reverse, must be able to say so.
	 */
	PlayerSkills,

	/**
	 * The condition flags, from SV_CMD_PLAYER_STATE.
	 *
	 * Eight bits, each a status icon. Carried verbatim; which icon a bit draws
	 * is presentation and is decided above this layer.
	 */
	PlayerConditions,

	/**
	 * A body slot changed, from SV_CMD_SET_INVENTORY or its delete.
	 *
	 * Carries the whole of what the player is wearing rather than the one slot
	 * that moved. WorldState already holds all ten, and sending the set means
	 * the HUD can never draw a mixture of two different moments.
	 */
	InventoryChanged,

	/**
	 * A container was opened, closed, or had an object added, changed or
	 * removed. Carries the full contents of that container for the same reason.
	 */
	ContainerChanged,

	/**
	 * The attack or follow target changed. Carries the whole combat state.
	 *
	 * Published when this client names a target and when Fusion32 takes one
	 * away with SV_CMD_CLEAR_TARGET, which are the only two moments the target
	 * can change. There is no third: the server acknowledges an accepted
	 * target with silence.
	 */
	CombatChanged,

	// ------------------------------------------------------------- channels
	//
	// One event per channel command Fusion32 sends. Four kinds for five
	// commands: SV_CMD_OPEN_CHANNEL and SV_CMD_OPEN_OWN_CHANNEL have the same
	// shape and differ only in whose channel it is, which `bOwnChannel`
	// carries.

	/**
	 * The channel list, from SV_CMD_CHANNELS, in answer to a request.
	 *
	 * `Channels` is the whole list as the server sent it. It is the only thing
	 * that may put a channel name on screen: a client that drew "Trade" or
	 * "Help" from a table of its own would be naming channels this player may
	 * not be allowed to see, since SendChannels filters by
	 * `ChannelAvailable(Channel, Player->ID)`.
	 */
	ChannelList,

	/**
	 * A channel this player is now in, from SV_CMD_OPEN_CHANNEL or
	 * SV_CMD_OPEN_OWN_CHANNEL. Carries `Channel` and `ChannelName`.
	 */
	ChannelOpened,

	/** A channel this player is no longer in, from SV_CMD_CLOSE_CHANNEL. */
	ChannelClosed,

	/**
	 * A private conversation with one named character, from
	 * SV_CMD_PRIVATE_CHANNEL. Carries `ChannelName` and no number, because
	 * SendPrivateChannel writes no number.
	 */
	PrivateChannelOpened,

	// -------------------------------------------------------------- editors
	//
	// Fusion32 opens both of these; this client never opens one by itself.
	// SV_CMD_EDIT_TEXT answers a use of a TEXT object -- `moveuse.cc`'s
	// `UseTextObject` and the spellbook branch of `UseAnnouncer` are its only
	// two callers -- and SV_CMD_EDIT_LIST answers one of the three house
	// spells in `magic.cc`.

	/**
	 * A book, sign, scroll, tombstone or spellbook to read, from
	 * SV_CMD_EDIT_TEXT. Carries the object id to submit against, the type id,
	 * the server's own length limit, the text and who last wrote it.
	 */
	EditTextOpened,

	/**
	 * A house guest list, subowner list or door list, from SV_CMD_EDIT_LIST.
	 * Carries which list it is, the id it is submitted against and the body.
	 */
	EditListOpened
};

/**
 * Which of the three lists an SV_CMD_EDIT_LIST carries.
 *
 * Mirrors `fusion32::protocol772::EditListType`, which mirrors `enums.hh`.
 * The two are pinned together in the bridge, the one file that sees both.
 */
UENUM()
enum class EReal33DEditListType : uint8
{
	Guest = 1,
	Subowner = 2,
	Door = 3,
	/** The byte was not one of the three. Reported rather than guessed. */
	Unknown = 0
};

/** The list's player-facing name. Presentation only; no protocol meaning. */
REAL33D_API const TCHAR* Real33DEditListTypeLabel(EReal33DEditListType Type);

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
 * The three positional modes, and only those. The addressed and channel modes
 * `CTalk` also accepts carry a required extra field, so they are not a *mode*
 * the player picks here at all: a channel line is addressed by the tab it is
 * typed into and a private line by the character it is typed at. Those travel
 * through `RequestChannelTalk` and `RequestPrivateTalk`, which take the field
 * the mode needs, so a mode can never reach the wire without it.
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
	bool bGround = false;

	/**
	 * The stack size Fusion32 reported for a cumulative object on this field.
	 *
	 * `bHasAmount` is false for everything non-cumulative, which is what the
	 * wire says: the amount byte is only present when the object type carries
	 * the CUMULATIVE flag. Kept so presentation never has to re-derive a
	 * count the server already stated, and so a pile can be told from a
	 * single object without asking the type table again.
	 */
	bool bHasAmount = false;
	uint8 Amount = 0;
};

/**
 * Everything SV_CMD_PLAYER_DATA carries, with an explicit unknown state.
 *
 * One command, one struct. Splitting the bar values away from the sheet values
 * would put the same server message in two places and let them disagree about
 * whether it had ever arrived; `bKnown` answers that once for all of them.
 */
struct FReal33DPlayerVitals
{
	bool bKnown = false;
	uint16 Hitpoints = 0;
	uint16 MaxHitpoints = 0;
	uint16 Mana = 0;
	uint16 MaxMana = 0;
	uint16 Level = 0;
	/** Percent of the way to the next level, as the server reports it. */
	uint8 LevelPercent = 0;
	uint32 Experience = 0;
	/** Free capacity in whole ounces, which is the unit 7.72 sends. */
	uint16 Capacity = 0;
	uint8 MagicLevel = 0;
	uint8 MagicLevelPercent = 0;
	uint8 SoulPoints = 0;
};

/** One fighting skill: its level and how far into the next one the player is. */
struct FReal33DSkill
{
	uint8 Level = 0;
	uint8 Percent = 0;
};

/**
 * The seven skills SV_CMD_PLAYER_SKILLS carries, in the order it sends them.
 *
 * Exactly seven because 7.72 has exactly seven. The later skills the 2D client
 * can draw -- criticals, leech, momentum -- do not exist on Fusion32, and a row
 * for one would be a number this client invented.
 */
struct FReal33DPlayerSkills
{
	bool bKnown = false;
	FReal33DSkill Fist;
	FReal33DSkill Club;
	FReal33DSkill Sword;
	FReal33DSkill Axe;
	FReal33DSkill Distance;
	FReal33DSkill Shielding;
	FReal33DSkill Fishing;
};

/**
 * The condition bitfield from SV_CMD_PLAYER_STATE.
 *
 * The bit values are Fusion32's own, from crplayer.cc::TPlayer::CheckState.
 * They are mirrored here rather than included so this header stays free of
 * anything under fusion32/, and the worker asserts the two agree.
 */
struct FReal33DConditions
{
	bool bKnown = false;
	uint8 Flags = 0;

	enum EFlag : uint8
	{
		Poisoned = 0x01,
		Burning = 0x02,
		Electrified = 0x04,
		Drunk = 0x08,
		ManaShield = 0x10,
		Slowed = 0x20,
		Hasted = 0x40,
		LogoutBlocked = 0x80,
	};

	bool Has(EFlag Flag) const { return (Flags & static_cast<uint8>(Flag)) != 0; }
};

/**
 * One object, as Fusion32 describes one: a type id and, when the type says so,
 * a liquid colour or a stack amount. Nothing else is on the wire.
 */
struct FReal33DItem
{
	uint16 TypeId = 0;
	bool bHasAmount = false;
	uint8 Amount = 0;
	bool bHasLiquidColour = false;
	uint8 LiquidColour = 0;
};

/**
 * What the player is wearing, indexed by the server's own slot numbers.
 *
 * Index 0 is unused so a slot number off the wire indexes this directly, which
 * is the same choice WorldState makes. `bOccupied` is false for a slot the
 * server has emptied, which is a different thing from an item of type zero.
 */
struct FReal33DInventory
{
	bool bKnown = false;
	static constexpr int32 SlotCount = 11;
	static constexpr int32 FirstSlot = 1;
	static constexpr int32 LastSlot = 10;

	bool bOccupied[SlotCount] = {};
	FReal33DItem Items[SlotCount];
};

/**
 * One container the player has open, mirroring SV_CMD_CONTAINER.
 *
 * `Objects` is in the server's own order: index 0 is the front of its object
 * list, which is where SV_CMD_CREATE_IN_CONTAINER prepends and what the slot
 * index of a change or a delete addresses.
 */
struct FReal33DContainer
{
	bool bOpen = false;
	uint8 Number = 0;
	uint16 TypeId = 0;
	FString Name;
	uint8 Capacity = 0;
	/** True when it sits inside another container. The server says no more. */
	bool bHasParent = false;
	TArray<FReal33DItem> Objects;
};

/**
 * Which of the three 7.72 fight stances, and whether to chase.
 *
 * Semantic here; the byte each becomes is ClientCore's business. The values
 * exist on the wire -- CL_CMD_SET_TACTICS carries all three fields -- which is
 * why these controls are live rather than dithered.
 */
UENUM()
enum class EReal33DAttackMode : uint8
{
	Offensive,
	Balanced,
	Defensive
};

UENUM()
enum class EReal33DChaseMode : uint8
{
	Stand,
	Follow
};

/**
 * Who the player is attacking or following, and the tactics last requested.
 *
 * A copy of ClientCore's `CombatState`. The target is what this client asked
 * for and Fusion32 has not revoked: the protocol has no "attack accepted"
 * command, so that is the whole of what can be known. Nothing here is decided
 * in Unreal.
 */
struct FReal33DCombat
{
	uint32 TargetCreatureId = 0;
	bool bFollowing = false;
	/** False until this client has actually sent a CL_CMD_SET_TACTICS. */
	bool bTacticsSent = false;
	EReal33DAttackMode AttackMode = EReal33DAttackMode::Balanced;
	EReal33DChaseMode ChaseMode = EReal33DChaseMode::Stand;
};

/** One row of the battle list: a creature the client can currently see. */
struct FReal33DBattleEntry
{
	uint32 CreatureId = 0;
	FString Name;
	uint8 HealthPercent = 100;
	bool bIsLocalPlayer = false;
	/** Chebyshev distance from the local player, for the classic nearest-first order. */
	int32 Distance = 0;
	/** Derived from WorldState's one combat record; never owned by the row. */
	bool bAttacked = false;
	bool bFollowed = false;
};

/** Terrain-only minimap hint, independent of live things and Actors. */
struct FReal33DMinimapCell
{
	Real33D::FMapPosition Position;
	uint16 GroundType = 0;
	uint16 ColorType = 0;
	bool bStaticObstacle = false;
	bool bLive = false;
};
struct FReal33DMinimapState
{
	Real33D::FMapPosition Player;
	bool bPlayerKnown = false;
	int32 KnownCells = 0;
	int32 LiveCells = 0;
	uint64 Revision = 0;
};

/**
 * One row of SV_CMD_CHANNELS, as the server sent it.
 *
 * `Id` and `Name` are both the server's. Nothing here is looked up in a table
 * of this client's own: `operate.cc::GetChannelName` returns the player's own
 * guild name for channel 0, so even the "fixed" channels cannot be named
 * locally without eventually naming one of them wrongly.
 */
struct FReal33DChannelEntry
{
	int32 Id = 0;
	FString Name;
	/**
	 * True for the one row that is not a channel: SendChannels writes id
	 * 0xFFFF and the name "Private Chat Channel" as an *offer* to create a
	 * channel when `MayOpenChannel` is true. Joining it would be meaningless,
	 * so the row is flagged rather than presented as joinable, and `Id` is not
	 * a channel number.
	 */
	bool bOwnChannelOffer = false;
};

/** Which kind of conversation a chat surface carries. Mirrors Real33D::ChatViewKind. */
UENUM()
enum class EReal33DChatViewKind : uint8
{
	Local,
	Channel,
	Private
};

/**
 * One chat surface: Local Chat, a joined channel, or a private conversation.
 *
 * Identity only. The lines themselves are fetched per view with
 * `GetChatTranscript`, so a widget cannot hold a transcript that has since been
 * evicted or cleared.
 */
struct FReal33DChatView
{
	/** Monotonic and never reused, including across a reconnect. */
	uint32 ViewId = 0;
	EReal33DChatViewKind Kind = EReal33DChatViewKind::Local;
	/** Meaningful for Channel only. */
	int32 Channel = 0;
	/** The server's channel name, or the character. Empty for Local Chat. */
	FString Name;
	/** The channel arrived as SV_CMD_OPEN_OWN_CHANNEL rather than the plain one. */
	bool bOwnChannel = false;
};

/**
 * A text or list editor Fusion32 opened, and everything it sent with it.
 *
 * One struct for both commands because one window shows both and only one can
 * be open at a time: the server sends a fresh one whenever the player uses
 * another object, and the newer one replaces the older.
 *
 * NOTHING HERE SAYS WHETHER THE OBJECT CAN BE WRITTEN. `SendEditText` sets
 * `MaxLength` to the type's limit minus one for a WRITE or WRITEONCE object
 * and to `strlen(Text)` for anything else, so `MaxLength == Text.Len()` does
 * not prove read-only, and no other field marks one either. `CEditText`
 * re-checks the WRITE and WRITEONCE flags itself and answers NOTACCESSIBLE,
 * so writability is Fusion32's ruling and this client does not predict it.
 * Guessing here is exactly the invention this milestone must not make.
 */
struct FReal33DEditor
{
	bool bOpen = false;
	/** True for SV_CMD_EDIT_TEXT, false for SV_CMD_EDIT_LIST. */
	bool bIsText = true;

	// Text form.
	/**
	 * The object the edit is submitted against, not a map position, so the
	 * text can be sent after the window has been open for a while.
	 */
	uint32 ObjectId = 0;
	uint16 TypeId = 0;
	/**
	 * The longest text Fusion32 will take back, when the object is writable.
	 * `CEditText` answers NOROOM for anything at or beyond the type's own
	 * limit, and `SendEditText` reports that limit minus one, so a text no
	 * longer than this is exactly the text the server accepts.
	 */
	uint16 MaxLength = 0;
	/** Who last wrote this text. Empty when nobody has, which is a fact. */
	FString Editor;

	// List form.
	EReal33DEditListType ListType = EReal33DEditListType::Unknown;
	/** A house id for Guest and Subowner, an object id for Door. */
	uint32 ListId = 0;

	/** The body, in both forms. */
	FString Text;
};

/** An event crossing the thread boundary. Copied, never shared. */
struct FReal33DEvent
{
	EReal33DEventKind Kind = EReal33DEventKind::AnchorMoved;
	Real33D::FMapPosition Position;
	Real33D::FMapPosition PreviousPosition;
	uint32 CreatureId = 0;
	FString CreatureName;
	/** Appearance already decoded into WorldState, never inferred from name/id. */
	uint16 OutfitId = 0;
	bool bDisguisedAsObject = false;
	/** enums.hh direction, or kNoDirection when the event has none. */
	uint8 Direction = 0;
	bool bIsLocalPlayer = false;
	TArray<FReal33DThing> Things;
	TArray<FReal33DMinimapCell> MinimapCells;
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
	 * True when this talk was addressed to this player alone: TALK_PRIVATE_MESSAGE.
	 *
	 * A flag rather than a mode comparison above the bridge. `TalkLayout` cannot
	 * answer it -- a private message and a gamemaster request are both `Plain`
	 * -- and `TalkMode` is a display string, so deciding it by matching that
	 * text would put a protocol fact in a label.
	 */
	bool bPrivateMessage = false;
	/**
	 * How the speaker was identified, for evidence: `Resolved`, `NoMatch`,
	 * `Ambiguous` or `NotPositional`. Anything but `Resolved` means
	 * `CreatureId` is zero and the message belongs on the fallback surface,
	 * because showing it above a guessed creature would be worse than not
	 * showing it in the world at all.
	 */
	FString SpeakerResolution;

	// Channels. Populated only for the four channel event kinds.
	/** The whole list, for ChannelList. */
	TArray<FReal33DChannelEntry> Channels;
	/** The channel's own name, for ChannelOpened and PrivateChannelOpened. */
	FString ChannelName;
	/** ChannelOpened arrived as SV_CMD_OPEN_OWN_CHANNEL. */
	bool bOwnChannel = false;

	/** Populated only for EditTextOpened and EditListOpened. */
	FReal33DEditor Editor;

	/** 0..100 as the server reports it. Meaningful for creature events. */
	uint8 HealthPercent = 100;
	FReal33DPlayerVitals Vitals;
	FReal33DPlayerSkills Skills;
	FReal33DConditions Conditions;
	FReal33DInventory Inventory;
	FReal33DContainer Container;
	FReal33DCombat Combat;
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
	/** Move requests this client put on the wire. */
	int32 MovesRequested = 0;
	/** Use requests this client put on the wire, in all three shapes. */
	int32 UsesRequested = 0;
	/** Look-at-point requests this client put on the wire. */
	int32 LooksRequested = 0;
	/** CL_CMD_ATTACK commands put on the wire, including attack-target toggles. */
	int32 AttacksRequested = 0;
	/** CL_CMD_FOLLOW commands put on the wire. */
	int32 FollowsRequested = 0;
	/** General CL_CMD_CANCEL commands put on the wire. */
	int32 CombatCancelsRequested = 0;
	/** CL_CMD_SET_TACTICS commands put on the wire. */
	int32 TacticsRequested = 0;
	/** Target revocations received from Fusion32 as SV_CMD_CLEAR_TARGET. */
	int32 TargetClearsReceived = 0;
	/** Say commands ClientCore refused before sending, with the reason logged. */
	int32 SaysRefusedLocally = 0;

	// Channels. Requests and arrivals are counted apart, as walks are: a join
	// this client sent is not proof the server opened anything.
	/** CL_CMD_GET_CHANNELS commands put on the wire. */
	int32 ChannelListsRequested = 0;
	/** CL_CMD_JOIN_CHANNEL commands put on the wire. */
	int32 ChannelJoinsRequested = 0;
	/** CL_CMD_LEAVE_CHANNEL commands put on the wire. */
	int32 ChannelLeavesRequested = 0;
	/** CL_CMD_PRIVATE_CHANNEL commands put on the wire. */
	int32 PrivateChannelsRequested = 0;
	/** CL_CMD_TALK commands sent in TALK_CHANNEL_CALL mode. */
	int32 ChannelSaysRequested = 0;
	/** CL_CMD_TALK commands sent in TALK_PRIVATE_MESSAGE mode. */
	int32 PrivateSaysRequested = 0;
	/** SV_CMD_CHANNELS lists Fusion32 sent. */
	int32 ChannelListsReceived = 0;
	/** SV_CMD_OPEN_CHANNEL and SV_CMD_OPEN_OWN_CHANNEL commands received. */
	int32 ChannelsOpened = 0;
	/** SV_CMD_CLOSE_CHANNEL commands received. */
	int32 ChannelsClosed = 0;
	/** SV_CMD_PRIVATE_CHANNEL commands received. */
	int32 PrivateChannelsOpened = 0;
	/** Talk that arrived with a channel number. */
	int32 ChannelTalkReceived = 0;
	/** Talk that arrived in TALK_PRIVATE_MESSAGE mode. */
	int32 PrivateTalkReceived = 0;

	// Editors. Opens are the server's, submissions are this client's.
	/** SV_CMD_EDIT_TEXT commands received. */
	int32 EditTextsOpened = 0;
	/** SV_CMD_EDIT_LIST commands received. */
	int32 EditListsOpened = 0;
	/** CL_CMD_EDIT_TEXT commands put on the wire. */
	int32 EditTextsSubmitted = 0;
	/** CL_CMD_EDIT_LIST commands put on the wire. */
	int32 EditListsSubmitted = 0;
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
	 * Where a move starts or ends. Semantic: the wire's special-coordinate
	 * encoding is built inside ClientCore, which is the only layer allowed to
	 * know that a container is y = 64 + its number.
	 */
	struct FMoveSlot
	{
		enum class EKind : uint8 { Map, Inventory, Container };
		EKind Kind = EKind::Inventory;
		/** Inventory slot, or the slot within the container. */
		uint8 Slot = 0;
		/** Which open container, 0-based. Ignored unless Kind is Container. */
		uint8 Container = 0;
		/** The field, when Kind is Map. */
		Real33D::FMapPosition Position;

		static FMoveSlot InInventory(uint8 InSlot);
		static FMoveSlot InContainer(uint8 InContainer, uint8 InSlot);
		static FMoveSlot OnMap(const Real33D::FMapPosition& InPosition);
	};

	/**
	 * Asks Fusion32 to move an object. An intent, exactly like a walk.
	 *
	 * Nothing moves on screen here. The server decides, and what comes back is
	 * the container and inventory commands that say what actually happened; a
	 * refused move simply produces none, which is the correct outcome because
	 * the object did not move.
	 *
	 * `Count` is how many of a stack. CMoveObject refuses a cumulative object
	 * with a count of zero, so a whole non-stackable object passes one.
	 *
	 * Returns the id identifying this request for the rest of its life.
	 */
	uint32 RequestMoveObject(const FMoveSlot& From, uint16 TypeId, uint8 StackIndex,
		const FMoveSlot& To, uint8 Count);

	/**
	 * Uses an object. CL_CMD_USE_OBJECT.
	 *
	 * `OpenAsContainer` is the open-container slot the server should show the
	 * object in when it turns out to be a container. It is part of the command
	 * and Fusion32 refuses one whose value is out of range, so the caller picks
	 * a free slot rather than leaving it zero and having a second bag replace
	 * the first.
	 *
	 * An intent, like everything else here. Whether the object does anything is
	 * Fusion32's ruling; what comes back -- a container window, a message, a
	 * changed tile -- is what this client then draws.
	 */
	uint32 RequestUseObject(const FMoveSlot& Object, uint16 TypeId, uint8 StackIndex,
		uint8 OpenAsContainer);

	/** Uses an object on another object. CL_CMD_USE_TWO_OBJECTS. */
	uint32 RequestUseWithObject(const FMoveSlot& Object, uint16 TypeId, uint8 StackIndex,
		const FMoveSlot& Target, uint16 TargetTypeId, uint8 TargetStackIndex);

	/** Uses an object on a creature, named by id. CL_CMD_USE_ON_CREATURE. */
	uint32 RequestUseOnCreature(const FMoveSlot& Object, uint16 TypeId, uint8 StackIndex,
		uint32 CreatureId);

	/** Asks Fusion32 to describe the object at this point via SV_CMD_MESSAGE. */
	uint32 RequestLookAtPoint(const FMoveSlot& Point);

	/**
	 * Attacks a creature, or cancels when that exact attack is already active.
	 *
	 * The toggle decision is made on the worker from WorldState, not in Slate.
	 * An accepted target is silent on 7.72, so the worker records it only after
	 * the command reaches the wire. Fusion32 can then revoke it with
	 * SV_CMD_CLEAR_TARGET.
	 */
	uint32 RequestAttack(uint32 CreatureId);

	/** Follows a creature, or cancels when that exact follow is already active. */
	uint32 RequestFollow(uint32 CreatureId);

	/** Sends Fusion32's general cancel command (CL_CMD_CANCEL). */
	uint32 RequestCancelCombat();

	/** Changes only the attack stance; ClientCore preserves the other tactics. */
	uint32 RequestAttackMode(EReal33DAttackMode Mode);

	/** Changes only stand/chase; ClientCore preserves the other tactics. */
	uint32 RequestChaseMode(EReal33DChaseMode Mode);

	// --------------------------------------------------------------- channels
	//
	// Four commands out, five in. Every one of them is an intent, exactly like
	// a walk: none of these opens, closes or fills a tab by itself. A channel
	// appears when Fusion32 answers with SV_CMD_OPEN_CHANNEL, and what travels
	// inside it is the ordinary SV_CMD_TALK the server broadcasts back.
	//
	// Each returns the id identifying the request for the rest of its life, or
	// 0 when there is no live worker to hand it to.

	/**
	 * Asks for the channel list. CL_CMD_GET_CHANNELS.
	 *
	 * The answer is the only thing that may name a channel on screen. Until it
	 * arrives this client knows of no channels at all, which is why the panel
	 * starts empty rather than with the eight `operate.hh` happens to define:
	 * which of them exist for this player is `ChannelAvailable`'s answer, and
	 * that is server state.
	 */
	uint32 RequestChannelList();

	/**
	 * Joins a channel. CL_CMD_JOIN_CHANNEL.
	 *
	 * Refused locally for `kRuleViolationsChannel` and for a number outside a
	 * word, and for nothing else: CJoinChannel checks the number against
	 * `GetNumberOfChannels()`, which is server state this client has no copy
	 * of, so a range cannot be enforced here without inventing one.
	 */
	uint32 RequestJoinChannel(int32 Channel);

	/**
	 * Leaves a channel. CL_CMD_LEAVE_CHANNEL.
	 *
	 * The one command here whose effect is applied without waiting:
	 * CLeaveChannel answers with nothing at all, so this client's own command
	 * is the only signal the channel has been left. The tab therefore closes
	 * here. A line already in flight when it closes is not lost -- it arrives
	 * in Local Chat, see `Real33DChatViews.h`.
	 */
	uint32 RequestLeaveChannel(int32 Channel);

	/**
	 * Opens a private conversation with a character by name.
	 * CL_CMD_PRIVATE_CHANNEL.
	 *
	 * The character must exist but need not be online: CPrivateChannel only
	 * calls `GetCharacterID`. An unknown name is answered with
	 * PLAYERNOTEXISTING, and the player's own name with a refusal message --
	 * both of which arrive as server messages the player reads, so neither is
	 * second-guessed here.
	 */
	uint32 RequestOpenPrivateChannel(const FString& Name);

	/**
	 * Speaks into a channel. CL_CMD_TALK in TALK_CHANNEL_CALL mode.
	 *
	 * Fusion32 broadcasts the line to every subscriber, this player included,
	 * so what fills the tab is the server's own copy and not a local echo.
	 *
	 * Two of its refusals are the server's to make and are not pre-empted
	 * here: speaking into a channel needs level 2 or better, and a channel
	 * this player is not subscribed to silently becomes an ordinary Say. Both
	 * are visible -- the first as a server message, the second as speech that
	 * appears in Local Chat instead of the tab.
	 */
	uint32 RequestChannelTalk(int32 Channel, const FString& Text);

	/**
	 * Sends a private message. CL_CMD_TALK in TALK_PRIVATE_MESSAGE mode.
	 *
	 * The addressee has to be online: `operate.cc` throws PLAYERNOTONLINE
	 * otherwise, and the player is told so.
	 *
	 * The sender gets no copy of their own line -- Fusion32 sends the receiver
	 * the talk and the sender "Message sent to X." through SV_CMD_MESSAGE -- so
	 * a sender's tab shows that confirmation and not the line. Echoing it
	 * locally would show the player a message the server might have refused.
	 */
	uint32 RequestPrivateTalk(const FString& Addressee, const FString& Text);

	/**
	 * The channel list Fusion32 last sent, and whether it has ever answered.
	 *
	 * Empty and false before the first SV_CMD_CHANNELS. The panel draws nothing
	 * in that state rather than a plausible list.
	 */
	void GetChannelList(TArray<FReal33DChannelEntry>& OutChannels) const;
	bool IsChannelListKnown() const;

	/** Changes whenever the list does, so the panel rebuilds only then. */
	uint64 GetChannelListRevision() const;

	/** Whether Fusion32 has confirmed this player is in a channel. */
	bool IsChannelJoined(int32 Channel) const;

	/**
	 * The chat surfaces, Local Chat first and then in the order they opened.
	 *
	 * Local Chat is always present and is never removed, so there is always
	 * somewhere for an unaddressed line to go.
	 */
	void GetChatViews(TArray<FReal33DChatView>& OutViews) const;

	/** The surface the player is looking at, and the one a typed line goes to. */
	uint32 GetSelectedChatView() const;
	bool SelectChatView(uint32 ViewId);

	/**
	 * Closes a surface the player is done with.
	 *
	 * A channel view leaves the channel on the way out, which is what the 2D's
	 * tab close does. Local Chat is refused.
	 */
	bool CloseChatView(uint32 ViewId);

	/**
	 * The transcript of what the player should be able to read, oldest first.
	 *
	 * Fed by DrainEvents from speech and server messages, never from protocol
	 * diagnostics. Bounded and ordered by ClientCore's `ChatLog`, which owns
	 * those rules because they are testable without a renderer.
	 *
	 * The no-argument form is Local Chat, which is what it has always been.
	 *
	 * Game thread only, like DrainEvents: one owner on one thread.
	 */
	void GetChatTranscript(TArray<FReal33DChatLine>& OutLines) const;

	/** The transcript of one surface. Empty for a view that is not open. */
	void GetChatTranscript(uint32 ViewId, TArray<FReal33DChatLine>& OutLines) const;

	/**
	 * Changes whenever any transcript or the set of surfaces does, so a widget
	 * can rebuild only then rather than every frame. Never reused, including
	 * across a reconnect.
	 *
	 * One revision for all of them on purpose: a tab strip and a transcript
	 * drawn from two separate counters could show a tab whose conversation had
	 * already gone.
	 */
	uint64 GetChatRevision() const;

	// ---------------------------------------------------------------- editors
	//
	// Fusion32 opens an editor; this client only submits one. There is no
	// command that asks for a text window, and none that closes one: a book
	// is read by USING it, which the HUD already does, and a house list by
	// casting one of the three house spells, which the player already types.

	/**
	 * The editor Fusion32 last opened, if one is open. Game thread only.
	 *
	 * Replaced wholesale when a newer one arrives, because the server treats
	 * it as one window: using a second book while the first is open sends a
	 * second SV_CMD_EDIT_TEXT and expects the first to be gone.
	 */
	const FReal33DEditor& GetEditor() const;

	/** Changes whenever the open editor does, so a widget rebuilds only then. */
	uint64 GetEditorRevision() const;

	/**
	 * Closes the window locally. Returns false when none was open.
	 *
	 * Local only, and there is nothing to tell the server: neither CEditText
	 * nor CEditList has a cancel, and Fusion32 keeps no record that a window
	 * was ever open. Closing therefore discards an unsent edit and nothing
	 * else.
	 */
	bool CloseEditor();

	/**
	 * Submits edited text. CL_CMD_EDIT_TEXT against the object id the matching
	 * SV_CMD_EDIT_TEXT carried.
	 *
	 * AN INTENT, AND THE RESULT IS SILENT ON SUCCESS. `operate.cc::EditText`
	 * writes the text and the editor name and sends nothing back; a refusal
	 * arrives as an ordinary SV_CMD_MESSAGE through `SendResult`
	 * ("Sorry, not possible." for NOTACCESSIBLE, "There is not enough room."
	 * for NOROOM). So this client must never report that an edit was saved.
	 * The only proof of acceptance is reading the object again and seeing the
	 * new text and attribution, which is what the live QA does.
	 *
	 * Returns the id identifying the request, or 0 when nothing was sent.
	 */
	uint32 RequestEditText(uint32 ObjectId, const FString& Text);

	/**
	 * Submits an edited house or door list. CL_CMD_EDIT_LIST.
	 *
	 * `Id` is a house id for Guest and Subowner and an object id for Door,
	 * exactly as `CEditList` reads it. Silent on success for the same reason;
	 * `ChangeGuests`, `ChangeSubowners` and `ChangeNameDoor` answer only when
	 * they refuse.
	 */
	uint32 RequestEditList(EReal33DEditListType Type, uint32 Id, const FString& Text);

	/**
	 * Channel 3, CHANNEL_RULEVIOLATIONS.
	 *
	 * Named here because this client refuses to join it. `CJoinChannel`
	 * answers that one with SV_CMD_OPEN_REQUEST_QUEUE (174) and the existing
	 * requests, and this project decodes neither; an undecoded command stops
	 * the payload walk, so asking for it would cost the rest of that packet.
	 * Refusing is a client limitation and is reported as one.
	 */
	static constexpr int32 kRuleViolationsChannel = 3;

	FReal33DStats GetStats() const;
	FReal33DMinimapState GetMinimapState() const;
	void GetMinimapCells(int32 Floor, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
		TArray<FReal33DMinimapCell>& OutCells) const;
	bool FindMinimapPath(Real33D::FMapPosition Start, Real33D::FMapPosition Goal,
		const TArray<Real33D::FMapPosition>& ObservedBlockers,
		TArray<Real33D::FMapPosition>& OutPath) const;

	/** Existing runtime objects.srv flag, never guessed from an item's id/name. */
	bool RequiresUseTarget(uint16 TypeId) const;

	/** Reads the connection config from the command line, or a sensible default. */
	static FReal33DConnectionConfig ConfigFromCommandLine();

private:
	/** Files one drained event into the transcript, if it belongs there. */
	void NoteChat(const FReal33DEvent& Event);

	/**
	 * Applies one drained channel event: opens, closes, or records the list.
	 *
	 * Separate from NoteChat because the two answer different questions. This
	 * one decides which surfaces exist; that one decides which surface a line
	 * goes on. Both run on the game thread, in arrival order, so a line that
	 * arrives in the same drain as the open that created its tab lands in it.
	 */
	void NoteChannel(const FReal33DEvent& Event);

	/** Applies one drained editor event: opens the window, or clears it. */
	void NoteEditor(const FReal33DEvent& Event);

	FReal33DWorker* Worker = nullptr;
	FRunnableThread* Thread = nullptr;
	/** Game thread only. Numbers key presses so evidence can follow one. */
	uint32 NextInputId = 0;

	/**
	 * The chat surfaces, one ClientCore ChatLog each, and the channel list
	 * Fusion32 last sent. Held behind a pointer so this header stays free of
	 * anything under fusion32/. Created in Initialize, destroyed in
	 * Deinitialize, game thread only.
	 *
	 * One pointer for all of it rather than one per thing: a surface and its
	 * transcript are created and destroyed together, and splitting them would
	 * let a log outlive the conversation it belongs to.
	 *
	 * A raw pointer rather than a TUniquePtr on purpose: a smart pointer would
	 * instantiate its deleter wherever this header is included, including UHT's
	 * generated constructor, and deleting an incomplete type there compiles to a
	 * delete that skips the destructor. With a raw pointer every line that knows
	 * what this is lives in the .cpp.
	 */
	struct FReal33DChatTranscript* Transcript = nullptr;
	struct FReal33DMinimapStore* Minimap = nullptr;
	void NoteMinimap(const FReal33DEvent& Event);
	void SaveMinimap();
};
