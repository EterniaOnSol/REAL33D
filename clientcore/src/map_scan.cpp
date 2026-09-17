#include "fusion32/protocol772/map_scan.h"

#include <algorithm>

namespace fusion32::protocol772 {

FloorRange ViewportFloorRange(std::int32_t player_z) noexcept {
    FloorRange range;
    if (player_z <= kSurfaceFloor) {
        range.step = -1;
        range.first = kSurfaceFloor;
        range.last = 0;
    } else {
        range.step = 1;
        range.first = player_z - 2;
        range.last = std::min<std::int32_t>(player_z + 2, kMaxFloor);
    }
    range.count = static_cast<std::size_t>(
        ((range.last - range.first) * range.step) + 1);
    return range;
}

MapScanner::MapScanner(const std::vector<std::uint8_t>& bytes, std::size_t at,
                       const ObjectTypeTable& types) noexcept
    : bytes_(bytes), at_(at), types_(types) {}

bool MapScanner::Fail(MapDecodeError code, std::size_t offset, const char* text) {
    if (error_ == MapDecodeError::None) {
        error_ = code;
        error_offset_ = offset;
        detail_ = text;
    }
    return false;
}

bool MapScanner::ReadU8(std::uint8_t* output) noexcept {
    if (bytes_.size() - at_ < 1) return false;
    *output = bytes_[at_];
    at_ += 1;
    return true;
}

bool MapScanner::ReadU16(std::uint16_t* output) noexcept {
    if (bytes_.size() - at_ < 2) return false;
    *output = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(bytes_[at_])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[at_ + 1]) << 8U));
    at_ += 2;
    return true;
}

bool MapScanner::ReadU32(std::uint32_t* output) noexcept {
    if (bytes_.size() - at_ < 4) return false;
    *output = static_cast<std::uint32_t>(bytes_[at_])
            | (static_cast<std::uint32_t>(bytes_[at_ + 1]) << 8U)
            | (static_cast<std::uint32_t>(bytes_[at_ + 2]) << 16U)
            | (static_cast<std::uint32_t>(bytes_[at_ + 3]) << 24U);
    at_ += 4;
    return true;
}

bool MapScanner::PeekU16(std::uint16_t* output) const noexcept {
    if (bytes_.size() - at_ < 2) return false;
    *output = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(bytes_[at_])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[at_ + 1]) << 8U));
    return true;
}

bool MapScanner::ReadRaw(std::size_t count, std::string* output) {
    if (bytes_.size() - at_ < count) return false;
    output->assign(reinterpret_cast<const char*>(bytes_.data() + at_), count);
    at_ += count;
    return true;
}

bool MapScanner::ReadRaw(std::size_t count, std::uint8_t* output) noexcept {
    if (bytes_.size() - at_ < count) return false;
    std::copy(bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
              bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + count),
              output);
    at_ += count;
    return true;
}

// Source: reference/game/src/sending.cc::SendString, a little-endian word
// length followed by the raw bytes. SendString casts strlen() to uint16 and
// never emits the 0xFFFF escape that the RSA login block uses.
bool MapScanner::ReadCreatureName(std::string* output) {
    const std::size_t start = at_;
    std::uint16_t length = 0;
    if (!ReadU16(&length)) {
        return Fail(MapDecodeError::Truncated, start, "creature name length");
    }
    if (length > kCreatureNameLimit) {
        return Fail(MapDecodeError::CreatureNameTooLong, start,
                    "creature name exceeds TCreature::Name capacity");
    }
    if (!ReadRaw(length, output)) {
        return Fail(MapDecodeError::Truncated, at_, "creature name bytes");
    }
    return true;
}

