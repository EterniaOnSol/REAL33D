#include "fusion32/protocol772/worldview.h"

#include <algorithm>

namespace fusion32::protocol772 {
namespace {

bool SameThing(const MapThing& left, const MapThing& right) noexcept {
    if (left.kind != right.kind) return false;
    if (left.kind == MapThingKind::Item) {
        return left.item.type_id == right.item.type_id
            && left.item.has_liquid_color == right.item.has_liquid_color
            && left.item.liquid_color == right.item.liquid_color
            && left.item.has_amount == right.item.has_amount
            && left.item.amount == right.item.amount;
    }
    return left.creature.creature_id == right.creature.creature_id;
}

bool SameCombat(const CombatState& left, const CombatState& right) noexcept {
    return left.target_creature_id == right.target_creature_id
        && left.following == right.following
        && left.tactics_sent == right.tactics_sent
        && left.attack_mode == right.attack_mode
        && left.chase_mode == right.chase_mode
        && left.secure_mode == right.secure_mode;
}

}  // namespace

bool SameStack(const std::vector<MapThing>& left,
               const std::vector<MapThing>& right) noexcept {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!SameThing(left[i], right[i])) return false;
    }
    return true;
}

void WorldView::Reset() noexcept {
    initialised_ = false;
    local_creature_id_ = 0;
    anchor_ = MapPosition{};
    tiles_.clear();
    creatures_.clear();
    combat_ = CombatState{};
}

std::vector<WorldEvent> WorldView::Diff(const WorldState& next) {
    std::vector<WorldEvent> events;
    if (!next.map_initialized) return events;

    if (next.local_creature_id != 0 && next.local_creature_id != local_creature_id_) {
        local_creature_id_ = next.local_creature_id;
        WorldEvent event;
        event.kind = WorldEventKind::LocalPlayerIdentified;
        event.creature_id = local_creature_id_;
        event.is_local_player = true;
        const auto record = next.known_creatures.find(local_creature_id_);
        if (record != next.known_creatures.end()) {
            event.creature_name = record->second.name;
            event.position = record->second.position;
            event.direction = record->second.direction;
        }
        events.push_back(std::move(event));
    }

    if (!initialised_ || !(anchor_ == next.viewport_anchor)) {
        WorldEvent event;
        event.kind = WorldEventKind::AnchorMoved;
        event.previous_position = anchor_;
        event.position = next.viewport_anchor;
        anchor_ = next.viewport_anchor;
        events.push_back(std::move(event));
    }

    // Accepted targets have no server acknowledgement. The command sender
    // records them in WorldState once their bytes reach the wire; every server
    // refusal or later revocation applies CLEAR_TARGET to that same record.
    // Diffing it here gives every presentation one source of truth.
    if (!SameCombat(combat_, next.combat)) {
        WorldEvent event;
        event.kind = WorldEventKind::CombatChanged;
        event.creature_id = next.combat.target_creature_id;
        event.combat = next.combat;
        combat_ = next.combat;
        events.push_back(std::move(event));
    }

    // ---- tiles -----------------------------------------------------------
    std::map<MapPosition, const MapTile*> current;
    for (const MapFloor& floor : next.floors) {
        for (const MapTile& tile : floor.tiles) {
            current[tile.position] = &tile;
        }
    }

    // Removals first, so a consumer never holds two things on one field while
    // a creature is being moved between them.
    for (const auto& entry : tiles_) {
        if (current.find(entry.first) != current.end()) continue;
        WorldEvent event;
        event.kind = WorldEventKind::TileRemoved;
        event.position = entry.first;
        events.push_back(std::move(event));
    }

    for (const auto& entry : current) {
        const auto previous = tiles_.find(entry.first);
        if (previous != tiles_.end()
            && SameStack(previous->second, entry.second->things)) {
            continue;
        }
        WorldEvent event;
        event.kind = WorldEventKind::TileUpserted;
        event.position = entry.first;
        event.things = entry.second->things;
        events.push_back(std::move(event));
    }

    std::map<MapPosition, std::vector<MapThing>> new_tiles;
    for (const auto& entry : current) {
        new_tiles[entry.first] = entry.second->things;
    }
    tiles_.swap(new_tiles);

    // ---- creatures -------------------------------------------------------
    // Only creatures standing on a stored tile are present. The known-creature
    // mirror deliberately retains entries for creatures that scrolled out of
    // view, which is faithful to the server but is not what a presentation
    // layer should draw.
    std::map<std::uint32_t, CreatureView> visible;
    for (const auto& entry : tiles_) {
        for (const MapThing& thing : entry.second) {
            if (thing.kind != MapThingKind::Creature) continue;
            CreatureView view;
            view.position = entry.first;
            view.direction = thing.creature.direction;
            const auto record = next.known_creatures.find(thing.creature.creature_id);
            if (record != next.known_creatures.end()) {
                view.name = record->second.name;
                view.direction = record->second.direction;
            }
            visible[thing.creature.creature_id] = std::move(view);
        }
    }

    for (const auto& entry : visible) {
        const auto previous = creatures_.find(entry.first);
        WorldEvent event;
        event.creature_id = entry.first;
        event.creature_name = entry.second.name;
        event.direction = entry.second.direction;
        event.position = entry.second.position;
        event.is_local_player = entry.first == local_creature_id_;
        if (previous == creatures_.end()) {
            event.kind = WorldEventKind::CreatureAppeared;
            events.push_back(std::move(event));
        } else if (!(previous->second.position == entry.second.position)) {
            event.kind = WorldEventKind::CreatureMoved;
            event.previous_position = previous->second.position;
            events.push_back(std::move(event));
        } else if (previous->second.direction != entry.second.direction) {
            // A turn is a move to the same field: the consumer reorients
            // without relocating.
            event.kind = WorldEventKind::CreatureMoved;
            event.previous_position = previous->second.position;
            events.push_back(std::move(event));
        }
    }

    for (const auto& entry : creatures_) {
        if (visible.find(entry.first) != visible.end()) continue;
        WorldEvent event;
        event.kind = WorldEventKind::CreatureVanished;
        event.creature_id = entry.first;
        event.creature_name = entry.second.name;
        event.position = entry.second.position;
        event.is_local_player = entry.first == local_creature_id_;
        events.push_back(std::move(event));
    }

    creatures_.swap(visible);
    initialised_ = true;
    return events;
}

const char* WorldEventKindName(WorldEventKind kind) noexcept {
    switch (kind) {
        case WorldEventKind::LocalPlayerIdentified: return "LocalPlayerIdentified";
        case WorldEventKind::AnchorMoved: return "AnchorMoved";
        case WorldEventKind::TileUpserted: return "TileUpserted";
        case WorldEventKind::TileRemoved: return "TileRemoved";
        case WorldEventKind::CreatureAppeared: return "CreatureAppeared";
        case WorldEventKind::CreatureMoved: return "CreatureMoved";
        case WorldEventKind::CreatureVanished: return "CreatureVanished";
        case WorldEventKind::CombatChanged: return "CombatChanged";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
