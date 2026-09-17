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
    if (creature.evicts_slot && creature.removed_creature_id != 0) {
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
            const MapThing removed = tile->things[update.delete_field.stack_index];
            tile->things.erase(tile->things.begin()
                               + static_cast<std::ptrdiff_t>(update.delete_field.stack_index));
            const MapTile snapshot = *tile;
            RefreshTileCreatures(state, snapshot);
            if (removed.kind == MapThingKind::Creature) {
                state->known_creatures.erase(removed.creature.creature_id);
            }
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

        case ServerUpdateKind::Message:
        case ServerUpdateKind::Unsupported:
            return result;
    }
    return result;
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
        case ServerUpdateKind::Unsupported: return "Unsupported";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
