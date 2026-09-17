#include "fusion32/protocol772/initial_world.h"

#include <algorithm>

namespace fusion32::protocol772 {

FullScreenDecodeResult DecodeFullScreen(const std::vector<std::uint8_t>& bytes,
                                        const ObjectTypeTable& types) {
    return DecodeFullScreen(bytes, 0, types);
}

FullScreenDecodeResult DecodeFullScreen(const std::vector<std::uint8_t>& bytes,
                                        std::size_t offset,
                                        const ObjectTypeTable& types) {
    FullScreenDecodeResult result;

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
    if (bytes[offset] != kServerCommandFullScreen) {
        result.error = MapDecodeError::NotFullScreen;
        result.error_offset = offset;
        result.detail = "payload does not start with SV_CMD_FULLSCREEN";
        return result;
    }

    MapScanner scanner(bytes, offset + 1, types);
    std::uint16_t player_x = 0;
    std::uint16_t player_y = 0;
    std::uint8_t player_z = 0;
    if (!ReadViewportHeader(&scanner, &player_x, &player_y, &player_z)) {
        result.error = scanner.error();
        result.error_offset = scanner.error_offset();
        result.detail = scanner.detail();
        return result;
    }

    FullScreenMessage& message = result.message;
    message.window.player_position = {static_cast<std::int32_t>(player_x),
                                      static_cast<std::int32_t>(player_y),
                                      static_cast<std::int32_t>(player_z)};
    message.window.min_x = static_cast<std::int32_t>(player_x) - kTerminalOffsetX;
    message.window.min_y = static_cast<std::int32_t>(player_y) - kTerminalOffsetY;
    message.window.width = kTerminalWidth;
    message.window.height = kTerminalHeight;

    const std::int32_t position_z = static_cast<std::int32_t>(player_z);
    const FloorRange range = ViewportFloorRange(position_z);
    message.first_floor = range.first;
    message.last_floor = range.last;
    message.floor_step = range.step;

    const std::int32_t end_z = range.last + range.step;
    for (std::int32_t floor_z = range.first; floor_z != end_z; floor_z += range.step) {
        MapFloor floor;
        const std::int32_t floor_offset = position_z - floor_z;
        if (!scanner.ScanRect(floor_z, floor_offset,
                              message.window.min_x + floor_offset,
                              message.window.min_y + floor_offset,
                              kTerminalWidth, kTerminalHeight, &floor)) {
            result.error = scanner.error();
            result.error_offset = scanner.error_offset();
            result.detail = scanner.detail();
            return result;
        }
        message.floors.push_back(std::move(floor));
    }

    if (!scanner.FinishRun()) {
        result.error = scanner.error();
        result.error_offset = scanner.error_offset();
        result.detail = scanner.detail();
        return result;
    }

    message.described_tiles = scanner.described_tiles();
    message.skipped_tiles = scanner.skipped_tiles();
    message.thing_count = scanner.thing_count();
    message.creature_count = scanner.creature_count();
    message.bytes_consumed = scanner.at() - offset;
    result.remaining_bytes.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(scanner.at()), bytes.end());
    return result;
}