// Source: reference/game/src/sending.cc::SendOutfit and cr.hh::TOutfit, whose
// union makes the two branches mutually exclusive.
bool MapScanner::ReadOutfit(OutfitDescriptor* output) {
    const std::size_t start = at_;
    if (!ReadU16(&output->outfit_id)) {
        return Fail(MapDecodeError::Truncated, start, "outfit id");
    }
    if (output->outfit_id == 0) {
        output->disguised_as_object = true;
        if (!ReadU16(&output->object_type)) {
            return Fail(MapDecodeError::Truncated, at_, "outfit object type");
        }
        return true;
    }
    if (!ReadRaw(output->colors.size(), output->colors.data())) {
        return Fail(MapDecodeError::Truncated, at_, "outfit colors");
    }
    return true;
}

// The trailing block shared by the word-97 and word-98 creature descriptors.
// Source: reference/game/src/sending.cc::SendMapObject lines 260-267.
bool MapScanner::ReadCreatureDescriptor(CreatureThing* output) {
    if (!ReadU8(&output->health_percent)) {
        return Fail(MapDecodeError::Truncated, at_, "creature health");
    }
    if (!ReadU8(&output->direction)) {
        return Fail(MapDecodeError::Truncated, at_, "creature direction");
    }
    if (!ReadOutfit(&output->outfit)) return false;
    if (!ReadU8(&output->light_brightness)) {
        return Fail(MapDecodeError::Truncated, at_, "creature light brightness");
    }
    if (!ReadU8(&output->light_color)) {
        return Fail(MapDecodeError::Truncated, at_, "creature light color");
    }
    if (!ReadU16(&output->speed)) {
        return Fail(MapDecodeError::Truncated, at_, "creature speed");
    }
    if (!ReadU8(&output->playerkilling_mark)) {
        return Fail(MapDecodeError::Truncated, at_, "playerkilling mark");
    }
    if (!ReadU8(&output->party_mark)) {
        return Fail(MapDecodeError::Truncated, at_, "party mark");
    }
    return true;
}

// One entry of a tile's object list. Source:
// reference/game/src/sending.cc::SendMapObject, which either emits a creature
// descriptor or delegates to SendItem.
bool MapScanner::ReadMapThing(MapThing* output) {
    const std::size_t start = at_;
    std::uint16_t word = 0;
    if (!ReadU16(&word)) {
        return Fail(MapDecodeError::Truncated, start, "object type word");
    }

    if (word == kCreatureMarkerKnown || word == kCreatureMarkerOutdated
        || word == kCreatureMarkerNew) {
        output->kind = MapThingKind::Creature;
        CreatureThing& creature = output->creature;
        if (word == kCreatureMarkerKnown) {
            creature.kind = CreatureDescriptorKind::Known;
            if (!ReadU32(&creature.creature_id)) {
                return Fail(MapDecodeError::Truncated, at_, "known creature id");
            }
            if (!ReadU8(&creature.direction)) {
                return Fail(MapDecodeError::Truncated, at_, "known creature direction");
            }
            return true;
        }
        if (word == kCreatureMarkerNew) {
            creature.kind = CreatureDescriptorKind::Introduced;
            creature.evicts_slot = true;
            if (!ReadU32(&creature.removed_creature_id)) {
                return Fail(MapDecodeError::Truncated, at_, "evicted creature id");
            }
            if (!ReadU32(&creature.creature_id)) {
                return Fail(MapDecodeError::Truncated, at_, "new creature id");
            }
            if (!ReadCreatureName(&creature.name)) return false;
            creature.has_name = true;
        } else {
            creature.kind = CreatureDescriptorKind::Outdated;
            if (!ReadU32(&creature.creature_id)) {
                return Fail(MapDecodeError::Truncated, at_, "outdated creature id");
            }
        }
        if (!ReadCreatureDescriptor(&creature)) return false;
        creature.has_descriptor = true;
        return true;
    }

    if (word <= kTypeIdLastBodyContainer) {
        // TYPEID_MAP_CONTAINER and the body containers are server-internal
        // holders (reference/game/src/objects.hh). GetFirstObject yields the
        // contents of the map container, never the container types themselves,
        // so decoding them as items would invent semantics.
        return Fail(MapDecodeError::ReservedObjectTypeId, start,
                    "server-internal container type id on the map");
    }

    output->kind = MapThingKind::Item;
    output->item.type_id = word;
    const ObjectTypeEncoding encoding = types_.Lookup(word);
    if (!encoding.known) {
        return Fail(MapDecodeError::UnknownObjectTypeId, start,
                    "object type id absent from the type table");
    }
    if (encoding.liquid_color) {
        output->item.has_liquid_color = true;
        if (!ReadU8(&output->item.liquid_color)) {
            return Fail(MapDecodeError::Truncated, at_, "liquid color");
        }
    }
    if (encoding.cumulative) {
        output->item.has_amount = true;
        if (!ReadU8(&output->item.amount)) {
            return Fail(MapDecodeError::Truncated, at_, "cumulative amount");
        }
    }
    return true;
}

