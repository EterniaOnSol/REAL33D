#ifndef FUSION32_PROTOCOL772_MOVEMENT_H
#define FUSION32_PROTOCOL772_MOVEMENT_H

#include "fusion32/protocol772/initial_world.h"
#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/object_types.h"
#include "fusion32/protocol772/player_state.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Client commands. Source: reference/game/src/connections.hh::ClientCommand and
// reference/game/src/receiving.cc::ReceiveData, whose dispatch calls
// CGoDirection with fixed offsets and reads nothing further from the buffer.
// A cardinal walk is therefore a one-byte payload.
constexpr std::uint8_t kClientCommandGoNorth = 101;
constexpr std::uint8_t kClientCommandGoEast = 102;
constexpr std::uint8_t kClientCommandGoSouth = 103;
constexpr std::uint8_t kClientCommandGoWest = 104;
constexpr std::uint8_t kClientCommandGoStop = 105;
constexpr std::uint8_t kClientCommandRotateNorth = 111;
constexpr std::uint8_t kClientCommandRotateEast = 112;
constexpr std::uint8_t kClientCommandRotateSouth = 113;
constexpr std::uint8_t kClientCommandRotateWest = 114;
// reference/game/src/connections.hh: CL_CMD_TALK = 150.
constexpr std::uint8_t kClientCommandTalk = 150;

// Fusion32 also accepts CL_CMD_GO_NORTHEAST (106) through CL_CMD_GO_NORTHWEST
// (109), routed through the same CGoDirection with both offsets non-zero, and
// charges them three times the walk delay in
// reference/game/src/cract.cc::TCreature::NotifyGo. They are intentionally not
// exposed here; see docs/protocol772/MOVEMENT.md.

// Server command values used by this layer. Source:
// reference/game/src/connections.hh::ServerCommand.
constexpr std::uint8_t kServerCommandRowNorth = 101;
constexpr std::uint8_t kServerCommandRowEast = 102;
constexpr std::uint8_t kServerCommandRowSouth = 103;
constexpr std::uint8_t kServerCommandRowWest = 104;
constexpr std::uint8_t kServerCommandFieldData = 105;
constexpr std::uint8_t kServerCommandAddField = 106;
constexpr std::uint8_t kServerCommandChangeField = 107;
constexpr std::uint8_t kServerCommandDeleteField = 108;
constexpr std::uint8_t kServerCommandMoveCreature = 109;
// reference/game/src/connections.hh: SV_CMD_TALK = 170.
constexpr std::uint8_t kServerCommandTalk = 170;
constexpr std::uint8_t kServerCommandMessage = 180;
constexpr std::uint8_t kServerCommandSnapback = 181;
constexpr std::uint8_t kServerCommandFloorUp = 190;
constexpr std::uint8_t kServerCommandFloorDown = 191;

// Source: reference/game/src/enums.hh, enum Direction.
enum class CardinalDirection : std::uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3,
};

std::uint8_t WalkCommandOpcode(CardinalDirection direction) noexcept;
std::uint8_t TurnCommandOpcode(CardinalDirection direction) noexcept;
const char* CardinalDirectionName(CardinalDirection direction) noexcept;

// The offsets receiving.cc::ReceiveData passes to CGoDirection.
MapPosition StepPosition(const MapPosition& from, CardinalDirection direction) noexcept;

// One-byte client payloads, ready to be XTEA-encrypted and framed.
std::vector<std::uint8_t> BuildWalkCommand(CardinalDirection direction);
std::vector<std::uint8_t> BuildTurnCommand(CardinalDirection direction);
std::vector<std::uint8_t> BuildStopCommand();

// Keepalive and clean exit. Source: reference/game/src/receiving.cc::CPing,
// which reads nothing, and CQuitGame, which logs out immediately when
// LogoutPossible returns 0.
std::vector<std::uint8_t> BuildPingCommand();
std::vector<std::uint8_t> BuildLogoutCommand();

/**
 * Where a move takes an object from, or puts one.
 *
 * Three shapes, and the wire says which by the coordinate itself rather than
 * by a tag: an x of 0xFFFF is not a map field, and the y then names either a
 * body slot or, in the CONTAINER_FIRST..CONTAINER_LAST range, an open
 * container whose z is the slot inside it. Source:
 * reference/game/src/receiving.cc::CheckSpecialCoordinates.
 *
 * The three named constructors exist so a caller states which of the three it
 * means and cannot assemble a coordinate that satisfies none of them.
 */
