#include "fusion32/protocol772/movement.h"

#include <algorithm>

namespace fusion32::protocol772 {
namespace {

struct RowRect {
    std::int32_t min_x = 0;
    std::int32_t min_y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

// Source: reference/game/src/sending.cc::SendRow lines 487-502, which collapses
// one axis of the full viewport onto the edge the step revealed.
RowRect RowRectangle(const MapPosition& anchor, CardinalDirection direction) noexcept {
    const std::int32_t min_x = anchor.x - kTerminalOffsetX;
    const std::int32_t min_y = anchor.y - kTerminalOffsetY;
    const std::int32_t max_x = min_x + kTerminalWidth - 1;
    const std::int32_t max_y = min_y + kTerminalHeight - 1;
    switch (direction) {
        case CardinalDirection::North: return {min_x, min_y, kTerminalWidth, 1};
        case CardinalDirection::South: return {min_x, max_y, kTerminalWidth, 1};
        case CardinalDirection::East: return {max_x, min_y, 1, kTerminalHeight};
        case CardinalDirection::West: return {min_x, min_y, 1, kTerminalHeight};
    }
    return {};
}

bool WithinFloorWindow(const MapPosition& anchor, const MapPosition& position) noexcept {
    const std::int32_t offset = anchor.z - position.z;
    const std::int32_t min_x = anchor.x - kTerminalOffsetX + offset;
    const std::int32_t min_y = anchor.y - kTerminalOffsetY + offset;
    return position.x >= min_x && position.x < min_x + kTerminalWidth
        && position.y >= min_y && position.y < min_y + kTerminalHeight;
}

MapFloor* FindOrCreateFloor(WorldState* state, std::int32_t z, std::int32_t offset) {
    for (MapFloor& floor : state->floors) {
        if (floor.z == z) {
            floor.offset = offset;
            return &floor;
        }
    }
    MapFloor created;
    created.z = z;
    created.offset = offset;
    state->floors.push_back(std::move(created));
    return &state->floors.back();
}

void MergeFloor(WorldState* state, const MapFloor& incoming, const MapPosition& anchor) {
    MapFloor* floor = FindOrCreateFloor(state, incoming.z, anchor.z - incoming.z);
    for (const MapTile& tile : incoming.tiles) {
        bool replaced = false;
        for (MapTile& existing : floor->tiles) {
            if (existing.position == tile.position) {
                existing = tile;
                replaced = true;
                break;
            }
        }
        if (!replaced) floor->tiles.push_back(tile);
    }
}

// A row or floor command only describes the newly revealed fields. Everything
// the viewport left behind must go, or the state would accumulate tiles the
// server no longer considers visible.
void PruneToAnchor(WorldState* state) {
    const FloorRange range = ViewportFloorRange(state->viewport_anchor.z);
    const std::int32_t low = std::min(range.first, range.last);
    const std::int32_t high = std::max(range.first, range.last);

    std::vector<MapFloor> kept;
    kept.reserve(state->floors.size());
    for (MapFloor& floor : state->floors) {
        if (floor.z < low || floor.z > high) continue;
        floor.offset = state->viewport_anchor.z - floor.z;
        std::vector<MapTile> tiles;
        tiles.reserve(floor.tiles.size());
        for (MapTile& tile : floor.tiles) {
            if (!WithinFloorWindow(state->viewport_anchor, tile.position)) continue;
            tiles.push_back(std::move(tile));
        }
        floor.tiles = std::move(tiles);
        kept.push_back(std::move(floor));
    }

    // Keep the server's emission order: descending above ground, ascending
    // below it.
    const std::int32_t step = range.step;
    std::sort(kept.begin(), kept.end(), [step](const MapFloor& a, const MapFloor& b) {
        return step < 0 ? a.z > b.z : a.z < b.z;
    });
    state->floors = std::move(kept);
    state->window = state->AnchoredWindow();
}

std::vector<ObjectPriority> TilePriorities(const MapTile& tile,
                                           const ObjectTypeTable& types) {
    std::vector<ObjectPriority> priorities;
    priorities.reserve(tile.things.size());
    for (const MapThing& thing : tile.things) {
        priorities.push_back(ThingPriority(thing, types));
    }
    return priorities;
}

MapTile* EnsureTile(WorldState* state, const MapPosition& position) {
    MapTile* tile = state->FindTile(position);
    if (tile != nullptr) return tile;
    MapFloor* floor = FindOrCreateFloor(state, position.z,
                                        state->viewport_anchor.z - position.z);
    MapTile created;
    created.position = position;
    floor->tiles.push_back(std::move(created));
    return &floor->tiles.back();
}

void RemoveTileIfEmpty(WorldState* state, const MapPosition& position) {
    for (MapFloor& floor : state->floors) {
        if (floor.z != position.z) continue;
        for (std::size_t i = 0; i < floor.tiles.size(); ++i) {
            if (floor.tiles[i].position != position) continue;
            if (floor.tiles[i].things.empty()) {
                floor.tiles.erase(floor.tiles.begin() + static_cast<std::ptrdiff_t>(i));
            }
            return;
        }
    }
}

void RecordCreature(WorldState* state, const CreatureThing& creature,
                    const MapPosition& position, std::size_t stack,
                    std::vector<WorldStateAnomaly>* anomalies) {
    // See the matching note in ApplyFullScreen: a word-97 whose evicted id
    // equals the introduced id is the server reusing this creature's own slot,
    // which is what every relog produces. It evicts nothing.
    if (creature.evicts_slot && creature.removed_creature_id != 0
        && creature.removed_creature_id != creature.creature_id) {
        if (state->known_creatures.erase(creature.removed_creature_id) == 0) {
            anomalies->push_back({WorldStateAnomalyKind::UnknownEvictedCreature,
                                  creature.removed_creature_id, position});
        }
    }
    const auto existing = state->known_creatures.find(creature.creature_id);
    if (!creature.has_descriptor && existing == state->known_creatures.end()) {
        anomalies->push_back({WorldStateAnomalyKind::UnknownCreatureReference,
                              creature.creature_id, position});
    }

    CreatureRecord& record = state->known_creatures[creature.creature_id];
    record.creature_id = creature.creature_id;
    record.position = position;
    record.stack_position = stack;
    record.direction = creature.direction;
    if (creature.has_name) record.name = creature.name;
    if (creature.has_descriptor) {
        record.has_descriptor = true;
        record.health_percent = creature.health_percent;
        record.outfit = creature.outfit;
        record.light_brightness = creature.light_brightness;
        record.light_color = creature.light_color;
        record.speed = creature.speed;
        record.playerkilling_mark = creature.playerkilling_mark;
        record.party_mark = creature.party_mark;
    }
}

// Every creature whose stack position shifted must have its record refreshed,
// or the mirror would report a stale index.
void RefreshTileCreatures(WorldState* state, const MapTile& tile) {
    for (std::size_t i = 0; i < tile.things.size(); ++i) {
        const MapThing& thing = tile.things[i];
        if (thing.kind != MapThingKind::Creature) continue;
        const auto found = state->known_creatures.find(thing.creature.creature_id);
        if (found == state->known_creatures.end()) continue;
        found->second.position = tile.position;
        found->second.stack_position = i;
    }
}

}  // namespace

std::uint8_t WalkCommandOpcode(CardinalDirection direction) noexcept {
    switch (direction) {
        case CardinalDirection::North: return kClientCommandGoNorth;
        case CardinalDirection::East: return kClientCommandGoEast;
        case CardinalDirection::South: return kClientCommandGoSouth;
        case CardinalDirection::West: return kClientCommandGoWest;
    }
    return 0;
}

std::uint8_t TurnCommandOpcode(CardinalDirection direction) noexcept {
    switch (direction) {
        case CardinalDirection::North: return kClientCommandRotateNorth;
        case CardinalDirection::East: return kClientCommandRotateEast;
        case CardinalDirection::South: return kClientCommandRotateSouth;
        case CardinalDirection::West: return kClientCommandRotateWest;
    }
    return 0;
}

const char* CardinalDirectionName(CardinalDirection direction) noexcept {
    switch (direction) {
        case CardinalDirection::North: return "North";
        case CardinalDirection::East: return "East";
        case CardinalDirection::South: return "South";
        case CardinalDirection::West: return "West";
    }
    return "Unknown";
}

MapPosition StepPosition(const MapPosition& from, CardinalDirection direction) noexcept {
    MapPosition to = from;
    switch (direction) {
        case CardinalDirection::North: to.y -= 1; break;
        case CardinalDirection::East: to.x += 1; break;
        case CardinalDirection::South: to.y += 1; break;
        case CardinalDirection::West: to.x -= 1; break;
    }
    return to;
}

std::vector<std::uint8_t> BuildWalkCommand(CardinalDirection direction) {
    return {WalkCommandOpcode(direction)};
}

std::vector<std::uint8_t> BuildTurnCommand(CardinalDirection direction) {
    return {TurnCommandOpcode(direction)};
}

std::vector<std::uint8_t> BuildStopCommand() {
    return {kClientCommandGoStop};
}

std::vector<std::uint8_t> BuildPingCommand() {
    return {kClientCommandPing};
}

std::vector<std::uint8_t> BuildLogoutCommand() {
    return {kClientCommandLogout};
}

MoveEndpoint MoveEndpoint::OnMap(const MapPosition& position) {
    MoveEndpoint endpoint;
    endpoint.x = static_cast<std::uint16_t>(position.x);
    endpoint.y = static_cast<std::uint16_t>(position.y);
    endpoint.z = static_cast<std::uint8_t>(position.z);
    return endpoint;
}

MoveEndpoint MoveEndpoint::InInventory(std::uint8_t slot) {
    MoveEndpoint endpoint;
    endpoint.x = kSpecialCoordinateX;
    endpoint.y = slot;
    endpoint.z = 0;
    return endpoint;
}

MoveEndpoint MoveEndpoint::InContainer(std::uint8_t container, std::uint8_t slot) {
    MoveEndpoint endpoint;
    endpoint.x = kSpecialCoordinateX;
    // The container number becomes a coordinate only here, on the wire. Every
    // other place in this client indexes containers from zero.
    endpoint.y = static_cast<std::uint16_t>(kContainerCoordinateFirst + container);
    endpoint.z = slot;
    return endpoint;
}

bool ResolveCarriedItem(const WorldState& state, std::uint16_t type_id,
                        CarriedItemLocation* out) {
    if (out == nullptr || type_id == 0) return false;
    for (std::uint8_t slot = 1; slot < state.inventory.size(); ++slot) {
        const auto& current = state.inventory[slot];
        if (current.occupied && current.item.type_id == type_id) {
            // info.cc::GetObject ignores RNum for body slots. Use the canonical
            // zero from the client fixture, never a made-up map stack index.
            *out = {MoveEndpoint::InInventory(slot), 0};
            return true;
        }
    }
    for (std::uint8_t number = 0; number < state.containers.size(); ++number) {
        const auto& container = state.containers[number];
        if (!container.open) continue;
        for (std::size_t index = 0; index < container.objects.size()
                && index <= 255; ++index) {
            if (container.objects[index].type_id == type_id) {
                const auto slot = static_cast<std::uint8_t>(index);
                *out = {MoveEndpoint::InContainer(number, slot), slot};
                return true;
            }
        }
    }
    return false;
}

namespace {

// Origin, type and stack index: the five fields every use command starts with,
// laid out the same way in all three of CUseObject, CUseTwoObjects and
// CUseOnCreature.
void AppendObjectRef(std::vector<std::uint8_t>* command, const MoveEndpoint& where,
                     std::uint16_t type_id, std::uint8_t stack_index) {
    const auto word = [command](std::uint16_t value) {
        command->push_back(static_cast<std::uint8_t>(value & 0xFF));
        command->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    };
    word(where.x);
    word(where.y);
    command->push_back(where.z);
    word(type_id);
    command->push_back(stack_index);
}

}  // namespace

std::vector<std::uint8_t> BuildUseObjectCommand(const MoveEndpoint& object,
                                                std::uint16_t type_id,
                                                std::uint8_t stack_index,
                                                std::uint8_t container) {
    std::vector<std::uint8_t> command;
    command.reserve(10);
    command.push_back(kClientCommandUseObject);
    AppendObjectRef(&command, object, type_id, stack_index);
    command.push_back(container);
    return command;
}

std::vector<std::uint8_t> BuildUseTwoObjectsCommand(const MoveEndpoint& object,
                                                    std::uint16_t type_id,
                                                    std::uint8_t stack_index,
                                                    const MoveEndpoint& target,
                                                    std::uint16_t target_type_id,
                                                    std::uint8_t target_stack_index) {
    std::vector<std::uint8_t> command;
    command.reserve(17);
    command.push_back(kClientCommandUseTwoObjects);
    AppendObjectRef(&command, object, type_id, stack_index);
    AppendObjectRef(&command, target, target_type_id, target_stack_index);
    return command;
}

std::vector<std::uint8_t> BuildUseOnCreatureCommand(const MoveEndpoint& object,
                                                    std::uint16_t type_id,
                                                    std::uint8_t stack_index,
                                                    std::uint32_t creature_id) {
    std::vector<std::uint8_t> command;
    command.reserve(13);
    command.push_back(kClientCommandUseOnCreature);
    AppendObjectRef(&command, object, type_id, stack_index);
    command.push_back(static_cast<std::uint8_t>(creature_id & 0xFF));
    command.push_back(static_cast<std::uint8_t>((creature_id >> 8) & 0xFF));
    command.push_back(static_cast<std::uint8_t>((creature_id >> 16) & 0xFF));
    command.push_back(static_cast<std::uint8_t>((creature_id >> 24) & 0xFF));
    return command;
}

namespace {

// CAttack reads one quad and nothing else, for both opcodes.
std::vector<std::uint8_t> BuildCombatTargetCommand(std::uint8_t opcode,
                                                   std::uint32_t creature_id) {
    std::vector<std::uint8_t> command;
    command.reserve(5);
    command.push_back(opcode);
    command.push_back(static_cast<std::uint8_t>(creature_id & 0xFF));
    command.push_back(static_cast<std::uint8_t>((creature_id >> 8) & 0xFF));
    command.push_back(static_cast<std::uint8_t>((creature_id >> 16) & 0xFF));
    command.push_back(static_cast<std::uint8_t>((creature_id >> 24) & 0xFF));
    return command;
}

}  // namespace

std::vector<std::uint8_t> BuildAttackCommand(std::uint32_t creature_id) {
    return BuildCombatTargetCommand(kClientCommandAttack, creature_id);
}

std::vector<std::uint8_t> BuildFollowCommand(std::uint32_t creature_id) {
    return BuildCombatTargetCommand(kClientCommandFollow, creature_id);
}

std::vector<std::uint8_t> BuildCancelCommand() {
    return std::vector<std::uint8_t>{kClientCommandCancel};
}

std::vector<std::uint8_t> BuildSetTacticsCommand(AttackMode attack,
                                                 ChaseMode chase,
                                                 SecureMode secure) {
    return std::vector<std::uint8_t>{
        kClientCommandSetTactics,
        static_cast<std::uint8_t>(attack),
        static_cast<std::uint8_t>(chase),
        static_cast<std::uint8_t>(secure),
    };
}

const char* AttackModeName(AttackMode mode) noexcept {
    switch (mode) {
        case AttackMode::Offensive: return "Offensive";
        case AttackMode::Balanced: return "Balanced";
        case AttackMode::Defensive: return "Defensive";
    }
    return "Unknown";
}

const char* ChaseModeName(ChaseMode mode) noexcept {
    switch (mode) {
        case ChaseMode::Stand: return "Stand";
        case ChaseMode::Follow: return "Follow";
    }
    return "Unknown";
}

void NoteCombatRequest(WorldState* state, std::uint32_t creature_id,
                       bool following) {
    if (state == nullptr) {
        return;
    }
    // SetAttackDest reads both of these as "stop", so neither can leave a
    // target behind.
    if (creature_id == 0 || creature_id == state->local_creature_id) {
        state->combat.target_creature_id = 0;
        state->combat.following = false;
        return;
    }
    state->combat.target_creature_id = creature_id;
    state->combat.following = following;
}

void NoteTacticsRequest(WorldState* state, AttackMode attack, ChaseMode chase,
                        SecureMode secure) {
    if (state == nullptr) {
        return;
    }
    state->combat.tactics_sent = true;
    state->combat.attack_mode = static_cast<std::uint8_t>(attack);
    state->combat.chase_mode = static_cast<std::uint8_t>(chase);
    state->combat.secure_mode = static_cast<std::uint8_t>(secure);
}

std::vector<std::uint8_t> BuildMoveObjectCommand(const MoveEndpoint& from,
                                                 std::uint16_t type_id,
                                                 std::uint8_t stack_index,
                                                 const MoveEndpoint& to,
                                                 std::uint8_t count) {
    // CMoveObject's own read order: origin word/word/byte, type word, stack
    // byte, destination word/word/byte, count byte.
    std::vector<std::uint8_t> command;
    command.reserve(14);
    const auto word = [&command](std::uint16_t value) {
        command.push_back(static_cast<std::uint8_t>(value & 0xFF));
        command.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    };
    command.push_back(kClientCommandMoveObject);
    word(from.x);
    word(from.y);
    command.push_back(from.z);
    word(type_id);
    command.push_back(stack_index);
    word(to.x);
    word(to.y);
    command.push_back(to.z);
    command.push_back(count);
    return command;
}

ObjectPriority ThingPriority(const MapThing& thing, const ObjectTypeTable& types) noexcept {
    if (thing.kind == MapThingKind::Creature) return ObjectPriority::Creature;
    return types.Lookup(thing.item.type_id).priority;
}

FloorRange FloorChangeRange(std::int32_t anchor_z, bool going_up) noexcept {
    FloorRange range;
    if (going_up) {
        if (anchor_z == kSurfaceFloor) {
            // Reaching the surface: the client already holds [10, 6].
            range.step = -1;
            range.first = 5;
            range.last = 0;
            range.count = 6;
        } else if (anchor_z > kSurfaceFloor) {
            range.step = -1;
            range.first = anchor_z - 2;
            range.last = anchor_z - 2;
            range.count = 1;
        }
    } else {
        if (anchor_z == kSurfaceFloor + 1) {
            // Going underground: the client already holds [7, 0].
            range.step = 1;
            range.first = kSurfaceFloor + 1;
            range.last = 10;
            range.count = 3;
        } else if (anchor_z > kSurfaceFloor + 1 && (anchor_z + 2) <= kMaxFloor) {
            range.step = 1;
            range.first = anchor_z + 2;
            range.last = anchor_z + 2;
            range.count = 1;
        }
    }
    return range;
}

ServerUpdateDecodeResult DecodeServerUpdate(const std::vector<std::uint8_t>& bytes,
                                            std::size_t offset,
                                            const MapPosition& anchor,
                                            const ObjectTypeTable& types) {
    ServerUpdateDecodeResult result;
    result.update.offset = offset;
    result.update.resulting_anchor = anchor;

    if (types.empty()) {
        result.error = MapDecodeError::EmptyObjectTypeTable;
        result.error_offset = offset;
        result.detail = "item lengths depend on the server object type flags";
        return result;
    }
    if (offset >= bytes.size()) {
        result.error = MapDecodeError::Truncated;
        result.error_offset = offset;
        result.detail = "empty payload";
        return result;
    }

    const std::uint8_t opcode = bytes[offset];
    result.update.opcode = opcode;
    result.update.name = ServerCommandName(opcode);

    const auto fail = [&result](MapScanner& scanner) {
        result.error = scanner.error();
        result.error_offset = scanner.error_offset();
        result.detail = scanner.detail();
        return result;
    };

    if (opcode == kServerCommandFullScreen) {
        result.update.kind = ServerUpdateKind::FullScreen;
        const auto decoded = DecodeFullScreen(bytes, offset, types);
        if (!decoded.ok()) {
            result.error = decoded.error;
            result.error_offset = decoded.error_offset;
            result.detail = decoded.detail;
            return result;
        }
        result.update.fullscreen = decoded.message;
        result.update.bytes_consumed = decoded.message.bytes_consumed;
        result.update.resulting_anchor = decoded.message.window.player_position;
        return result;
    }

    MapScanner scanner(bytes, offset + 1, types);

    if (opcode == kServerCommandRowNorth || opcode == kServerCommandRowEast
        || opcode == kServerCommandRowSouth || opcode == kServerCommandRowWest) {
        result.update.kind = ServerUpdateKind::Row;
        CardinalDirection direction = CardinalDirection::North;
        if (opcode == kServerCommandRowEast) direction = CardinalDirection::East;
        else if (opcode == kServerCommandRowSouth) direction = CardinalDirection::South;
        else if (opcode == kServerCommandRowWest) direction = CardinalDirection::West;
        result.update.row.direction = direction;

        // NotifyGo advances the player one axis before calling SendRow, so the
        // revealed edge belongs to the stepped position, not the previous one.
        const MapPosition stepped = StepPosition(anchor, direction);
        if (stepped.z > kMaxFloor || stepped.z < 0) {
            result.error = MapDecodeError::InvalidPlayerFloor;
            result.error_offset = offset;
            result.detail = "row anchor left the addressable floors";
            return result;
        }
        result.update.resulting_anchor = stepped;

        const RowRect rect = RowRectangle(stepped, direction);
        const FloorRange range = ViewportFloorRange(stepped.z);
        const std::int32_t end_z = range.last + range.step;
        for (std::int32_t z = range.first; z != end_z; z += range.step) {
            MapFloor floor;
            const std::int32_t floor_offset = stepped.z - z;
            if (!scanner.ScanRect(z, floor_offset, rect.min_x + floor_offset,
                                  rect.min_y + floor_offset, rect.width, rect.height,
                                  &floor)) {
                return fail(scanner);
            }
            result.update.row.floors.push_back(std::move(floor));
        }
        if (!scanner.FinishRun()) return fail(scanner);
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandFloorUp || opcode == kServerCommandFloorDown) {
        result.update.kind = ServerUpdateKind::FloorChange;
        const bool going_up = opcode == kServerCommandFloorUp;
        result.update.floor_change.going_up = going_up;

        // NotifyGo shifts x and y by one alongside z before calling SendFloors.
        MapPosition stepped = anchor;
        if (going_up) {
            stepped.x += 1;
            stepped.y += 1;
            stepped.z -= 1;
        } else {
            stepped.x -= 1;
            stepped.y -= 1;
            stepped.z += 1;
        }
        if (stepped.z > kMaxFloor || stepped.z < 0) {
            result.error = MapDecodeError::InvalidPlayerFloor;
            result.error_offset = offset;
            result.detail = "floor change left the addressable floors";
            return result;
        }
        result.update.resulting_anchor = stepped;

        const FloorRange range = FloorChangeRange(stepped.z, going_up);
        if (range.count > 0) {
            const std::int32_t min_x = stepped.x - kTerminalOffsetX;
            const std::int32_t min_y = stepped.y - kTerminalOffsetY;
            const std::int32_t end_z = range.last + range.step;
            for (std::int32_t z = range.first; z != end_z; z += range.step) {
                MapFloor floor;
                const std::int32_t floor_offset = stepped.z - z;
                if (!scanner.ScanRect(z, floor_offset, min_x + floor_offset,
                                      min_y + floor_offset, kTerminalWidth,
                                      kTerminalHeight, &floor)) {
                    return fail(scanner);
                }
                result.update.floor_change.floors.push_back(std::move(floor));
            }
            if (!scanner.FinishRun()) return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandFieldData) {
        result.update.kind = ServerUpdateKind::FieldData;
        MapPosition position;
        if (!ReadFieldPosition(&scanner, &position)) return fail(scanner);
        result.update.field_data.position = position;
        MapFloor floor;
        // SendFieldData scans exactly one absolute position, with no offset.
        if (!scanner.ScanRect(position.z, 0, position.x, position.y, 1, 1, &floor)) {
            return fail(scanner);
        }
        if (!scanner.FinishRun()) return fail(scanner);
        result.update.field_data.tile.position = position;
        if (!floor.tiles.empty()) result.update.field_data.tile = floor.tiles.front();
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandAddField) {
        result.update.kind = ServerUpdateKind::AddField;
        if (!ReadFieldPosition(&scanner, &result.update.add_field.position)) {
            return fail(scanner);
        }
        if (!ReadStandaloneMapThing(&scanner, &result.update.add_field.thing)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandChangeField) {
        result.update.kind = ServerUpdateKind::ChangeField;
        if (!ReadFieldPosition(&scanner, &result.update.change_field.position)) {
            return fail(scanner);
        }
        if (!ReadScannerByte(&scanner, &result.update.change_field.stack_index,
                             "change field stack index")) {
            return fail(scanner);
        }
        if (result.update.change_field.stack_index >= kMapObjectsPerPointLimit) {
            result.error = MapDecodeError::InvalidStackIndex;
            result.error_offset = scanner.at() - 1;
            result.detail = "stack index at or above MAX_OBJECTS_PER_POINT";
            return result;
        }
        if (!ReadStandaloneMapThing(&scanner, &result.update.change_field.thing)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandDeleteField) {
        result.update.kind = ServerUpdateKind::DeleteField;
        if (!ReadFieldPosition(&scanner, &result.update.delete_field.position)) {
            return fail(scanner);
        }
        if (!ReadScannerByte(&scanner, &result.update.delete_field.stack_index,
                             "delete field stack index")) {
            return fail(scanner);
        }
        if (result.update.delete_field.stack_index >= kMapObjectsPerPointLimit) {
            result.error = MapDecodeError::InvalidStackIndex;
            result.error_offset = scanner.at() - 1;
            result.detail = "stack index at or above MAX_OBJECTS_PER_POINT";
            return result;
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandMoveCreature) {
        result.update.kind = ServerUpdateKind::MoveCreature;
        if (!ReadFieldPosition(&scanner, &result.update.move_creature.origin)) {
            return fail(scanner);
        }
        if (!ReadScannerByte(&scanner, &result.update.move_creature.origin_stack_index,
                             "move creature stack index")) {
            return fail(scanner);
        }
        if (result.update.move_creature.origin_stack_index >= kMapObjectsPerPointLimit) {
            result.error = MapDecodeError::InvalidStackIndex;
            result.error_offset = scanner.at() - 1;
            result.detail = "stack index at or above MAX_OBJECTS_PER_POINT";
            return result;
        }
        if (!ReadFieldPosition(&scanner, &result.update.move_creature.destination)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandSnapback) {
        result.update.kind = ServerUpdateKind::Snapback;
        if (!ReadScannerByte(&scanner, &result.update.snapback.direction,
                             "snapback direction")) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandTalk) {
        result.update.kind = ServerUpdateKind::Talk;
        TalkUpdate& talk = result.update.talk;

        // The common head every SendTalk overload writes.
        if (!ReadScannerQuad(&scanner, &talk.statement_id, "talk statement id")) {
            return fail(scanner);
        }
        if (!ReadScannerString(&scanner, &talk.speaker, "talk speaker")) {
            return fail(scanner);
        }
        if (!ReadScannerByte(&scanner, &talk.mode, "talk mode")) {
            return fail(scanner);
        }

        // The mode is what says which tail follows. An unrecognised one makes
        // the rest of the command unlocatable, so this stops rather than
        // guessing a length and desynchronising the stream.
        talk.layout = TalkLayoutForMode(talk.mode);
        if (talk.layout == TalkLayout::Unsupported) {
            result.error = MapDecodeError::UnknownTalkMode;
            result.error_offset = scanner.at() - 1;
            result.detail = "no SendTalk overload accepts this talk mode";
            return result;
        }

        if (talk.layout == TalkLayout::Positional) {
            std::uint16_t x = 0;
            std::uint16_t y = 0;
            std::uint8_t z = 0;
            if (!ReadScannerWord(&scanner, &x, "talk x")) return fail(scanner);
            if (!ReadScannerWord(&scanner, &y, "talk y")) return fail(scanner);
            if (!ReadScannerByte(&scanner, &z, "talk z")) return fail(scanner);
            talk.has_position = true;
            talk.position.x = static_cast<std::int32_t>(x);
            talk.position.y = static_cast<std::int32_t>(y);
            talk.position.z = static_cast<std::int32_t>(z);
        } else if (talk.layout == TalkLayout::Channel) {
            if (!ReadScannerWord(&scanner, &talk.channel, "talk channel")) {
                return fail(scanner);
            }
            talk.has_channel = true;
        } else if (talk.mode == static_cast<std::uint8_t>(TalkMode::GamemasterRequest)) {
            // The only mode whose overload writes a quad before the text.
            if (!ReadScannerQuad(&scanner, &talk.request_data, "talk request data")) {
                return fail(scanner);
            }
            talk.has_request_data = true;
        }

        if (!ReadScannerString(&scanner, &talk.text, "talk text")) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandMessage) {
        result.update.kind = ServerUpdateKind::Message;
        if (!ReadScannerByte(&scanner, &result.update.message.mode, "message mode")) {
            return fail(scanner);
        }
        if (!ReadScannerString(&scanner, &result.update.message.text, "message text")) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    // Zero-payload commands.
    if (opcode == kServerCommandPing || opcode == kServerCommandClearTarget) {
        result.update.kind = opcode == kServerCommandPing ? ServerUpdateKind::Ping
                                                          : ServerUpdateKind::ClearTarget;
        result.update.bytes_consumed = 1;
        return result;
    }

    if (opcode == kServerCommandAmbient) {
        result.update.kind = ServerUpdateKind::Ambient;
        if (!DecodeAmbient(&scanner, &result.update.ambient)) return fail(scanner);
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandGraphicalEffect) {
        result.update.kind = ServerUpdateKind::GraphicalEffect;
        if (!DecodeGraphicalEffect(&scanner, &result.update.graphical_effect)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandTextualEffect) {
        result.update.kind = ServerUpdateKind::TextualEffect;
        if (!DecodeTextualEffect(&scanner, &result.update.textual_effect)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandMissileEffect) {
        result.update.kind = ServerUpdateKind::MissileEffect;
        if (!DecodeMissileEffect(&scanner, &result.update.missile_effect)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandMarkCreature) {
        result.update.kind = ServerUpdateKind::MarkCreature;
        if (!DecodeMarkCreature(&scanner, &result.update.mark_creature)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode >= kServerCommandCreatureHealth && opcode <= kServerCommandCreatureParty) {
        result.update.kind = ServerUpdateKind::CreatureAttribute;
        if (!DecodeCreatureAttribute(&scanner, opcode, &result.update.creature_attribute)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandPlayerData) {
        result.update.kind = ServerUpdateKind::PlayerData;
        if (!DecodePlayerData(&scanner, &result.update.player_data)) return fail(scanner);
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandPlayerSkills) {
        result.update.kind = ServerUpdateKind::PlayerSkills;
        if (!DecodePlayerSkills(&scanner, &result.update.player_skills)) return fail(scanner);
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandPlayerState) {
        result.update.kind = ServerUpdateKind::PlayerState;
        if (!DecodePlayerState(&scanner, &result.update.player_state)) return fail(scanner);
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandSetInventory || opcode == kServerCommandDeleteInventory) {
        result.update.kind = ServerUpdateKind::Inventory;
        if (!DecodeInventory(&scanner, opcode, types, &result.update.inventory)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandContainer || opcode == kServerCommandCloseContainer
        || opcode == kServerCommandCreateInContainer
        || opcode == kServerCommandChangeInContainer
        || opcode == kServerCommandDeleteInContainer) {
        result.update.kind = ServerUpdateKind::Container;
        if (!DecodeContainer(&scanner, opcode, &result.update.container)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandBuddyData || opcode == kServerCommandBuddyOnline
        || opcode == kServerCommandBuddyOffline) {
        result.update.kind = ServerUpdateKind::Buddy;
        if (!DecodeBuddy(&scanner, opcode, &result.update.buddy)) return fail(scanner);
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    if (opcode == kServerCommandOutfitDialog) {
        result.update.kind = ServerUpdateKind::OutfitDialog;
        if (!DecodeOutfitDialog(&scanner, &result.update.outfit_dialog)) {
            return fail(scanner);
        }
        result.update.bytes_consumed = scanner.at() - offset;
        return result;
    }

    result.update.kind = ServerUpdateKind::Unsupported;
    result.update.bytes_consumed = 0;
    return result;
}

WorldStateApplyResult ApplyServerUpdate(WorldState* state, const ServerUpdate& update,
                                        const ObjectTypeTable& types) {
    WorldStateApplyResult result;
    if (state == nullptr) return result;

    switch (update.kind) {
        case ServerUpdateKind::FullScreen: {
            const auto applied = ApplyFullScreen(state, update.fullscreen);
            result.anomalies = applied.anomalies;
            return result;
        }

        case ServerUpdateKind::Row: {
            state->viewport_anchor = update.resulting_anchor;
            for (const MapFloor& floor : update.row.floors) {
                MergeFloor(state, floor, state->viewport_anchor);
            }
            PruneToAnchor(state);
            for (const MapFloor& floor : update.row.floors) {
                for (const MapTile& tile : floor.tiles) {
                    for (std::size_t i = 0; i < tile.things.size(); ++i) {
                        if (tile.things[i].kind != MapThingKind::Creature) continue;
                        RecordCreature(state, tile.things[i].creature, tile.position, i,
                                       &result.anomalies);
                    }
                }
            }
            return result;
        }

        case ServerUpdateKind::FloorChange: {
            state->viewport_anchor = update.resulting_anchor;
            for (const MapFloor& floor : update.floor_change.floors) {
                MergeFloor(state, floor, state->viewport_anchor);
            }
            PruneToAnchor(state);
            for (const MapFloor& floor : update.floor_change.floors) {
                for (const MapTile& tile : floor.tiles) {
                    for (std::size_t i = 0; i < tile.things.size(); ++i) {
                        if (tile.things[i].kind != MapThingKind::Creature) continue;
                        RecordCreature(state, tile.things[i].creature, tile.position, i,
                                       &result.anomalies);
                    }
                }
            }
            return result;
        }

        case ServerUpdateKind::FieldData: {
            if (!WithinFloorWindow(state->viewport_anchor, update.field_data.position)) {
                result.anomalies.push_back({WorldStateAnomalyKind::FieldOutsideViewport,
                                            0, update.field_data.position});
                return result;
            }
            MapTile* tile = EnsureTile(state, update.field_data.position);
            *tile = update.field_data.tile;
            const MapTile snapshot = *tile;
            RefreshTileCreatures(state, snapshot);
            for (std::size_t i = 0; i < snapshot.things.size(); ++i) {
                if (snapshot.things[i].kind != MapThingKind::Creature) continue;
                RecordCreature(state, snapshot.things[i].creature, snapshot.position, i,
                               &result.anomalies);
            }
            RemoveTileIfEmpty(state, update.field_data.position);
            return result;
        }

        case ServerUpdateKind::AddField: {
            if (!WithinFloorWindow(state->viewport_anchor, update.add_field.position)) {
                result.anomalies.push_back({WorldStateAnomalyKind::FieldOutsideViewport,
                                            0, update.add_field.position});
                return result;
            }
            MapTile* tile = EnsureTile(state, update.add_field.position);
            const std::size_t index = MapStackInsertIndex(
                TilePriorities(*tile, types), ThingPriority(update.add_field.thing, types));
            tile->things.insert(tile->things.begin() + static_cast<std::ptrdiff_t>(index),
                                update.add_field.thing);
            const MapTile snapshot = *tile;
            RefreshTileCreatures(state, snapshot);
            if (update.add_field.thing.kind == MapThingKind::Creature) {
                RecordCreature(state, update.add_field.thing.creature, snapshot.position,
                               index, &result.anomalies);
            }
            return result;
        }

        case ServerUpdateKind::ChangeField: {
            MapTile* tile = state->FindTile(update.change_field.position);
            if (tile == nullptr || update.change_field.stack_index >= tile->things.size()) {
                result.anomalies.push_back({WorldStateAnomalyKind::StackIndexOutOfRange,
                                            0, update.change_field.position});
                return result;
            }
            tile->things[update.change_field.stack_index] = update.change_field.thing;
            const MapTile snapshot = *tile;
            RefreshTileCreatures(state, snapshot);
            if (update.change_field.thing.kind == MapThingKind::Creature) {
                RecordCreature(state, update.change_field.thing.creature, snapshot.position,
                               update.change_field.stack_index, &result.anomalies);
            }
            return result;
        }

        case ServerUpdateKind::DeleteField: {
            MapTile* tile = state->FindTile(update.delete_field.position);
            if (tile == nullptr || update.delete_field.stack_index >= tile->things.size()) {
                result.anomalies.push_back({WorldStateAnomalyKind::StackIndexOutOfRange,
                                            0, update.delete_field.position});
                return result;
            }
            tile->things.erase(tile->things.begin()
                               + static_cast<std::ptrdiff_t>(update.delete_field.stack_index));
            const MapTile snapshot = *tile;
            RefreshTileCreatures(state, snapshot);
            // A removed creature leaves the map but stays in the mirror. The
            // server sends this same command whether the creature scrolled out
            // of view or was destroyed, and TConnection::KnownCreatureTable
            // only frees an entry in ~TCreature or when NewKnownCreature reuses
            // the slot. Dropping it here would make the server answer a later
            // reappearance with a word-98 or word-99 descriptor this client no
            // longer recognises. Use visible_creature_ids() for what is on the
            // map.
            RemoveTileIfEmpty(state, update.delete_field.position);
            return result;
        }

        case ServerUpdateKind::MoveCreature: {
            MapTile* origin = state->FindTile(update.move_creature.origin);
            if (origin == nullptr
                || update.move_creature.origin_stack_index >= origin->things.size()
                || origin->things[update.move_creature.origin_stack_index].kind
                       != MapThingKind::Creature) {
                result.anomalies.push_back({WorldStateAnomalyKind::MoveOriginMismatch,
                                            0, update.move_creature.origin});
                return result;
            }
            const MapThing moved = origin->things[update.move_creature.origin_stack_index];
            origin->things.erase(origin->things.begin()
                                 + static_cast<std::ptrdiff_t>(
                                       update.move_creature.origin_stack_index));
            const MapTile origin_snapshot = *origin;
            RefreshTileCreatures(state, origin_snapshot);
            RemoveTileIfEmpty(state, update.move_creature.origin);

            MapTile* destination = EnsureTile(state, update.move_creature.destination);
            const std::size_t index = MapStackInsertIndex(
                TilePriorities(*destination, types), ObjectPriority::Creature);
            destination->things.insert(
                destination->things.begin() + static_cast<std::ptrdiff_t>(index), moved);
            const MapTile destination_snapshot = *destination;
            RefreshTileCreatures(state, destination_snapshot);

            auto found = state->known_creatures.find(moved.creature.creature_id);
            if (found == state->known_creatures.end()) {
                result.anomalies.push_back({WorldStateAnomalyKind::UnknownCreatureReference,
                                            moved.creature.creature_id,
                                            update.move_creature.destination});
                CreatureRecord& record = state->known_creatures[moved.creature.creature_id];
                record.creature_id = moved.creature.creature_id;
            }
            CreatureRecord& record = state->known_creatures[moved.creature.creature_id];
            record.position = update.move_creature.destination;
            record.stack_position = index;
            return result;
        }

        case ServerUpdateKind::Snapback: {
            // A rejected step changes no map state. Only the facing the server
            // confirmed is recorded.
            if (state->local_creature_id != 0) {
                const auto found = state->known_creatures.find(state->local_creature_id);
                if (found != state->known_creatures.end()) {
                    found->second.direction = update.snapback.direction;
                }
            }
            return result;
        }

        case ServerUpdateKind::CreatureAttribute: {
            const CreatureAttributeUpdate& update_attribute = update.creature_attribute;
            const auto found = state->known_creatures.find(update_attribute.creature_id);
            if (found == state->known_creatures.end()) {
                // AnnounceChangedCreature only reaches connections that already
                // know the creature, so a miss means the local mirror is behind.
                result.anomalies.push_back({WorldStateAnomalyKind::UnknownCreatureReference,
                                            update_attribute.creature_id, MapPosition{}});
                return result;
            }
            CreatureRecord& record = found->second;
            switch (update_attribute.attribute) {
                case CreatureAttribute::Health:
                    record.health_percent = update_attribute.health_percent;
                    break;
                case CreatureAttribute::Light:
                    record.light_brightness = update_attribute.light_brightness;
                    record.light_color = update_attribute.light_color;
                    break;
                case CreatureAttribute::Outfit:
                    record.outfit = update_attribute.outfit;
                    break;
                case CreatureAttribute::Speed:
                    record.speed = update_attribute.speed;
                    break;
                case CreatureAttribute::Skull:
                    record.playerkilling_mark = update_attribute.playerkilling_mark;
                    break;
                case CreatureAttribute::Party:
                    record.party_mark = update_attribute.party_mark;
                    break;
            }
            return result;
        }

        case ServerUpdateKind::Ambient:
            state->ambient_light.known = true;
            state->ambient_light.brightness = update.ambient.brightness;
            state->ambient_light.color = update.ambient.color;
            return result;

        case ServerUpdateKind::PlayerData:
            state->stats = update.player_data.stats;
            return result;

        case ServerUpdateKind::PlayerSkills:
            state->skills = update.player_skills.skills;
            return result;

        case ServerUpdateKind::PlayerState:
            state->state.known = true;
            state->state.flags = update.player_state.flags;
            return result;

        // Decoded so a frame can be walked, but carrying no WorldState
        // semantics this task has demonstrated. Presentation effects, the
        // inventory and the buddy list are all out of scope.
        case ServerUpdateKind::GraphicalEffect:
        case ServerUpdateKind::TextualEffect:
        case ServerUpdateKind::MissileEffect:
        case ServerUpdateKind::Inventory: {
            // SV_CMD_SET_INVENTORY and SV_CMD_DELETE_INVENTORY are the whole
            // of what the server says about what the player is wearing: there
            // is no bulk equipment command, so login arrives as one of these
            // per occupied slot from SendBodyInventory.
            const std::uint8_t slot = update.inventory.slot;
            if (slot < state->inventory.size()) {
                if (update.inventory.cleared) {
                    state->inventory[slot] = InventorySlot{};
                } else {
                    state->inventory[slot].occupied = true;
                    state->inventory[slot].item = update.inventory.item;
                }
            }
            return result;
        }

        case ServerUpdateKind::Container: {
            const std::uint8_t number = update.container.container;
            if (number >= state->containers.size()) {
                return result;
            }
            OpenContainer& container = state->containers[number];
            switch (update.container.kind) {
                case ContainerUpdateKind::Opened:
                    // Replaced wholesale rather than merged. SendContainer is
                    // the server restating the container from scratch, and it
                    // is sent on refresh as well as on open, so keeping any of
                    // the previous contents would be keeping something the
                    // server has just declined to mention.
                    container.open = true;
                    container.type_id = update.container.type_id;
                    container.name = update.container.name;
                    container.capacity = update.container.capacity;
                    container.has_parent = update.container.has_parent;
                    container.objects = update.container.items;
                    break;

                case ContainerUpdateKind::Closed:
                    container = OpenContainer{};
                    break;

                case ContainerUpdateKind::Created:
                    // At the front: SendCreateInContainer sends no index, and
                    // the server's own list is walked from its first object.
                    if (container.open) {
                        container.objects.insert(container.objects.begin(),
                                                 update.container.item);
                        if (container.objects.size() > kMaxObjectsPerContainer) {
                            container.objects.resize(kMaxObjectsPerContainer);
                        }
                    }
                    break;

                case ContainerUpdateKind::Changed:
                    if (container.open
                        && update.container.slot < container.objects.size()) {
                        container.objects[update.container.slot] = update.container.item;
                    }
                    break;

                case ContainerUpdateKind::Deleted:
                    if (container.open
                        && update.container.slot < container.objects.size()) {
                        container.objects.erase(
                            container.objects.begin() + update.container.slot);
                    }
                    break;
            }
            return result;
        }

        // The one thing Fusion32 says about a combat target. It is sent from
        // `TCombat::StopAttack(0)` and from nowhere else, which is why every
        // way a target can end -- cancelled, refused, dead, out of range,
        // gone, logged out -- arrives here as the same command.
        case ServerUpdateKind::ClearTarget:
            state->combat.target_creature_id = 0;
            state->combat.following = false;
            return result;

        case ServerUpdateKind::MarkCreature:
        case ServerUpdateKind::Buddy:
        case ServerUpdateKind::OutfitDialog:
        case ServerUpdateKind::Ping:
        case ServerUpdateKind::Message:
        // Talk is decoded, typed and surfaced, and stores nothing.
        //
        // WorldState mirrors what the server keeps about the world. Fusion32
        // keeps no chat history per connection: SendTalk serialises and
        // forgets. A client-side transcript would be a feature this client
        // does not have, and holding one in WorldState would make it look like
        // server state. Talk therefore reaches the caller as an event and ends
        // there, exactly like the effects.
        case ServerUpdateKind::Talk:
        case ServerUpdateKind::Unsupported:
            return result;
    }
    return result;
}

TalkLayout TalkLayoutForMode(std::uint8_t mode) noexcept {
    // One case per mode each SendTalk overload accepts, in the order
    // reference/game/src/sending.cc tests them. Anything absent here is absent
    // there, and must not be given a tail it never had.
    switch (static_cast<TalkMode>(mode)) {
        case TalkMode::Say:
        case TalkMode::Whisper:
        case TalkMode::Yell:
        case TalkMode::AnimalLow:
        case TalkMode::AnimalLoud:
            return TalkLayout::Positional;

        case TalkMode::ChannelCall:
        case TalkMode::GamemasterChannelCall:
        case TalkMode::HighlightChannelCall:
        case TalkMode::AnonymousChannelCall:
            return TalkLayout::Channel;

        case TalkMode::PrivateMessage:
        case TalkMode::GamemasterRequest:
        case TalkMode::GamemasterAnswer:
        case TalkMode::PlayerAnswer:
        case TalkMode::GamemasterBroadcast:
        case TalkMode::GamemasterMessage:
            return TalkLayout::Plain;
    }
    return TalkLayout::Unsupported;
}

const char* TalkModeName(std::uint8_t mode) noexcept {
    switch (static_cast<TalkMode>(mode)) {
        case TalkMode::Say: return "Say";
        case TalkMode::Whisper: return "Whisper";
        case TalkMode::Yell: return "Yell";
        case TalkMode::PrivateMessage: return "PrivateMessage";
        case TalkMode::ChannelCall: return "ChannelCall";
        case TalkMode::GamemasterRequest: return "GamemasterRequest";
        case TalkMode::GamemasterAnswer: return "GamemasterAnswer";
        case TalkMode::PlayerAnswer: return "PlayerAnswer";
        case TalkMode::GamemasterBroadcast: return "GamemasterBroadcast";
        case TalkMode::GamemasterChannelCall: return "GamemasterChannelCall";
        case TalkMode::GamemasterMessage: return "GamemasterMessage";
        case TalkMode::HighlightChannelCall: return "HighlightChannelCall";
        case TalkMode::AnonymousChannelCall: return "AnonymousChannelCall";
        case TalkMode::AnimalLow: return "AnimalLow";
        case TalkMode::AnimalLoud: return "AnimalLoud";
    }
    return "UnknownTalkMode";
}

const char* MessageModeName(std::uint8_t mode) noexcept {
    // reference/game/src/sending.cc::SendMessage whitelists exactly these.
    switch (mode) {
        case 18: return "AdminMessage";
        case 19: return "EventMessage";
        case 20: return "LoginMessage";
        case 21: return "StatusMessage";
        case 22: return "InfoMessage";
        case 23: return "FailureMessage";
        default: return "UnknownMessageMode";
    }
}

const char* ServerUpdateKindName(ServerUpdateKind kind) noexcept {
    switch (kind) {
        case ServerUpdateKind::FullScreen: return "FullScreen";
        case ServerUpdateKind::Row: return "Row";
        case ServerUpdateKind::FloorChange: return "FloorChange";
        case ServerUpdateKind::FieldData: return "FieldData";
        case ServerUpdateKind::AddField: return "AddField";
        case ServerUpdateKind::ChangeField: return "ChangeField";
        case ServerUpdateKind::DeleteField: return "DeleteField";
        case ServerUpdateKind::MoveCreature: return "MoveCreature";
        case ServerUpdateKind::Snapback: return "Snapback";
        case ServerUpdateKind::Message: return "Message";
        case ServerUpdateKind::Talk: return "Talk";
        case ServerUpdateKind::Ping: return "Ping";
        case ServerUpdateKind::Ambient: return "Ambient";
        case ServerUpdateKind::GraphicalEffect: return "GraphicalEffect";
        case ServerUpdateKind::TextualEffect: return "TextualEffect";
        case ServerUpdateKind::MissileEffect: return "MissileEffect";
        case ServerUpdateKind::MarkCreature: return "MarkCreature";
        case ServerUpdateKind::CreatureAttribute: return "CreatureAttribute";
        case ServerUpdateKind::PlayerData: return "PlayerData";
        case ServerUpdateKind::PlayerSkills: return "PlayerSkills";
        case ServerUpdateKind::PlayerState: return "PlayerState";
        case ServerUpdateKind::ClearTarget: return "ClearTarget";
        case ServerUpdateKind::Inventory: return "Inventory";
        case ServerUpdateKind::Container: return "Container";
        case ServerUpdateKind::Buddy: return "Buddy";
        case ServerUpdateKind::OutfitDialog: return "OutfitDialog";
        case ServerUpdateKind::Unsupported: return "Unsupported";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