bool ReadStandaloneMapThing(MapScanner* scanner, MapThing* output) {
    return scanner->ReadMapThing(output);
}

bool ReadViewportHeader(MapScanner* scanner, std::uint16_t* x, std::uint16_t* y,
                        std::uint8_t* z) {
    const std::size_t start = scanner->at_;
    if (!scanner->ReadU16(x) || !scanner->ReadU16(y) || !scanner->ReadU8(z)) {
        return scanner->Fail(MapDecodeError::Truncated, scanner->at_, "viewport header");
    }
    if (*z > kMaxFloor) {
        return scanner->Fail(MapDecodeError::InvalidPlayerFloor, start + 4,
                             "floor above the addressable maximum");
    }
    return true;
}

bool ReadFieldPosition(MapScanner* scanner, MapPosition* position) {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint8_t z = 0;
    if (!ReadViewportHeader(scanner, &x, &y, &z)) return false;
    position->x = static_cast<std::int32_t>(x);
    position->y = static_cast<std::int32_t>(y);
    position->z = static_cast<std::int32_t>(z);
    return true;
}

bool ReadScannerByte(MapScanner* scanner, std::uint8_t* value, const char* what) {
    if (!scanner->ReadU8(value)) {
        return scanner->Fail(MapDecodeError::Truncated, scanner->at_, what);
    }
    return true;
}

bool FailScanner(MapScanner* scanner, MapDecodeError code, const char* detail) {
    return scanner->Fail(code, scanner->at_, detail);
}

bool ReadScannerWord(MapScanner* scanner, std::uint16_t* value, const char* what) {
    if (!scanner->ReadU16(value)) {
        return scanner->Fail(MapDecodeError::Truncated, scanner->at_, what);
    }
    return true;
}

bool ReadScannerQuad(MapScanner* scanner, std::uint32_t* value, const char* what) {
    if (!scanner->ReadU32(value)) {
        return scanner->Fail(MapDecodeError::Truncated, scanner->at_, what);
    }
    return true;
}

bool ReadScannerOutfit(MapScanner* scanner, OutfitDescriptor* value) {
    return scanner->ReadOutfit(value);
}

bool ReadScannerItem(MapScanner* scanner, ItemThing* value) {
    const std::size_t start = scanner->at_;
    std::uint16_t type_id = 0;
    if (!scanner->ReadU16(&type_id)) {
        return scanner->Fail(MapDecodeError::Truncated, start, "item type id");
    }
    if (type_id <= kTypeIdLastBodyContainer || type_id == kTypeIdCreatureContainer) {
        return scanner->Fail(MapDecodeError::ReservedObjectTypeId, start,
                             "server-internal container type id carried as an item");
    }
    value->type_id = type_id;
    const ObjectTypeEncoding encoding = scanner->types_.Lookup(type_id);
    if (!encoding.known) {
        return scanner->Fail(MapDecodeError::UnknownObjectTypeId, start,
                             "object type id absent from the type table");
    }
    if (encoding.liquid_color) {
        value->has_liquid_color = true;
        if (!scanner->ReadU8(&value->liquid_color)) {
            return scanner->Fail(MapDecodeError::Truncated, scanner->at_, "liquid color");
        }
    }
    if (encoding.cumulative) {
        value->has_amount = true;
        if (!scanner->ReadU8(&value->amount)) {
            return scanner->Fail(MapDecodeError::Truncated, scanner->at_, "cumulative amount");
        }
    }
    return true;
}

