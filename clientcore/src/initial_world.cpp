#include "fusion32/protocol772/initial_world.h"

#include <algorithm>

namespace fusion32::protocol772 {
namespace {

// Bounded forward cursor. Every read is checked; nothing is consumed on
// failure, so `at` always marks the first byte the decoder could not satisfy.
class Cursor {
public:
    Cursor(const std::vector<std::uint8_t>& bytes, std::size_t at)
        : bytes_(bytes), at_(at) {}

    std::size_t at() const noexcept { return at_; }
    std::size_t remaining() const noexcept { return bytes_.size() - at_; }

    bool ReadU8(std::uint8_t* output) noexcept {
        if (remaining() < 1) return false;
        *output = bytes_[at_];
        at_ += 1;
        return true;
    }

    bool ReadU16(std::uint16_t* output) noexcept {
        if (remaining() < 2) return false;
        *output = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(bytes_[at_])
            | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[at_ + 1]) << 8U));
        at_ += 2;
        return true;
    }

    bool ReadU32(std::uint32_t* output) noexcept {
        if (remaining() < 4) return false;
        *output = static_cast<std::uint32_t>(bytes_[at_])
                | (static_cast<std::uint32_t>(bytes_[at_ + 1]) << 8U)
                | (static_cast<std::uint32_t>(bytes_[at_ + 2]) << 16U)
                | (static_cast<std::uint32_t>(bytes_[at_ + 3]) << 24U);
        at_ += 4;
        return true;
    }

    bool PeekU16(std::uint16_t* output) const noexcept {
        if (remaining() < 2) return false;
        *output = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(bytes_[at_])
            | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[at_ + 1]) << 8U));
        return true;
    }

    bool ReadBytes(std::size_t count, std::string* output) {
        if (remaining() < count) return false;
        output->assign(reinterpret_cast<const char*>(bytes_.data() + at_), count);
        at_ += count;
        return true;
    }

    bool ReadBytes(std::size_t count, std::uint8_t* output) noexcept {
        if (remaining() < count) return false;
        std::copy(bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
                  bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + count),
                  output);
        at_ += count;
        return true;
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t at_;
};

struct DecodeContext {
    const ObjectTypeTable* types = nullptr;
    MapDecodeError error = MapDecodeError::None;
    std::size_t error_offset = 0;
    std::string detail;

    bool Fail(MapDecodeError code, std::size_t offset, const char* text) {
        error = code;
        error_offset = offset;
        detail = text;
        return false;
    }

    bool ok() const noexcept { return error == MapDecodeError::None; }
};

// Source: reference/game/src/sending.cc::SendString, a little-endian word
// length followed by the raw bytes. SendString casts strlen() to uint16 and
// never emits the 0xFFFF escape that the RSA login block uses.
bool ReadCreatureName(Cursor* cursor, DecodeContext* context, std::string* output) {
    const std::size_t start = cursor->at();
    std::uint16_t length = 0;
    if (!cursor->ReadU16(&length)) {
        return context->Fail(MapDecodeError::Truncated, start, "creature name length");
    }
    if (length > kCreatureNameLimit) {
        return context->Fail(MapDecodeError::CreatureNameTooLong, start,
                             "creature name exceeds TCreature::Name capacity");
    }
    if (!cursor->ReadBytes(length, output)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "creature name bytes");
    }
    return true;
}

// Source: reference/game/src/sending.cc::SendOutfit. TOutfit's union makes the
// two branches mutually exclusive, so exactly one of them is on the wire.
bool ReadOutfit(Cursor* cursor, DecodeContext* context, OutfitDescriptor* output) {
    const std::size_t start = cursor->at();
    if (!cursor->ReadU16(&output->outfit_id)) {
        return context->Fail(MapDecodeError::Truncated, start, "outfit id");
    }
    if (output->outfit_id == 0) {
        output->disguised_as_object = true;
        if (!cursor->ReadU16(&output->object_type)) {
            return context->Fail(MapDecodeError::Truncated, cursor->at(), "outfit object type");
        }
        return true;
    }
    if (!cursor->ReadBytes(output->colors.size(), output->colors.data())) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "outfit colors");
    }
    return true;
}

// The trailing block shared by the word-97 and word-98 creature descriptors.
// Source: reference/game/src/sending.cc::SendMapObject lines 260-267.
bool ReadCreatureDescriptor(Cursor* cursor, DecodeContext* context, CreatureThing* output) {
    if (!cursor->ReadU8(&output->health_percent)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "creature health");
    }
    if (!cursor->ReadU8(&output->direction)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "creature direction");
    }
    if (!ReadOutfit(cursor, context, &output->outfit)) return false;
    if (!cursor->ReadU8(&output->light_brightness)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "creature light brightness");
    }
    if (!cursor->ReadU8(&output->light_color)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "creature light color");
    }
    if (!cursor->ReadU16(&output->speed)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "creature speed");
    }
    if (!cursor->ReadU8(&output->playerkilling_mark)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "playerkilling mark");
    }
    if (!cursor->ReadU8(&output->party_mark)) {
        return context->Fail(MapDecodeError::Truncated, cursor->at(), "party mark");
    }
    return true;
}

