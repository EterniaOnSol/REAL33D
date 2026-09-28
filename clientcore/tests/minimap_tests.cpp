#include "fusion32/protocol772/minimap.h"
#include "fusion32/protocol772/initial_world.h"
#include "fixtures/fullscreen_772_vectors.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
using namespace fusion32::protocol772;
namespace v = fusion32::protocol772::test_vectors;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)

int main() {
    try {
        const auto loaded = LoadObjectTypeTableFromObjectsSrv(
            "TypeID = 100\nFlags = {Bank,Unmove}\n"
            "TypeID = 101\nFlags = {Bank,Unmove,Unpass}\n"
            "TypeID = 102\nFlags = {Bank,Unpass}\n"
            "TypeID = 200\nFlags = {Bottom,Unmove,Unpass}\n"
            "TypeID = 201\nFlags = {Unpass}\n");
        CHECK(loaded.ok()); CHECK(loaded.table.Lookup(200).unmove);
        CHECK(!loaded.table.Lookup(201).unmove);
        std::map<MapPosition, std::vector<MapThing>> tiles;
        for (int z : {6, 7, 8}) {
            for (int x = 990; x < 1040; ++x) {
                for (int y = 990; y < 1020; ++y) tiles[{x,y,z}] = {v::PlainItem(100)};
            }
        }
        tiles[{1001,1000,7}].push_back(v::PlainItem(200));
        tiles[{1002,1000,7}].push_back(v::PlainItem(201));
        tiles[{1003,1000,7}] = {v::PlainItem(102)};
        const auto observe = [&](MapPosition p) {
            auto copy = tiles;
            copy[p].push_back(v::IntroducedCreature(42, 0, "Player"));
            v::ServerEmitter emitter([&copy](const MapPosition& at) -> const std::vector<MapThing>* {
                const auto found = copy.find(at);
                return found == copy.end() ? nullptr : &found->second;
            });
            emitter.FullScreen(static_cast<std::uint16_t>(p.x), static_cast<std::uint16_t>(p.y),
                               static_cast<std::uint8_t>(p.z));
            const auto decoded = DecodeFullScreen(emitter.bytes(), loaded.table);
            CHECK(decoded.ok());
            WorldState world;
            world.local_creature_id = 42;
            CHECK(ApplyFullScreen(&world, decoded.message).clean());
            return world;
        };
        KnownMinimap map;
        auto world = observe({1000,1000,7});
        const auto cells = ProjectMinimapFloor(world, loaded.table);
        CHECK(cells.size() == 18U*14U);
        map.Observe(cells);
        CHECK(map.terrain().size() == 252); CHECK(map.live_count() == 252);
        CHECK(map.terrain().at({1001,1000,7}).static_obstacle);
        CHECK(!map.terrain().at({1002,1000,7}).static_obstacle); // movable blocker excluded
        CHECK(map.terrain().at({1003,1000,7}).static_obstacle); // Bank+Unpass blocks without explicit Unmove
        CHECK(map.terrain().at({1000,1000,7}).ground_type == 100); // player excluded
        CHECK(map.terrain().count({1000,1000,6}) == 0); // other described floors undiscovered
        const std::map<std::uint16_t, std::uint8_t> palette{
            {std::uint16_t{100},std::uint8_t{24}},
            {std::uint16_t{200},std::uint8_t{129}},
            {std::uint16_t{201},std::uint8_t{210}}};
        KnownMinimap colored;
        colored.Observe(ProjectMinimapFloor(world, loaded.table, palette));
        CHECK(colored.terrain().at({1001,1000,7}).color_type == 200); // static wall color wins
        CHECK(colored.terrain().at({1002,1000,7}).color_type == 100); // movable color excluded
        CHECK(colored.terrain().at({1000,1000,7}).color_type == 100); // creature excluded
        KnownMinimap colored_copy;
        CHECK(colored_copy.Load(colored.Save("colors"), "colors"));
        CHECK(colored_copy.terrain() == colored.terrain());
        map.Observe(ProjectMinimapFloor(observe({1020,1000,7}), loaded.table));
        CHECK(map.terrain().size() == 504); CHECK(!map.IsLive({1000,1000,7}));
        CHECK(map.terrain().at({1000,1000,7}).ground_type == 100);
        map.Observe(ProjectMinimapFloor(observe({1000,1000,8}), loaded.table));
        CHECK(map.terrain().count({1000,1000,8}) == 1);
        CHECK(!map.IsLive({1000,1000,7}));
        map.Observe(ProjectMinimapFloor(observe({1000,1000,7}), loaded.table));
        CHECK(map.IsLive({1000,1000,7})); CHECK(map.terrain().count({1000,1000,8}) == 1);
        tiles[{1000,1000,7}] = {v::PlainItem(101)};
        map.Observe(ProjectMinimapFloor(observe({1000,1000,7}), loaded.table));
        CHECK(map.terrain().at({1000,1000,7}).ground_type == 101);
        world.known_creatures.at(42).position.x += 1;
        CHECK(ProjectMinimapFloor(world, loaded.table).empty()); // no desynchronized discovery
        const auto saved = map.Save("world_character");
        KnownMinimap restored;
        CHECK(restored.Load(saved, "world_character")); CHECK(restored.live_count() == 0);
        CHECK(restored.terrain() == map.terrain());
        CHECK(!restored.Load(saved, "different_character"));
        for (const auto& bad : {
            "REAL33D_MINIMAP_1 world_character 1000001\n",
            "REAL33D_MINIMAP_1 world_character 1\n-1 1 7 100 0\n",
            "REAL33D_MINIMAP_1 world_character 1\n1 1 16 100 0\n",
            "REAL33D_MINIMAP_1 world_character 1\n1 1 7 99 0\n",
            "REAL33D_MINIMAP_1 world_character 1\n1 1 7 100 2\n",
            "REAL33D_MINIMAP_1 world_character 2\n1 1 7 100 0\n1 1 7 100 0\n",
            "REAL33D_MINIMAP_1 world_character 1\n1 1 7 100 0\ntrailing",
            "REAL33D_MINIMAP_2 world_character 1\n1 1 7 100 0 99\n",
            "REAL33D_MINIMAP_3 world_character 0\n"}) {
            CHECK(!restored.Load(bad, "world_character"));
            CHECK(restored.terrain() == map.terrain());
        }
        map.EndObservation(); CHECK(map.live_count() == 0); CHECK(!map.terrain().empty());
        // Known exploration, not a wider live viewport: a 300-step corridor.
        KnownMinimap routes;
        std::vector<MinimapCell> corridor;
        for (int x=1000; x<=1300; ++x) corridor.push_back({{x,1000,7},{100,false,100}});
        routes.Observe(corridor);
        routes.EndObservation(); // Historical terrain still supplies estimates.
        std::vector<MapPosition> path;
        CHECK(routes.FindPath({1000,1000,7},{1300,1000,7},{},path));
        CHECK(path.size() == 300);
        CHECK((path.front() == MapPosition{1001,1000,7}));
        CHECK((path.back() == MapPosition{1300,1000,7}));
        CHECK(!routes.FindPath({1000,1000,7},{1301,1000,7},{},path)); CHECK(path.empty());
        CHECK(!routes.FindPath({1000,1000,7},{1300,1000,6},{},path));
        CHECK(!routes.FindPath({1000,1000,7},{1300,1000,7},{{1150,1000,7}},path));
        routes.Observe({{{1150,1000,7},{100,true,200}}});
        CHECK(!routes.FindPath({1000,1000,7},{1300,1000,7},{},path));
        routes.Observe({{{1149,1001,7},{100,false,100}},{{1150,1001,7},{100,false,100}},
                        {{1151,1001,7},{100,false,100}}});
        CHECK(routes.FindPath({1000,1000,7},{1300,1000,7},{},path)); CHECK(path.size() == 302);
        auto previous = MapPosition{1000,1000,7};
        for (const auto& step : path) {
            CHECK(step.z == previous.z);
            CHECK(std::abs(step.x-previous.x)+std::abs(step.y-previous.y) == 1);
            CHECK(!(step == MapPosition{1150,1000,7}));
            previous = step;
        }
        CHECK(!routes.FindPath({1000,1000,7},{1300,1000,7},{{1160,1000,7}},path));
        KnownMinimap route_cache;
        CHECK(route_cache.Load(routes.Save("route"),"route"));
        CHECK(route_cache.FindPath({1000,1000,7},{1300,1000,7},{},path));
        CHECK(path.size() == 302); CHECK(route_cache.live_count() == 0);
        KnownMinimap edge;
        edge.Observe({{{65535,0,7},{100,false,100}}});
        CHECK(edge.FindPath({65534,0,7},{65535,0,7},{},path)); CHECK(path.size() == 1);
        CHECK(!edge.FindPath({65535,0,7},{65536,0,7},{},path)); CHECK(path.empty());
        KnownMinimap exploration;
        exploration.Observe(corridor); exploration.EndObservation();
        const auto original_count = exploration.terrain().size();
        CHECK(exploration.FindNavigationPath({1000,1000,7},{1600,1000,7},{},path));
        CHECK(path.size() == 300); CHECK((path.back() == MapPosition{1300,1000,7}));
        CHECK(exploration.terrain().size() == original_count); CHECK(exploration.live_count() == 0);
        for (const auto& step : path) CHECK(exploration.terrain().count(step) == 1);
        // A distant unknown coordinate does not allocate unknown-world nodes.
        CHECK(exploration.FindNavigationPath({1000,1000,7},{65000,1000,7},{},path));
        CHECK(path.size() == 300); CHECK(exploration.terrain().size() == original_count);
        std::vector<MinimapCell> discovered;
        for (int x=1301; x<=1600; ++x) discovered.push_back({{x,1000,7},{100,false,100}});
        exploration.Observe(discovered);
        CHECK(exploration.FindNavigationPath({1300,1000,7},{1600,1000,7},{},path));
        CHECK(path.size() == 300); CHECK((path.back() == MapPosition{1600,1000,7}));
        CHECK(!exploration.FindNavigationPath({1300,1000,7},{1600,1000,6},{},path));
        CHECK(!exploration.FindNavigationPath({1300,1000,7},{65000,1000,7},{{65000,1000,7}},path));
        exploration.Observe({{{1600,1000,7},{100,true,200}}});
        CHECK(!exploration.FindNavigationPath({1300,1000,7},{1600,1000,7},{},path));
        std::cout << "minimap_tests: PASS (18x14 observation, retention/cache, known/unknown-goal segments, blockers/detour/bounds; no phantom discovery)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "minimap_tests: FAILED: " << e.what() << '\n'; return 1;
    }
}