bool ReadScannerString(MapScanner* scanner, std::string* value, const char* what) {
    const std::size_t start = scanner->at_;
    std::uint16_t length = 0;
    if (!scanner->ReadU16(&length)) {
        return scanner->Fail(MapDecodeError::Truncated, start, what);
    }
    if (!scanner->ReadRaw(length, value)) {
        return scanner->Fail(MapDecodeError::Truncated, scanner->at_, what);
    }
    return true;
}

// Reads the objects of one described tile and returns the skip count carried by
// the marker that terminates it. Source:
// reference/game/src/sending.cc::SendMapPoint, bounded by MAX_OBJECTS_PER_POINT.
bool MapScanner::ReadDescribedTile(MapTile* tile, std::uint32_t* skip_after) {
    while (true) {
        std::uint16_t word = 0;
        if (!PeekU16(&word)) {
            return Fail(MapDecodeError::Truncated, at_, "tile object or skip marker");
        }
        if (word >= kSkipMarkerBase) {
            std::uint16_t marker = 0;
            static_cast<void>(ReadU16(&marker));
            *skip_after = static_cast<std::uint32_t>(marker & 0x00FFU);
            return true;
        }
        if (tile->things.size() >= kMapObjectsPerPointLimit) {
            return Fail(MapDecodeError::TooManyObjectsInTile, at_,
                        "tile exceeds MAX_OBJECTS_PER_POINT without a skip marker");
        }
        MapThing thing;
        if (!ReadMapThing(&thing)) return false;
        tile->things.push_back(std::move(thing));
    }
}

bool MapScanner::ScanRect(std::int32_t z, std::int32_t offset,
                          std::int32_t origin_x, std::int32_t origin_y,
                          std::int32_t width, std::int32_t height,
                          MapFloor* floor) {
    if (!ok()) return false;
    floor->z = z;
    floor->offset = offset;
    for (std::int32_t ix = 0; ix < width; ++ix) {
        for (std::int32_t iy = 0; iy < height; ++iy) {
            if (skip_ > 0) {
                skip_ -= 1;
                skipped_tiles_ += 1;
                continue;
            }
            std::uint16_t word = 0;
            if (!PeekU16(&word)) {
                return Fail(MapDecodeError::Truncated, at_, "tile object or skip marker");
            }
            if (word >= kSkipMarkerBase) {
                std::uint16_t marker = 0;
                static_cast<void>(ReadU16(&marker));
                skip_ = static_cast<std::uint32_t>(marker & 0x00FFU);
                skipped_tiles_ += 1;
                continue;
            }

            MapTile tile;
            tile.position = {origin_x + ix, origin_y + iy, z};
            std::uint32_t skip_after = 0;
            if (!ReadDescribedTile(&tile, &skip_after)) return false;
            thing_count_ += tile.things.size();
            for (const MapThing& thing : tile.things) {
                if (thing.kind == MapThingKind::Creature) creature_count_ += 1;
            }
            floor->tiles.push_back(std::move(tile));
            described_tiles_ += 1;
            skip_ = skip_after;
        }
    }
    return true;
}

bool MapScanner::FinishRun() {
    if (!ok()) return false;
    if (skip_ != 0) {
        return Fail(MapDecodeError::SkipRunExceedsWindow, at_,
                    "skip run outlives the scanned window");
    }
    return true;
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
        case MapDecodeError::UnexpectedCommand: return "UnexpectedCommand";
        case MapDecodeError::InvalidStackIndex: return "InvalidStackIndex";
        case MapDecodeError::InvalidDirection: return "InvalidDirection";
        case MapDecodeError::TrailingBytes: return "TrailingBytes";
        case MapDecodeError::InvalidInventorySlot: return "InvalidInventorySlot";
        case MapDecodeError::UnknownTalkMode: return "UnknownTalkMode";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