// One entry of a tile's object list. Source:
// reference/game/src/sending.cc::SendMapObject, which either emits a creature
// descriptor or delegates to SendItem.
bool ReadMapThing(Cursor* cursor, DecodeContext* context, MapThing* output) {
    const std::size_t start = cursor->at();
    std::uint16_t word = 0;
    if (!cursor->ReadU16(&word)) {
        return context->Fail(MapDecodeError::Truncated, start, "object type word");
    }

    if (word == kCreatureMarkerKnown || word == kCreatureMarkerOutdated
        || word == kCreatureMarkerNew) {
        output->kind = MapThingKind::Creature;
        CreatureThing& creature = output->creature;
        if (word == kCreatureMarkerKnown) {
            creature.kind = CreatureDescriptorKind::Known;
            if (!cursor->ReadU32(&creature.creature_id)) {
                return context->Fail(MapDecodeError::Truncated, cursor->at(), "known creature id");
            }
            if (!cursor->ReadU8(&creature.direction)) {
                return context->Fail(MapDecodeError::Truncated, cursor->at(),
                                     "known creature direction");
            }
            return true;
        }
        if (word == kCreatureMarkerNew) {
            creature.kind = CreatureDescriptorKind::Introduced;
            creature.evicts_slot = true;
            if (!cursor->ReadU32(&creature.removed_creature_id)) {
                return context->Fail(MapDecodeError::Truncated, cursor->at(),
                                     "evicted creature id");
            }
            if (!cursor->ReadU32(&creature.creature_id)) {
                return context->Fail(MapDecodeError::Truncated, cursor->at(), "new creature id");
            }
            if (!ReadCreatureName(cursor, context, &creature.name)) return false;
            creature.has_name = true;
        } else {
            creature.kind = CreatureDescriptorKind::Outdated;
            if (!cursor->ReadU32(&creature.creature_id)) {
                return context->Fail(MapDecodeError::Truncated, cursor->at(),
                                     "outdated creature id");
            }
        }
        if (!ReadCreatureDescriptor(cursor, context, &creature)) return false;
        creature.has_descriptor = true;
        return true;
    }

    if (word <= kTypeIdLastBodyContainer) {
        // TYPEID_MAP_CONTAINER and the body containers are server-internal
        // holders (reference/game/src/objects.hh). GetFirstObject yields the
        // contents of the map container, never the container types themselves,
        // so decoding them as items would invent semantics.
        return context->Fail(MapDecodeError::ReservedObjectTypeId, start,
                             "server-internal container type id on the map");
    }

    output->kind = MapThingKind::Item;
    output->item.type_id = word;
    const ObjectTypeEncoding encoding = context->types->Lookup(word);
    if (!encoding.known) {
        return context->Fail(MapDecodeError::UnknownObjectTypeId, start,
                             "object type id absent from the type table");
    }
    if (encoding.liquid_color) {
        output->item.has_liquid_color = true;
        if (!cursor->ReadU8(&output->item.liquid_color)) {
            return context->Fail(MapDecodeError::Truncated, cursor->at(), "liquid color");
        }
    }
    if (encoding.cumulative) {
        output->item.has_amount = true;
        if (!cursor->ReadU8(&output->item.amount)) {
            return context->Fail(MapDecodeError::Truncated, cursor->at(), "cumulative amount");
        }
    }
    return true;
}

// Reads the objects of one described tile and returns the skip count carried by
// the marker that terminates it. Source:
// reference/game/src/sending.cc::SendMapPoint, bounded by MAX_OBJECTS_PER_POINT.
bool ReadDescribedTile(Cursor* cursor, DecodeContext* context, MapTile* tile,
                       std::uint32_t* skip_after) {
    while (true) {
        std::uint16_t word = 0;
        if (!cursor->PeekU16(&word)) {
            return context->Fail(MapDecodeError::Truncated, cursor->at(),
                                 "tile object or skip marker");
        }
        if (word >= kSkipMarkerBase) {
            std::uint16_t marker = 0;
            static_cast<void>(cursor->ReadU16(&marker));
            *skip_after = static_cast<std::uint32_t>(marker & 0x00FFU);
            return true;
        }
        if (tile->things.size() >= kMapObjectsPerPointLimit) {
            return context->Fail(MapDecodeError::TooManyObjectsInTile, cursor->at(),
                                 "tile exceeds MAX_OBJECTS_PER_POINT without a skip marker");
        }
        MapThing thing;
        if (!ReadMapThing(cursor, context, &thing)) return false;
        tile->things.push_back(std::move(thing));
    }
}

}  // namespace

FullScreenDecodeResult DecodeFullScreen(const std::vector<std::uint8_t>& bytes,
                                        const ObjectTypeTable& types) {
    return DecodeFullScreen(bytes, 0, types);
}

