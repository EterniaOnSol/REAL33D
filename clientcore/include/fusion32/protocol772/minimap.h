#ifndef FUSION32_PROTOCOL772_MINIMAP_H
#define FUSION32_PROTOCOL772_MINIMAP_H
#include "fusion32/protocol772/worldstate.h"
#include <set>

namespace fusion32::protocol772 {
// Observed terrain hints. No entity/stack or assertion of current walkability.
struct MinimapTerrain {
    std::uint16_t ground_type = 0;
    bool static_obstacle = false;
    std::uint16_t color_type = 0; // Last observed static appearance with a color hint.
    bool operator==(const MinimapTerrain& other) const noexcept {
        return ground_type == other.ground_type && static_obstacle == other.static_obstacle
            && color_type == other.color_type;
    }
};
struct MinimapCell {
    MapPosition position;
    MinimapTerrain terrain;
};
// Only described tiles on the actual current player floor. Does not alter State.
std::vector<MinimapCell> ProjectMinimapFloor(const WorldState& state,
    const ObjectTypeTable& types, const std::map<std::uint16_t, std::uint8_t>& colors = {});

class KnownMinimap {
public:
    static constexpr std::size_t kCellLimit = 1000000;
    void Observe(const std::vector<MinimapCell>& cells);
    void EndObservation() noexcept { live_.clear(); }
    const std::map<MapPosition, MinimapTerrain>& terrain() const noexcept { return terrain_; }
    bool IsLive(const MapPosition& position) const { return live_.count(position) != 0; }
    std::size_t live_count() const noexcept { return live_.size(); }
    // Same-floor route estimate through retained terrain, no viewport/distance
    // cutoff. Current observed blockers override history. Caller must validate
    // every real adjacent step against live state and wait for server acceptance.
    bool FindPath(const MapPosition& start, const MapPosition& goal,
                  const std::set<MapPosition>& observed_blockers,
                  std::vector<MapPosition>& path) const;
    // Unknown goals are requests, not discovered terrain. Returns a known
    // segment toward an exploration frontier when a complete route is unknown.
    // Caller keeps the original goal and replans after real observations arrive.
    bool FindNavigationPath(const MapPosition& start, const MapPosition& goal,
                            const std::set<MapPosition>& observed_blockers,
                            std::vector<MapPosition>& path) const;
    // Own versioned terrain-only format, not OTMM. Scope is a non-secret world/
    // character key supplied by caller. Load is bounded and transactional.
    std::string Save(const std::string& scope) const;
    bool Load(const std::string& text, const std::string& scope);
private:
    std::map<MapPosition, MinimapTerrain> terrain_;
    std::set<MapPosition> live_;
};
} // namespace fusion32::protocol772
#endif
