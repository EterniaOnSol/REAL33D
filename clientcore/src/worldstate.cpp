#include "fusion32/protocol772/worldstate.h"

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