WorldStateApplyResult ApplyFullScreen(WorldState* state, const FullScreenMessage& message) {
    WorldStateApplyResult result;
    if (state == nullptr) return result;

    state->window = message.window;
    state->floors = message.floors;
    state->map_initialized = true;
    state->viewport_anchor = message.window.player_position;

    for (const MapFloor& floor : state->floors) {
        for (const MapTile& tile : floor.tiles) {
            for (std::size_t stack = 0; stack < tile.things.size(); ++stack) {
                const MapThing& thing = tile.things[stack];
                if (thing.kind != MapThingKind::Creature) continue;
                const CreatureThing& creature = thing.creature;

                if (creature.evicts_slot && creature.removed_creature_id != 0) {
                    if (state->known_creatures.erase(creature.removed_creature_id) == 0) {
                        result.anomalies.push_back(
                            {WorldStateAnomalyKind::UnknownEvictedCreature,
                             creature.removed_creature_id, tile.position});
                    }
                }

                const auto existing = state->known_creatures.find(creature.creature_id);
                if (!creature.has_descriptor && existing == state->known_creatures.end()) {
                    result.anomalies.push_back(
                        {WorldStateAnomalyKind::UnknownCreatureReference,
                         creature.creature_id, tile.position});
                }
                if (creature.kind == CreatureDescriptorKind::Outdated
                    && existing == state->known_creatures.end()) {
                    result.anomalies.push_back(
                        {WorldStateAnomalyKind::UnknownCreatureReference,
                         creature.creature_id, tile.position});
                }

                CreatureRecord& record = state->known_creatures[creature.creature_id];
                record.creature_id = creature.creature_id;
                record.position = tile.position;
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
        }
    }

    if (state->known_creatures.size() > kKnownCreatureTableSize) {
        result.anomalies.push_back(
            {WorldStateAnomalyKind::KnownCreatureTableOverflow, 0, MapPosition{}});
    }
    return result;
}

InitialWorldResult DecodeInitialWorld(const std::vector<std::uint8_t>& preserved_bytes,
                                      const ObjectTypeTable& types) {
    InitialWorldResult result;
    if (preserved_bytes.empty() || preserved_bytes[0] != kServerCommandFullScreen) {
        result.status = InitialWorldStatus::NoFullScreen;
        result.fullscreen.error = MapDecodeError::NotFullScreen;
        result.fullscreen.detail = "preserved tail does not start with SV_CMD_FULLSCREEN";
        result.fullscreen.remaining_bytes = preserved_bytes;
        if (!preserved_bytes.empty()) {
            result.has_trailing_command = true;
            result.trailing_command = {preserved_bytes[0],
                                       ServerCommandName(preserved_bytes[0]), 0};
        }
        return result;
    }

    result.fullscreen = DecodeFullScreen(preserved_bytes, 0, types);
    result.status = result.fullscreen.ok() ? InitialWorldStatus::FullScreenDecoded
                                           : InitialWorldStatus::DecodeFailed;
    if (result.fullscreen.ok() && !result.fullscreen.remaining_bytes.empty()) {
        const std::uint8_t opcode = result.fullscreen.remaining_bytes[0];
        result.has_trailing_command = true;
        result.trailing_command = {opcode, ServerCommandName(opcode),
                                   result.fullscreen.message.bytes_consumed};
    }
    return result;
}

const char* InitialWorldStatusName(InitialWorldStatus status) noexcept {
    switch (status) {
        case InitialWorldStatus::FullScreenDecoded: return "FullScreenDecoded";
        case InitialWorldStatus::NoFullScreen: return "NoFullScreen";
        case InitialWorldStatus::DecodeFailed: return "DecodeFailed";
    }
    return "Unknown";
}

const char* ServerCommandName(std::uint8_t opcode) noexcept {
    switch (opcode) {
        case 10: return "SV_CMD_INIT_GAME";
        case 11: return "SV_CMD_RIGHTS";
        case 20: return "SV_CMD_LOGIN_ERROR";
        case 21: return "SV_CMD_LOGIN_PREMIUM";
        case 22: return "SV_CMD_LOGIN_WAITINGLIST";
        case 30: return "SV_CMD_PING";
        case 100: return "SV_CMD_FULLSCREEN";
        case 101: return "SV_CMD_ROW_NORTH";
        case 102: return "SV_CMD_ROW_EAST";
        case 103: return "SV_CMD_ROW_SOUTH";
        case 104: return "SV_CMD_ROW_WEST";
        case 105: return "SV_CMD_FIELD_DATA";
        case 106: return "SV_CMD_ADD_FIELD";
        case 107: return "SV_CMD_CHANGE_FIELD";
        case 108: return "SV_CMD_DELETE_FIELD";
        case 109: return "SV_CMD_MOVE_CREATURE";
        case 110: return "SV_CMD_CONTAINER";
        case 111: return "SV_CMD_CLOSE_CONTAINER";
        case 112: return "SV_CMD_CREATE_IN_CONTAINER";
        case 113: return "SV_CMD_CHANGE_IN_CONTAINER";
        case 114: return "SV_CMD_DELETE_IN_CONTAINER";
        case 120: return "SV_CMD_SET_INVENTORY";
        case 121: return "SV_CMD_DELETE_INVENTORY";
        case 125: return "SV_CMD_TRADE_OFFER_OWN";
        case 126: return "SV_CMD_TRADE_OFFER_PARTNER";
        case 127: return "SV_CMD_CLOSE_TRADE";
        case 130: return "SV_CMD_AMBIENTE";
        case 131: return "SV_CMD_GRAPHICAL_EFFECT";
        case 132: return "SV_CMD_TEXTUAL_EFFECT";
        case 133: return "SV_CMD_MISSILE_EFFECT";
        case 134: return "SV_CMD_MARK_CREATURE";
        case 140: return "SV_CMD_CREATURE_HEALTH";
        case 141: return "SV_CMD_CREATURE_LIGHT";
        case 142: return "SV_CMD_CREATURE_OUTFIT";
        case 143: return "SV_CMD_CREATURE_SPEED";
        case 144: return "SV_CMD_CREATURE_SKULL";
        case 145: return "SV_CMD_CREATURE_PARTY";
        case 150: return "SV_CMD_EDIT_TEXT";
        case 151: return "SV_CMD_EDIT_LIST";
        case 160: return "SV_CMD_PLAYER_DATA";
        case 161: return "SV_CMD_PLAYER_SKILLS";
        case 162: return "SV_CMD_PLAYER_STATE";
        case 163: return "SV_CMD_CLEAR_TARGET";
        case 170: return "SV_CMD_TALK";
        case 171: return "SV_CMD_CHANNELS";
        case 172: return "SV_CMD_OPEN_CHANNEL";
        case 173: return "SV_CMD_PRIVATE_CHANNEL";
        case 174: return "SV_CMD_OPEN_REQUEST_QUEUE";
        case 175: return "SV_CMD_DELETE_REQUEST";
        case 176: return "SV_CMD_FINISH_REQUEST";
        case 177: return "SV_CMD_CLOSE_REQUEST";
        case 178: return "SV_CMD_OPEN_OWN_CHANNEL";
        case 179: return "SV_CMD_CLOSE_CHANNEL";
        case 180: return "SV_CMD_MESSAGE";
        case 181: return "SV_CMD_SNAPBACK";
        case 190: return "SV_CMD_FLOOR_UP";
        case 191: return "SV_CMD_FLOOR_DOWN";
        case 200: return "SV_CMD_OUTFIT";
        case 210: return "SV_CMD_BUDDY_DATA";
        case 211: return "SV_CMD_BUDDY_ONLINE";
        case 212: return "SV_CMD_BUDDY_OFFLINE";
        default: return "Unknown";
    }
}

}  // namespace fusion32::protocol772