struct MoveEndpoint {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint8_t z = 0;

    static MoveEndpoint OnMap(const MapPosition& position);
    static MoveEndpoint InInventory(std::uint8_t slot);
    static MoveEndpoint InContainer(std::uint8_t container, std::uint8_t slot);
};

/** A currently carried instance, resolved from server-owned state at send time. */
struct CarriedItemLocation {
    MoveEndpoint endpoint;
    std::uint8_t stack_index = 0;
};

/**
 * Finds the first actual instance of a bound type: body slots in server order,
 * then open containers by number and object index. A binding stores a type,
 * never a stale coordinate. Closed containers and absent items cannot resolve.
 */
bool ResolveCarriedItem(const WorldState& state, std::uint16_t type_id,
                        CarriedItemLocation* out);

/**
 * Asks Fusion32 to move an object. CL_CMD_MOVE_OBJECT, opcode 120.
 *
 * `stack_index` is the object's position where it currently is: the stack
 * position on a map field, or the slot within a container. CMoveObject passes
 * it to `GetObject`, so it has to be the index the server would use, not a
 * guess.
 *
 * `count` is how many of a cumulative stack to move. CMoveObject rejects a
 * cumulative object with a count of zero outright, so a caller moving a whole
 * non-stackable object passes one.
 *
 * This only asks. The server decides, and the authoritative answer arrives as
 * the container and inventory commands that follow -- nothing here changes
 * WorldState, exactly as a walk request changes no position.
 */
std::vector<std::uint8_t> BuildMoveObjectCommand(const MoveEndpoint& from,
                                                 std::uint16_t type_id,
                                                 std::uint8_t stack_index,
                                                 const MoveEndpoint& to,
                                                 std::uint8_t count);

/**
 * Uses an object. CL_CMD_USE_OBJECT, opcode 130.
 *
 * `container` is not padding. reference/game/src/receiving.cc::CUseObject
 * reads it as the index of the open-container slot the object should be
 * shown in when the object is a container, and refuses the command outright
 * if it is not below the player's open-container table size. For anything
 * that is not a container the server ignores it.
 *
 * CUseObject also refuses an object whose type carries MULTIUSE: those are
 * the ones that need a second target, and the server expects
 * CL_CMD_USE_TWO_OBJECTS or CL_CMD_USE_ON_CREATURE for them instead. This
 * function does not know the object's flags and does not guess -- the caller
 * asks for the shape of use it means, and Fusion32 rules on it.
 */
std::vector<std::uint8_t> BuildUseObjectCommand(const MoveEndpoint& object,
                                                std::uint16_t type_id,
                                                std::uint8_t stack_index,
                                                std::uint8_t container);

/**
 * Uses one object on another. CL_CMD_USE_TWO_OBJECTS, opcode 131.
 *
 * Both ends are full positions, so the target may be an object on a map
 * field, in a container or in a body slot. Source: CUseTwoObjects.
 */
std::vector<std::uint8_t> BuildUseTwoObjectsCommand(const MoveEndpoint& object,
                                                    std::uint16_t type_id,
                                                    std::uint8_t stack_index,
                                                    const MoveEndpoint& target,
                                                    std::uint16_t target_type_id,
                                                    std::uint8_t target_stack_index);

/**
 * Uses an object on a creature. CL_CMD_USE_ON_CREATURE, opcode 132.
 *
 * The target is named by creature id rather than by position, which is what
 * lets it keep working while the creature is moving. Source: CUseOnCreature.
 */
std::vector<std::uint8_t> BuildUseOnCreatureCommand(const MoveEndpoint& object,
                                                    std::uint16_t type_id,
                                                    std::uint8_t stack_index,
                                                    std::uint32_t creature_id);

// ------------------------------------------------------------ combat commands
//
// Fusion32 has exactly four. Source: reference/game/src/connections.hh for the
// opcodes and receiving.cc for the bodies, where CL_CMD_ATTACK and
// CL_CMD_FOLLOW are the same handler, CAttack, told apart by one bool.

