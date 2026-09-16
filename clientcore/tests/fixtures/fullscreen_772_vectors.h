#ifndef FUSION32_PROTOCOL772_TEST_FULLSCREEN_772_VECTORS_H
#define FUSION32_PROTOCOL772_TEST_FULLSCREEN_772_VECTORS_H

#include "fusion32/protocol772/initial_world.h"
#include "fusion32/protocol772/worldstate.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace fusion32::protocol772::test_vectors {

// A literal port of the 7.72 emitter so fixtures are produced by the traced
// algorithm instead of being retyped. Mirrors, symbol for symbol:
//   reference/game/src/sending.cc::SendFullScreen
//   reference/game/src/sending.cc::SendMapPoint
//   reference/game/src/sending.cc::SkipFlush
//   reference/game/src/sending.cc::SendMapObject / SendItem / SendOutfit
class FullScreenEncoder {
public:
    using TileProvider = std::function<const std::vector<MapThing>*(const MapPosition&)>;

    static std::vector<std::uint8_t> Encode(std::uint16_t player_x,
                                            std::uint16_t player_y,
                                            std::uint8_t player_z,
                                            const TileProvider& tiles) {
        FullScreenEncoder encoder(tiles);
        encoder.Run(player_x, player_y, player_z);
        return encoder.out_;
    }

private:
    explicit FullScreenEncoder(const TileProvider& tiles) : tiles_(tiles) {}

    void Byte(std::uint8_t value) { out_.push_back(value); }

    void Word(std::uint16_t value) {
        out_.push_back(static_cast<std::uint8_t>(value));
        out_.push_back(static_cast<std::uint8_t>(value >> 8U));
    }

    void Quad(std::uint32_t value) {
        out_.push_back(static_cast<std::uint8_t>(value));
        out_.push_back(static_cast<std::uint8_t>(value >> 8U));
        out_.push_back(static_cast<std::uint8_t>(value >> 16U));
        out_.push_back(static_cast<std::uint8_t>(value >> 24U));
    }

    void Str(const std::string& value) {
        Word(static_cast<std::uint16_t>(value.size()));
        for (const char character : value) {
            out_.push_back(static_cast<std::uint8_t>(character));
        }
    }

    void Outfit(const OutfitDescriptor& outfit) {
        Word(outfit.outfit_id);
        if (outfit.outfit_id == 0) {
            Word(outfit.object_type);
        } else {
            for (const std::uint8_t color : outfit.colors) Byte(color);
        }
    }

    void Thing(const MapThing& thing) {
        if (thing.kind == MapThingKind::Item) {
            Word(thing.item.type_id);
            if (thing.item.has_liquid_color) Byte(thing.item.liquid_color);
            if (thing.item.has_amount) Byte(thing.item.amount);
            return;
        }
        const CreatureThing& creature = thing.creature;
        if (creature.kind == CreatureDescriptorKind::Known) {
            Word(kCreatureMarkerKnown);
            Quad(creature.creature_id);
            Byte(creature.direction);
            return;
        }
        if (creature.kind == CreatureDescriptorKind::Introduced) {
            Word(kCreatureMarkerNew);
            Quad(creature.removed_creature_id);
            Quad(creature.creature_id);
            Str(creature.name);
        } else {
            Word(kCreatureMarkerOutdated);
            Quad(creature.creature_id);
        }
        Byte(creature.health_percent);
        Byte(creature.direction);
        Outfit(creature.outfit);
        Byte(creature.light_brightness);
        Byte(creature.light_color);
        Word(creature.speed);
        Byte(creature.playerkilling_mark);
        Byte(creature.party_mark);
    }

    void SkipFlush() {
        while (skip_ >= 0) {
            const int count = std::min(skip_, 255);
            Byte(static_cast<std::uint8_t>(count));
            Byte(0xFF);
            skip_ -= (count + 1);
        }
    }

    void MapPoint(const MapPosition& position) {
        const std::vector<MapThing>* things = tiles_ ? tiles_(position) : nullptr;
        if (things != nullptr && !things->empty()) {
            SkipFlush();
            std::size_t emitted = 0;
            for (const MapThing& thing : *things) {
                if (emitted >= kMapObjectsPerPointLimit) break;
                Thing(thing);
                emitted += 1;
            }
        }
        skip_ += 1;
    }

    void Run(std::uint16_t player_x, std::uint16_t player_y, std::uint8_t player_z) {
        const std::int32_t px = static_cast<std::int32_t>(player_x);
        const std::int32_t py = static_cast<std::int32_t>(player_y);
        const std::int32_t pz = static_cast<std::int32_t>(player_z);
        const std::int32_t min_x = px - kTerminalOffsetX;
        const std::int32_t min_y = py - kTerminalOffsetY;
        const std::int32_t max_x = min_x + kTerminalWidth - 1;
        const std::int32_t max_y = min_y + kTerminalHeight - 1;

        std::int32_t start_z = 0;
        std::int32_t end_z = 0;
        std::int32_t step_z = 0;
        if (pz <= kSurfaceFloor) {
            step_z = -1;
            start_z = kSurfaceFloor;
            end_z = 0 + step_z;
        } else {
            step_z = 1;
            start_z = pz - 2;
            end_z = std::min<std::int32_t>(pz + 2, kMaxFloor) + step_z;
        }

        Byte(kServerCommandFullScreen);
        Word(player_x);
        Word(player_y);
        Byte(player_z);

        skip_ = -1;
        for (std::int32_t z = start_z; z != end_z; z += step_z) {
            const std::int32_t offset = pz - z;
            for (std::int32_t x = min_x; x <= max_x; x += 1) {
                for (std::int32_t y = min_y; y <= max_y; y += 1) {
                    MapPoint({x + offset, y + offset, z});
                }
            }
        }
        SkipFlush();
    }

