#ifndef FUSION32_PROTOCOL772_WORLDVIEW_H
#define FUSION32_PROTOCOL772_WORLDVIEW_H

#include "fusion32/protocol772/worldstate.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Turns successive WorldStates into the semantic events a presentation layer
// needs, so that layer never sees a packet, an opcode or a byte.
//
// This adapter is deliberately free of any engine dependency: it is ordinary
// C++17 and is covered by the same deterministic suites as the rest of
// ClientCore. Unreal consumes what comes out of here; it does not reach past
// it into the protocol.
//
// WorldState remains authoritative. These events describe what changed in it,
// never what a client wishes would change.

enum class WorldEventKind {
    LocalPlayerIdentified,
    AnchorMoved,
    TileUpserted,
    TileRemoved,
    CreatureAppeared,
    CreatureMoved,
    CreatureVanished,
};

const char* WorldEventKindName(WorldEventKind kind) noexcept;

struct WorldEvent {
    WorldEventKind kind = WorldEventKind::AnchorMoved;

    // Where the thing is now. For TileRemoved and CreatureVanished this is
    // where it last was.
    MapPosition position;
    // Only meaningful for CreatureMoved.
    MapPosition previous_position;

    std::uint32_t creature_id = 0;
    std::string creature_name;
    std::uint8_t direction = 0;
    bool is_local_player = false;

    // Only meaningful for TileUpserted: the tile's stack in order.
    std::vector<MapThing> things;
};

// Diffs one WorldState against the last one it was shown.
//
// Holding its own copy of the previous view is what lets the presentation layer
// stay incremental without the network thread knowing anything about actors.
class WorldView {
public:
    // Returns the events that take the previous view to `next`, in an order a
    // consumer can apply directly: identity first, then the anchor, then tile
    // removals, then tile upserts, then creature changes.
    std::vector<WorldEvent> Diff(const WorldState& next);

    void Reset() noexcept;

    bool initialised() const noexcept { return initialised_; }
    const MapPosition& anchor() const noexcept { return anchor_; }
    std::size_t tile_count() const noexcept { return tiles_.size(); }
    std::size_t creature_count() const noexcept { return creatures_.size(); }

private:
    struct CreatureView {
        MapPosition position;
        std::string name;
        std::uint8_t direction = 0;
    };

    bool initialised_ = false;
    std::uint32_t local_creature_id_ = 0;
    MapPosition anchor_;
    std::map<MapPosition, std::vector<MapThing>> tiles_;
    std::map<std::uint32_t, CreatureView> creatures_;
};

// True when two stacks would draw identically, used to avoid re-emitting a
// tile that did not change.
bool SameStack(const std::vector<MapThing>& left,
               const std::vector<MapThing>& right) noexcept;

}  // namespace fusion32::protocol772

#endif