constexpr std::uint8_t kClientCommandSetTactics = 160;
constexpr std::uint8_t kClientCommandAttack = 161;
constexpr std::uint8_t kClientCommandFollow = 162;
constexpr std::uint8_t kClientCommandCancel = 190;

// Source: reference/game/src/enums.hh. CSetTactics refuses any other value
// outright, so these are the whole of what 7.72 accepts.
enum class AttackMode : std::uint8_t {
    Offensive = 1,
    Balanced = 2,
    Defensive = 3,
};

// CHASE_MODE_RANGE exists in the server enum but CSetTactics rejects it: the
// switch there accepts NONE and CLOSE only. It is deliberately absent here,
// because offering it would build a command the server drops on the floor.
enum class ChaseMode : std::uint8_t {
    Stand = 0,   // CHASE_MODE_NONE
    Follow = 1,  // CHASE_MODE_CLOSE
};

enum class SecureMode : std::uint8_t {
    Disabled = 0,
    Enabled = 1,
};

/**
 * Attacks a creature. CL_CMD_ATTACK, opcode 161.
 *
 * The body is the target's creature id and nothing else. Source:
 * receiving.cc::CAttack, which reads one quad and hands it to
 * `TCombat::SetAttackDest(TargetID, false)`.
 *
 * A creature id of zero is the documented cancel: SetAttackDest treats 0, and
 * the player's own id, as "stop", and answers with SV_CMD_CLEAR_TARGET. This
 * only asks. Fusion32 says nothing when it accepts a target; it speaks only to
 * refuse, and then it both clears the target and sends the reason as a failure
 * message.
 */
std::vector<std::uint8_t> BuildAttackCommand(std::uint32_t creature_id);

/**
 * Follows a creature. CL_CMD_FOLLOW, opcode 162.
 *
 * The same body and the same handler as the attack, with `Follow` true. The
 * difference is on the server: a following combat skips every attack
 * permission check, never strikes, and forces CHASE_MODE_CLOSE so the player
 * walks after the target. Source: `TCombat::SetAttackDest` and
 * `TCombat::CanToDoAttack`.
 */
std::vector<std::uint8_t> BuildFollowCommand(std::uint32_t creature_id);

/**
 * Stops attacking or following. CL_CMD_CANCEL, opcode 190, no body.
 *
 * Not the same as an attack on creature zero. CCancel also clears the player's
 * to-do list and sends a snapback when it had something to clear, so it stops
 * a walk in progress as well; `BuildAttackCommand(0)` stops only the combat.
 * Both end in `StopAttack(0)` and therefore in SV_CMD_CLEAR_TARGET.
 */
std::vector<std::uint8_t> BuildCancelCommand();

/**
 * Sets the three tactics. CL_CMD_SET_TACTICS, opcode 160.
 *
 * Three bytes, in this order: attack mode, chase mode, secure mode. Source:
 * receiving.cc::CSetTactics.
 *
 * Fusion32 never reports these back. There is no server command carrying them
 * and none of them appears in SV_CMD_PLAYER_DATA or SV_CMD_PLAYER_STATE, so a
 * client knows only what it last sent. WorldState records them as exactly
 * that and says so.
 */
std::vector<std::uint8_t> BuildSetTacticsCommand(AttackMode attack,
                                                 ChaseMode chase,
                                                 SecureMode secure);

const char* AttackModeName(AttackMode mode) noexcept;
const char* ChaseModeName(ChaseMode mode) noexcept;

/**
 * Records in WorldState the target this client has just asked Fusion32 for.
 *
 * Call it when the command reaches the wire, not before. The server
 * acknowledges an accepted target with nothing at all, so this is the only
 * moment the id is knowable; SV_CMD_CLEAR_TARGET is what takes it away again.
 *
 * The two cancel spellings are mirrored from `TCombat::SetAttackDest`, which
 * treats a target id of zero and the player's own id alike: both stop the
 * attack rather than start one. Passing either clears the stored target, so a
 * caller cannot end up believing it is attacking itself.
 */
void NoteCombatRequest(WorldState* state, std::uint32_t creature_id,
                       bool following);

/** Records the tactics this client has just put on the wire. */
void NoteTacticsRequest(WorldState* state, AttackMode attack, ChaseMode chase,
                        SecureMode secure);

// ---------------------------------------------------------------- server side