    TileProvider tiles_;
    std::vector<std::uint8_t> out_;
    int skip_ = -1;
};

inline std::vector<std::uint8_t> HexBytes(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    std::uint16_t pending = 0;
    bool half = false;
    for (const char character : hex) {
        int value = -1;
        if (character >= '0' && character <= '9') value = character - '0';
        else if (character >= 'a' && character <= 'f') value = character - 'a' + 10;
        else if (character >= 'A' && character <= 'F') value = character - 'A' + 10;
        else continue;
        if (!half) {
            pending = static_cast<std::uint16_t>(value << 4);
            half = true;
        } else {
            bytes.push_back(static_cast<std::uint8_t>(pending | value));
            half = false;
        }
    }
    return bytes;
}

// Hand-computed golden bytes. Player (1000, 1000, 7): floors 7..0, an 18x14
// window, 2016 scanned positions, every one of them empty. SkipFlush drains
// Skip = 2015 as seven (255, 0xFF) markers followed by (223, 0xFF).
inline const char* kGoldenEmptySurfaceHex =
    "64"            // SV_CMD_FULLSCREEN
    "e803"          // PlayerX = 1000
    "e803"          // PlayerY = 1000
    "07"            // PlayerZ = 7
    "ffff" "ffff" "ffff" "ffff" "ffff" "ffff" "ffff"
    "dfff";

// Player (1000, 1000, 8): floors 6..10, 1260 positions, all empty. Skip = 1259
// drains as four (255, 0xFF) markers followed by (235, 0xFF).
inline const char* kGoldenEmptyUndergroundHex =
    "64"
    "e803"
    "e803"
    "08"
    "ffff" "ffff" "ffff" "ffff"
    "ebff";

// Player (1000, 1000, 7) with one described tile at the first scanned position
// (992, 994, 7) holding, in stack order: a plain item, an introduced creature,
// a cumulative item and a liquid container. The tile terminator is the first
// (255, 0xFF) of the eight markers that drain the remaining 2015 positions.
inline const char* kGoldenPopulatedTileHex =
    "64"
    "e803"
    "e803"
    "07"
    "6400"                      // item type 100
    "6100"                      // creature marker 97 (introduced)
    "00000000"                  // evicted creature id 0
    "01020304"                  // creature id 0x04030201
    "0400" "526f6f6b"           // name "Rook"
    "64"                        // health 100%
    "02"                        // direction 2 (DIRECTION_SOUTH)
    "8000"                      // outfit id 128
    "4e453a4c"                  // colors 78, 69, 58, 76
    "00"                        // light brightness
    "00"                        // light color
    "dc00"                      // speed 220
    "00"                        // playerkilling mark (SKULL_NONE)
    "00"                        // party mark (PARTY_SHIELD_NONE)
    "c800" "25"                 // item type 200, cumulative amount 37
    "2c01" "05"                 // item type 300, liquid colour 5
    "ffff" "ffff" "ffff" "ffff" "ffff" "ffff" "ffff"
    "dfff";

inline ObjectTypeTable GoldenObjectTypeTable() {
    ObjectTypeTable table;
    ObjectTypeEncoding plain;
    ObjectTypeEncoding cumulative;
    cumulative.cumulative = true;
    ObjectTypeEncoding liquid;
    liquid.liquid_color = true;
    table.Declare(100, plain);
    table.Declare(101, plain);
    table.Declare(102, plain);
    table.Declare(200, cumulative);
    table.Declare(300, liquid);
    table.Declare(400, plain);
    return table;
}

inline MapThing PlainItem(std::uint16_t type_id) {
    MapThing thing;
    thing.kind = MapThingKind::Item;
    thing.item.type_id = type_id;
    return thing;
}

inline MapThing CumulativeItem(std::uint16_t type_id, std::uint8_t amount) {
    MapThing thing = PlainItem(type_id);
    thing.item.has_amount = true;
    thing.item.amount = amount;
    return thing;
}

inline MapThing LiquidItem(std::uint16_t type_id, std::uint8_t color) {
    MapThing thing = PlainItem(type_id);
    thing.item.has_liquid_color = true;
    thing.item.liquid_color = color;
    return thing;
}

inline MapThing IntroducedCreature(std::uint32_t creature_id, std::uint32_t evicted,
                                   const std::string& name) {
    MapThing thing;
    thing.kind = MapThingKind::Creature;
    CreatureThing& creature = thing.creature;
    creature.kind = CreatureDescriptorKind::Introduced;
    creature.creature_id = creature_id;
    creature.evicts_slot = true;
    creature.removed_creature_id = evicted;
    creature.has_name = true;
    creature.name = name;
    creature.has_descriptor = true;
    creature.health_percent = 100;
    creature.direction = 2;
    creature.outfit.outfit_id = 128;
    creature.outfit.colors = {78, 69, 58, 76};
    creature.speed = 220;
    return thing;
}

inline MapThing OutdatedCreature(std::uint32_t creature_id) {
    MapThing thing = IntroducedCreature(creature_id, 0, "");
    thing.creature.kind = CreatureDescriptorKind::Outdated;
    thing.creature.evicts_slot = false;
    thing.creature.removed_creature_id = 0;
    thing.creature.has_name = false;
    thing.creature.name.clear();
    return thing;
}

inline MapThing KnownCreature(std::uint32_t creature_id, std::uint8_t direction) {
    MapThing thing;
    thing.kind = MapThingKind::Creature;
    thing.creature.kind = CreatureDescriptorKind::Known;
    thing.creature.creature_id = creature_id;
    thing.creature.direction = direction;
    return thing;
}

}  // namespace fusion32::protocol772::test_vectors

#endif
