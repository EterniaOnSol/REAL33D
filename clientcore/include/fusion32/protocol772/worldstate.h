#ifndef FUSION32_PROTOCOL772_WORLDSTATE_H
#define FUSION32_PROTOCOL772_WORLDSTATE_H

#include "fusion32/protocol772/object_types.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Terminal geometry. Source: reference/game/src/connections.cc lines 219-222,
// assigned once in the TConnection constructor and never reassigned anywhere
// else in the tree.
constexpr std::int32_t kTerminalOffsetX = 8;
constexpr std::int32_t kTerminalOffsetY = 6;
constexpr std::int32_t kTerminalWidth = 18;
constexpr std::int32_t kTerminalHeight = 14;

// Source: reference/game/src/connections.hh, TKnownCreature KnownCreatureTable[150].
constexpr std::size_t kKnownCreatureTableSize = 150;

// Source: reference/game/src/cr.hh, TCreature::Name is char[30].
constexpr std::size_t kCreatureNameLimit = 29;

struct MapPosition {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    bool operator==(const MapPosition& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const MapPosition& other) const noexcept { return !(*this == other); }
    bool operator<(const MapPosition& other) const noexcept {
        if (z != other.z) return z < other.z;
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

// Source: reference/game/src/sending.cc::SendItem.
struct ItemThing {
    std::uint16_t type_id = 0;
    bool has_liquid_color = false;
    std::uint8_t liquid_color = 0;
    bool has_amount = false;
    std::uint8_t amount = 0;
};

// Source: reference/game/src/sending.cc::SendOutfit and cr.hh::TOutfit, whose
// union makes the two branches mutually exclusive.
struct OutfitDescriptor {
    std::uint16_t outfit_id = 0;
    bool disguised_as_object = false;
    std::uint16_t object_type = 0;
    std::array<std::uint8_t, 4> colors{};
};

enum class CreatureDescriptorKind {
    Known,      // word 99: already up to date on this connection
    Outdated,   // word 98: known id, full descriptor follows
    Introduced, // word 97: new slot, evicts `removed_creature_id`
};

// Source: reference/game/src/sending.cc::SendMapObject, creature branch.
struct CreatureThing {
    CreatureDescriptorKind kind = CreatureDescriptorKind::Known;
    std::uint32_t creature_id = 0;
    bool evicts_slot = false;
    std::uint32_t removed_creature_id = 0;
    bool has_name = false;
    std::string name;
    bool has_descriptor = false;
    std::uint8_t health_percent = 0;
    std::uint8_t direction = 0;
    OutfitDescriptor outfit;
    std::uint8_t light_brightness = 0;
    std::uint8_t light_color = 0;
    std::uint16_t speed = 0;
    std::uint8_t playerkilling_mark = 0;
    std::uint8_t party_mark = 0;
};

enum class MapThingKind { Item, Creature };

struct MapThing {
    MapThingKind kind = MapThingKind::Item;
    ItemThing item;
    CreatureThing creature;
};

// Stack order is the server's object linked list order; the index in `things`
// is the stack position. Source: reference/game/src/sending.cc::SendMapPoint
// walking `Obj.getNextObject()`.
struct MapTile {
    MapPosition position;
    std::vector<MapThing> things;
};

struct MapFloor {
    std::int32_t z = 0;
    std::int32_t offset = 0;  // PlayerZ - z, added to both x and y
    std::vector<MapTile> tiles;  // described tiles only, in emission order
};

struct MapWindow {
    MapPosition player_position;
    std::int32_t min_x = 0;  // before the per-floor offset
    std::int32_t min_y = 0;
    std::int32_t width = kTerminalWidth;
    std::int32_t height = kTerminalHeight;
};

// What the decoded FULLSCREEN establishes about a creature, and nothing more.
struct CreatureRecord {
    std::uint32_t creature_id = 0;
    bool has_descriptor = false;
    std::string name;
    std::uint8_t health_percent = 0;
    std::uint8_t direction = 0;
    OutfitDescriptor outfit;
    std::uint8_t light_brightness = 0;
    std::uint8_t light_color = 0;
    std::uint16_t speed = 0;
    std::uint8_t playerkilling_mark = 0;
    std::uint8_t party_mark = 0;
    MapPosition position;
    std::size_t stack_position = 0;
};

enum class WorldStateAnomalyKind {
    // A word-98 or word-99 descriptor named a creature the client had never
    // been introduced to. The server only emits those for entries present in
    // its own KnownCreatureTable, so the local mirror is out of step.
    UnknownCreatureReference,
    // A word-97 descriptor evicted a slot the local mirror did not hold.
    UnknownEvictedCreature,
    // More live creatures than the server-side table can hold.
    KnownCreatureTableOverflow,
};

struct WorldStateAnomaly {
    WorldStateAnomalyKind kind = WorldStateAnomalyKind::UnknownCreatureReference;
    std::uint32_t creature_id = 0;
    MapPosition position;
};

struct WorldState {
    bool map_initialized = false;
    MapWindow window;
    std::vector<MapFloor> floors;
    std::map<std::uint32_t, CreatureRecord> known_creatures;

    const MapTile* FindTile(const MapPosition& position) const noexcept;
    std::size_t tile_count() const noexcept;
    std::size_t thing_count() const noexcept;
};

const char* CreatureDescriptorKindName(CreatureDescriptorKind kind) noexcept;
const char* WorldStateAnomalyKindName(WorldStateAnomalyKind kind) noexcept;

}  // namespace fusion32::protocol772

#endif