enum class ServerUpdateKind {
    FullScreen,
    Row,
    FloorChange,
    FieldData,
    AddField,
    ChangeField,
    DeleteField,
    MoveCreature,
    Snapback,
    Message,
    Talk,
    Ping,
    Ambient,
    GraphicalEffect,
    TextualEffect,
    MissileEffect,
    MarkCreature,
    CreatureAttribute,
    PlayerData,
    PlayerSkills,
    PlayerState,
    ClearTarget,
    Inventory,
    Container,
    Buddy,
    OutfitDialog,
    Unsupported,
};

const char* ServerUpdateKindName(ServerUpdateKind kind) noexcept;

struct RowUpdate {
    CardinalDirection direction = CardinalDirection::North;
    std::vector<MapFloor> floors;
};

struct FloorChangeUpdate {
    bool going_up = false;
    std::vector<MapFloor> floors;  // empty when the client already has them
};

struct FieldDataUpdate {
    MapPosition position;
    MapTile tile;  // things may be empty, meaning the field was cleared
};

struct AddFieldUpdate {
    MapPosition position;
    MapThing thing;
};

struct ChangeFieldUpdate {
    MapPosition position;
    std::uint8_t stack_index = 0;
    MapThing thing;
};

struct DeleteFieldUpdate {
    MapPosition position;
    std::uint8_t stack_index = 0;
};

struct MoveCreatureUpdate {
    MapPosition origin;
    std::uint8_t origin_stack_index = 0;
    MapPosition destination;
};

struct SnapbackUpdate {
    std::uint8_t direction = 0;
};

struct MessageUpdate {
    std::uint8_t mode = 0;
    std::string text;
};

// Talk. Source: reference/game/src/sending.cc declares three SendTalk
// overloads, all emitting SV_CMD_TALK, all beginning
//
//     byte  SV_CMD_TALK
//     quad  StatementID
//     string Sender          (uint16 length + bytes, per SendString)
//     byte  Mode
//
// and then differing. The mode decides which tail follows, and the overload
// that accepts a mode is the only thing that says which tail it is:
//
//   SendTalk(..., int x, int y, int z, Text)   "positional"
//       modes SAY, WHISPER, YELL, ANIMAL_LOW, ANIMAL_LOUD
//       word x, word y, byte z, string Text
//
//   SendTalk(..., int Channel, Text)           "channel"
//       modes CHANNEL_CALL, GAMEMASTER_CHANNELCALL, HIGHLIGHT_CHANNELCALL,
//             ANONYMOUS_CHANNELCALL
//       word Channel, string Text
//       ANONYMOUS_CHANNELCALL sends an empty Sender rather than omitting it.
//
//   SendTalk(..., const char *Text, int Data)  "plain"
//       modes PRIVATE_MESSAGE, GAMEMASTER_REQUEST, GAMEMASTER_ANSWER,
//             PLAYER_ANSWER, GAMEMASTER_BROADCAST, GAMEMASTER_MESSAGE
//       quad Data, but only when the mode is GAMEMASTER_REQUEST
//       string Text
//
// TALK_MODE in enums.hh also declares ANONYMOUS_BROADCAST (13) and
// ANONYMOUS_MESSAGE (15), which no overload accepts, and 18..23, which belong
// to SendMessage and a different opcode. None of those can arrive here.
enum class TalkMode : std::uint8_t {
    Say = 1,
    Whisper = 2,
    Yell = 3,
    PrivateMessage = 4,
    ChannelCall = 5,
    GamemasterRequest = 6,
    GamemasterAnswer = 7,
    PlayerAnswer = 8,
    GamemasterBroadcast = 9,
    GamemasterChannelCall = 10,
    GamemasterMessage = 11,
    HighlightChannelCall = 12,
    AnonymousChannelCall = 14,
    AnimalLow = 16,
    AnimalLoud = 17,
};

// Which tail the mode implies. Derived from the overload that accepts it.
enum class TalkLayout {
    Positional,
    Channel,
    Plain,
    // No SendTalk overload accepts this mode, so the tail is unknown.
    Unsupported,
};

TalkLayout TalkLayoutForMode(std::uint8_t mode) noexcept;
const char* TalkModeName(std::uint8_t mode) noexcept;