FullScreenDecodeResult DecodeFullScreen(const std::vector<std::uint8_t>& bytes,
                                        std::size_t offset,
                                        const ObjectTypeTable& types) {
    FullScreenDecodeResult result;
    DecodeContext context;
    context.types = &types;

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

    Cursor cursor(bytes, offset + 1);
    std::uint16_t player_x = 0;
    std::uint16_t player_y = 0;
    std::uint8_t player_z = 0;
    if (!cursor.ReadU16(&player_x) || !cursor.ReadU16(&player_y)
        || !cursor.ReadU8(&player_z)) {
        result.error = MapDecodeError::Truncated;
        result.error_offset = cursor.at();
        result.detail = "fullscreen header";
        return result;
    }
    if (player_z > kMaxFloor) {
        result.error = MapDecodeError::InvalidPlayerFloor;
        result.error_offset = offset + 5;
        result.detail = "player floor above the addressable maximum";
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
    std::int32_t start_z = 0;
    std::int32_t end_z = 0;
    std::int32_t step_z = 0;
    if (position_z <= kSurfaceFloor) {
        step_z = -1;
        start_z = kSurfaceFloor;
        end_z = 0 + step_z;
    } else {
        step_z = 1;
        start_z = position_z - 2;
        end_z = std::min<std::int32_t>(position_z + 2, kMaxFloor) + step_z;
    }
    message.first_floor = start_z;
    message.last_floor = end_z - step_z;
    message.floor_step = step_z;

    // `Skip` is a single file-scope counter in the server, reset once before
    // the floor loop, so a run of empty tiles can cross a floor boundary.
    std::uint32_t skip = 0;
    for (std::int32_t floor_z = start_z; floor_z != end_z; floor_z += step_z) {
        MapFloor floor;
        floor.z = floor_z;
        floor.offset = position_z - floor_z;
        for (std::int32_t ix = 0; ix < kTerminalWidth; ++ix) {
            for (std::int32_t iy = 0; iy < kTerminalHeight; ++iy) {
                if (skip > 0) {
                    skip -= 1;
                    message.skipped_tiles += 1;
                    continue;
                }
                std::uint16_t word = 0;
                if (!cursor.PeekU16(&word)) {
                    context.Fail(MapDecodeError::Truncated, cursor.at(),
                                 "tile object or skip marker");
                    result.error = context.error;
                    result.error_offset = context.error_offset;
                    result.detail = context.detail;
                    return result;
                }
                if (word >= kSkipMarkerBase) {
                    std::uint16_t marker = 0;
                    static_cast<void>(cursor.ReadU16(&marker));
                    skip = static_cast<std::uint32_t>(marker & 0x00FFU);
                    message.skipped_tiles += 1;
                    continue;
                }

                MapTile tile;
                tile.position = {message.window.min_x + ix + floor.offset,
                                 message.window.min_y + iy + floor.offset,
                                 floor_z};
                std::uint32_t skip_after = 0;
                if (!ReadDescribedTile(&cursor, &context, &tile, &skip_after)) {
                    result.error = context.error;
                    result.error_offset = context.error_offset;
                    result.detail = context.detail;
                    return result;
                }
                message.thing_count += tile.things.size();
                for (const MapThing& thing : tile.things) {
                    if (thing.kind == MapThingKind::Creature) message.creature_count += 1;
                }
                floor.tiles.push_back(std::move(tile));
                message.described_tiles += 1;
                skip = skip_after;
            }
        }
        message.floors.push_back(std::move(floor));
    }

    if (skip != 0) {
        result.error = MapDecodeError::SkipRunExceedsWindow;
        result.error_offset = cursor.at();
        result.detail = "skip run outlives the fullscreen window";
        return result;
    }

    message.bytes_consumed = cursor.at() - offset;
    result.remaining_bytes.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(cursor.at()), bytes.end());
    return result;
}

WorldStateApplyResult ApplyFullScreen(WorldState* state, const FullScreenMessage& message) {
    WorldStateApplyResult result;
    if (state == nullptr) return result;

    state->window = message.window;
    state->floors = message.floors;
    state->map_initialized = true;

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

const char* MapDecodeErrorName(MapDecodeError error) noexcept {
    switch (error) {
        case MapDecodeError::None: return "None";
        case MapDecodeError::NotFullScreen: return "NotFullScreen";
        case MapDecodeError::Truncated: return "Truncated";
        case MapDecodeError::InvalidPlayerFloor: return "InvalidPlayerFloor";
        case MapDecodeError::ReservedObjectTypeId: return "ReservedObjectTypeId";
        case MapDecodeError::UnknownObjectTypeId: return "UnknownObjectTypeId";
        case MapDecodeError::TooManyObjectsInTile: return "TooManyObjectsInTile";
        case MapDecodeError::CreatureNameTooLong: return "CreatureNameTooLong";
        case MapDecodeError::SkipRunExceedsWindow: return "SkipRunExceedsWindow";
        case MapDecodeError::EmptyObjectTypeTable: return "EmptyObjectTypeTable";
    }
    return "Unknown";
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
