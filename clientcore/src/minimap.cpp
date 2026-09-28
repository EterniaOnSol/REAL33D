#include "fusion32/protocol772/minimap.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <queue>

namespace fusion32::protocol772 {
namespace {
bool ValidPosition(const MapPosition& p) {
    return p.x >= 0 && p.x <= 65535 && p.y >= 0 && p.y <= 65535 && p.z >= 0 && p.z <= 15;
}
}
std::vector<MinimapCell> ProjectMinimapFloor(const WorldState& state,
    const ObjectTypeTable& types, const std::map<std::uint16_t, std::uint8_t>& colors) {
    std::vector<MinimapCell> cells;
    if (!state.map_initialized || !state.viewport_synchronized()) return cells;
    const auto window = state.AnchoredWindow();
    for (const auto& floor : state.floors) {
        if (floor.z != state.viewport_anchor.z) continue;
        for (const auto& tile : floor.tiles) {
            const auto& p = tile.position;
            if (!ValidPosition(p) || p.z != floor.z || p.x < window.min_x
                || p.x >= window.min_x + kTerminalWidth || p.y < window.min_y
                || p.y >= window.min_y + kTerminalHeight) continue;
            MinimapCell cell;
            cell.position = p;
            for (const auto& thing : tile.things) {
                if (thing.kind != MapThingKind::Item) continue;
                const auto type = types.Lookup(thing.item.type_id);
                if (!type.known) continue;
                if (type.priority == ObjectPriority::Bank) cell.terrain.ground_type = thing.item.type_id;
                if (type.unpass && (type.unmove || type.priority == ObjectPriority::Bank))
                    cell.terrain.static_obstacle = true;
                if ((type.priority == ObjectPriority::Bank || type.unmove)
                    && colors.count(thing.item.type_id)) cell.terrain.color_type = thing.item.type_id;
            }
            cells.push_back(cell);
        }
    }
    return cells;
}
void KnownMinimap::Observe(const std::vector<MinimapCell>& cells) {
    live_.clear();
    for (const auto& cell : cells) {
        if (!ValidPosition(cell.position)) continue;
        const auto found = terrain_.find(cell.position);
        if (found == terrain_.end() && terrain_.size() >= kCellLimit) continue;
        terrain_[cell.position] = cell.terrain;
        live_.insert(cell.position);
    }
}
bool KnownMinimap::FindPath(const MapPosition& start, const MapPosition& goal,
                           const std::set<MapPosition>& observed_blockers,
                           std::vector<MapPosition>& path) const {
    path.clear();
    if (!ValidPosition(start) || !ValidPosition(goal) || start.z != goal.z) return false;
    if (start == goal) return true;
    const auto usable = [&](const MapPosition& p) {
        const auto found = terrain_.find(p);
        return ValidPosition(p) && found != terrain_.end() && found->second.ground_type != 0
            && !found->second.static_obstacle && observed_blockers.count(p) == 0;
    };
    if (!usable(goal)) return false;
    struct Entry { MapPosition position; int cost; int estimate; };
    struct Prefer {
        bool operator()(const Entry& a, const Entry& b) const {
            if (a.estimate != b.estimate) return a.estimate > b.estimate;
            if (a.cost != b.cost) return a.cost < b.cost;
            return b.position < a.position;
        }
    };
    struct Record { int cost; MapPosition parent; };
    const auto distance = [&](const MapPosition& p) { return std::abs(p.x-goal.x) + std::abs(p.y-goal.y); };
    std::priority_queue<Entry, std::vector<Entry>, Prefer> frontier;
    std::map<MapPosition, Record> visited;
    frontier.push({start,0,distance(start)});
    visited.emplace(start, Record{0,start});
    while (!frontier.empty()) {
        const auto at = frontier.top();
        frontier.pop();
        if (visited.at(at.position).cost != at.cost) continue;
        if (at.position == goal) {
            for (auto p = goal; !(p == start); p = visited.at(p).parent) path.push_back(p);
            std::reverse(path.begin(), path.end());
            return true;
        }
        const MapPosition next[] = {
            {at.position.x,at.position.y-1,start.z}, {at.position.x+1,at.position.y,start.z},
            {at.position.x,at.position.y+1,start.z}, {at.position.x-1,at.position.y,start.z}};
        for (const auto& p : next) {
            if (!usable(p)) continue;
            const int cost = at.cost + 1;
            const auto known = visited.find(p);
            if (known != visited.end() && known->second.cost <= cost) continue;
            visited.insert_or_assign(p, Record{cost,at.position});
            frontier.push({p,cost,cost+distance(p)});
        }
    }
    return false;
}
bool KnownMinimap::FindNavigationPath(const MapPosition& start, const MapPosition& goal,
    const std::set<MapPosition>& observed_blockers, std::vector<MapPosition>& path) const {
    path.clear();
    if (!ValidPosition(start) || !ValidPosition(goal) || start.z != goal.z) return false;
    const auto usable = [&](const MapPosition& p) {
        const auto found = terrain_.find(p);
        return ValidPosition(p) && found != terrain_.end() && found->second.ground_type != 0
            && !found->second.static_obstacle && observed_blockers.count(p) == 0;
    };
    if (observed_blockers.count(goal) && !(goal == start)) return false;
    if (terrain_.count(goal) && !(goal == start) && !usable(goal)) return false;
    if (FindPath(start,goal,observed_blockers,path)) return true;
    struct Record { MapPosition parent; int cost; };
    std::map<MapPosition, Record> visited;
    std::vector<MapPosition> queue{start};
    visited.emplace(start,Record{start,0});
    bool found_frontier = false;
    MapPosition frontier;
    int best_distance = 0, best_cost = 0;
    for (std::size_t i=0; i<queue.size(); ++i) {
        const auto at = queue[i];
        const int cost = visited.at(at).cost;
        const MapPosition next[] = {{at.x,at.y-1,at.z},{at.x+1,at.y,at.z},
                                    {at.x,at.y+1,at.z},{at.x-1,at.y,at.z}};
        for (const auto& p : next) {
            if (!ValidPosition(p)) continue;
            // Stop the segment at observed terrain; never append an unknown tile.
            if (!(at == start) && !terrain_.count(p) && !observed_blockers.count(p)) {
                const int distance = std::abs(p.x-goal.x)+std::abs(p.y-goal.y);
                if (!found_frontier || distance < best_distance
                    || (distance == best_distance && cost < best_cost)) {
                    found_frontier = true; frontier = at;
                    best_distance = distance; best_cost = cost;
                }
            }
            if (!usable(p) || visited.count(p)) continue;
            visited.emplace(p,Record{at,cost+1});
            queue.push_back(p);
        }
    }
    if (!found_frontier) return false;
    for (auto p=frontier; !(p == start); p=visited.at(p).parent) path.push_back(p);
    std::reverse(path.begin(),path.end());
    return !path.empty();
}

std::string KnownMinimap::Save(const std::string& scope) const {
    std::ostringstream out;
    out << "REAL33D_MINIMAP_2 " << scope << ' ' << terrain_.size() << '\n';
    for (const auto& entry : terrain_) {
        out << entry.first.x << ' ' << entry.first.y << ' ' << entry.first.z << ' '
            << entry.second.ground_type << ' ' << entry.second.static_obstacle << ' '
            << entry.second.color_type << '\n';
    }
    return out.str();
}
bool KnownMinimap::Load(const std::string& text, const std::string& scope) {
    if (text.size() > 32U * 1024U * 1024U || scope.empty()) return false;
    std::istringstream in(text);
    std::string magic, loaded_scope;
    std::size_t count = 0;
    if (!(in >> magic >> loaded_scope >> count)
        || (magic != "REAL33D_MINIMAP_1" && magic != "REAL33D_MINIMAP_2")
        || loaded_scope != scope || count > kCellLimit) return false;
    std::map<MapPosition, MinimapTerrain> parsed;
    for (std::size_t i = 0; i < count; ++i) {
        MapPosition p;
        unsigned int ground = 0, obstacle = 0, color_type = 0;
        if (!(in >> p.x >> p.y >> p.z >> ground >> obstacle) || !ValidPosition(p)
            || ground >= kSkipMarkerBase || (ground != 0 && ground < kFirstMapObjectTypeId)
            || obstacle > 1 || parsed.count(p) != 0) return false;
        if (magic == "REAL33D_MINIMAP_2" && (!(in >> color_type)
            || color_type >= kSkipMarkerBase || (color_type != 0 && color_type < kFirstMapObjectTypeId))) return false;
        parsed.emplace(p, MinimapTerrain{static_cast<std::uint16_t>(ground), obstacle != 0,
            static_cast<std::uint16_t>(color_type)});
    }
    in >> std::ws;
    if (!in.eof()) return false;
    terrain_.swap(parsed);
    live_.clear();
    return true;
}
} // namespace fusion32::protocol772