// The modes SV_CMD_MESSAGE carries. These are TALK_MODE values too, but they
// travel under a different opcode and no SendTalk overload accepts them, so
// TalkModeName does not name them: asking it would answer "UnknownTalkMode"
// about a mode that is perfectly well known. Source:
// reference/game/src/sending.cc::SendMessage and enums.hh.
const char* MessageModeName(std::uint8_t mode) noexcept;

struct TalkUpdate {
    std::uint32_t statement_id = 0;
    // Empty for ANONYMOUS_CHANNELCALL, which the server blanks deliberately,
    // and for the ANIMAL modes, whose only caller passes "".
    std::string speaker;
    std::uint8_t mode = 0;
    TalkLayout layout = TalkLayout::Unsupported;
    std::string text;

    // Present only for the positional modes.
    bool has_position = false;
    MapPosition position;

    // Present only for the channel modes.
    bool has_channel = false;
    std::uint16_t channel = 0;

    // Present only for GAMEMASTER_REQUEST, which is the one mode whose overload
    // writes an extra quad between the mode and the text.
    bool has_request_data = false;
    std::uint32_t request_data = 0;
};

struct ServerUpdate {
    ServerUpdateKind kind = ServerUpdateKind::Unsupported;
    std::uint8_t opcode = 0;
    const char* name = "Unknown";
    std::size_t offset = 0;
    std::size_t bytes_consumed = 0;

    // The viewport anchor this command leaves behind. Equal to the incoming
    // anchor for every command except Row and FloorChange.
    MapPosition resulting_anchor;

    FullScreenMessage fullscreen;
    RowUpdate row;
    FloorChangeUpdate floor_change;
    FieldDataUpdate field_data;
    AddFieldUpdate add_field;
    ChangeFieldUpdate change_field;
    DeleteFieldUpdate delete_field;
    MoveCreatureUpdate move_creature;
    SnapbackUpdate snapback;
    MessageUpdate message;
    TalkUpdate talk;

    AmbientUpdate ambient;
    GraphicalEffectUpdate graphical_effect;
    TextualEffectUpdate textual_effect;
    MissileEffectUpdate missile_effect;
    MarkCreatureUpdate mark_creature;
    CreatureAttributeUpdate creature_attribute;
    PlayerDataUpdate player_data;
    PlayerSkillsUpdate player_skills;
    PlayerStateUpdate player_state;
    InventoryUpdate inventory;
    ContainerUpdate container;
    BuddyUpdate buddy;
    OutfitDialogUpdate outfit_dialog;
};

struct ServerUpdateDecodeResult {
    ServerUpdate update;
    MapDecodeError error = MapDecodeError::None;
    std::size_t error_offset = 0;
    std::string detail;

    bool ok() const noexcept { return error == MapDecodeError::None; }
};

// Decodes exactly one server command starting at `offset`.
//
// SV_CMD_ROW_* and SV_CMD_FLOOR_UP/DOWN carry no coordinates: the server built
// them from the player position it had already advanced one axis at a time in
// TCreature::NotifyGo. `anchor` must therefore be the viewport anchor as of
// the previous command, and the decoder applies the same axis step the server
// did before reporting `resulting_anchor`.
//
// An opcode this layer does not decode yields ServerUpdateKind::Unsupported
// with the opcode named and `bytes_consumed` zero, so the caller knows exactly
// where the stream stopped and nothing is discarded.
ServerUpdateDecodeResult DecodeServerUpdate(const std::vector<std::uint8_t>& bytes,
                                            std::size_t offset,
                                            const MapPosition& anchor,
                                            const ObjectTypeTable& types);

// The floor set SendFloors emits for a step that changed z. Source:
// reference/game/src/sending.cc::SendFloors lines 525-558. Returns count 0 when
// the client already holds every floor it needs, in which case the command is
// a single opcode byte.
FloorRange FloorChangeRange(std::int32_t anchor_z, bool going_up) noexcept;

// Applies one decoded command. Map geometry, tile stacks and the creature
// mirror are updated in place; anomalies are reported, never swallowed. A
// rejected step (Snapback / Message) changes no map state at all.
WorldStateApplyResult ApplyServerUpdate(WorldState* state, const ServerUpdate& update,
                                        const ObjectTypeTable& types);

// The priority PlaceObject would assign to this thing.
ObjectPriority ThingPriority(const MapThing& thing, const ObjectTypeTable& types) noexcept;

}  // namespace fusion32::protocol772

#endif
