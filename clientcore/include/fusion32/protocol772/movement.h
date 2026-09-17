#ifndef FUSION32_PROTOCOL772_MOVEMENT_H
#define FUSION32_PROTOCOL772_MOVEMENT_H

#include "fusion32/protocol772/initial_world.h"
#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/object_types.h"
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
