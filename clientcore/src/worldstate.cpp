#include "fusion32/protocol772/worldstate.h"

#include <algorithm>

namespace fusion32::protocol772 {

const MapTile* WorldState::FindTile(const MapPosition& position) const noexcept {
    for (const MapFloor& floor : floors) {
        if (floor.z != position.z) continue;
        for (const MapTile& tile : floor.tiles) {
            if (tile.position == position) return &tile;
        }
    }
    return nullptr;
}

MapTile* WorldState::FindTile(const MapPosition& position) noexcept {
    for (MapFloor& floor : floors) {
        if (floor.z != position.z) continue;
        for (MapTile& tile : floor.tiles) {
            if (tile.position == position) return &tile;
        }
    }
    return nullptr;
}

MapWindow WorldState::AnchoredWindow() const noexcept {
    MapWindow anchored;
    anchored.player_position = viewport_anchor;
    anchored.min_x = viewport_anchor.x - kTerminalOffsetX;
    anchored.min_y = viewport_anchor.y - kTerminalOffsetY;
    anchored.width = kTerminalWidth;
    anchored.height = kTerminalHeight;
    return anchored;
}

bool WorldState::viewport_synchronized() const noexcept {
    if (local_creature_id == 0) return false;
    const auto found = known_creatures.find(local_creature_id);
    if (found == known_creatures.end()) return false;
    return found->second.position == viewport_anchor;
}

std::vector<std::uint32_t> WorldState::visible_creature_ids() const {
    std::vector<std::uint32_t> ids;
    for (const MapFloor& floor : floors) {
        for (const MapTile& tile : floor.tiles) {
            for (const MapThing& thing : tile.things) {
                if (thing.kind != MapThingKind::Creature) continue;
                ids.push_back(thing.creature.creature_id);
            }
        }
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::size_t WorldState::tile_count() const noexcept {
    std::size_t total = 0;
    for (const MapFloor& floor : floors) total += floor.tiles.size();
    return total;
}

std::size_t WorldState::thing_count() const noexcept {
    std::size_t total = 0;
    for (const MapFloor& floor : floors) {
        for (const MapTile& tile : floor.tiles) total += tile.things.size();
    }
    return total;
}

const char* CreatureDescriptorKindName(CreatureDescriptorKind kind) noexcept {
    switch (kind) {
        case CreatureDescriptorKind::Known: return "Known";
        case CreatureDescriptorKind::Outdated: return "Outdated";
        case CreatureDescriptorKind::Introduced: return "Introduced";
    }
    return "Unknown";
}

const char* WorldStateAnomalyKindName(WorldStateAnomalyKind kind) noexcept {
    switch (kind) {
        case WorldStateAnomalyKind::ViewportDesynchronized:
            return "ViewportDesynchronized";
        case WorldStateAnomalyKind::StackIndexOutOfRange:
            return "StackIndexOutOfRange";
        case WorldStateAnomalyKind::MoveOriginMismatch:
            return "MoveOriginMismatch";
        case WorldStateAnomalyKind::FieldOutsideViewport:
            return "FieldOutsideViewport";
        case WorldStateAnomalyKind::UnknownCreatureReference:
            return "UnknownCreatureReference";
        case WorldStateAnomalyKind::UnknownEvictedCreature:
            return "UnknownEvictedCreature";
        case WorldStateAnomalyKind::KnownCreatureTableOverflow:
            return "KnownCreatureTableOverflow";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
