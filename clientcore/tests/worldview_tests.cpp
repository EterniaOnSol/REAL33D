#include "fusion32/protocol772/worldview.h"

#include "fusion32/protocol772/movement.h"

#include "fixtures/fullscreen_772_vectors.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace fusion32::protocol772;
namespace vectors = fusion32::protocol772::test_vectors;

class Failure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define CHECK(condition) do { if (!(condition)) throw Failure(#condition); } while (false)

using TileMap = std::map<MapPosition, std::vector<MapThing>>;

vectors::FullScreenEncoder::TileProvider Provider(const TileMap& tiles) {
    return [&tiles](const MapPosition& position) -> const std::vector<MapThing>* {
        const auto found = tiles.find(position);
        return found == tiles.end() ? nullptr : &found->second;
    };
}

const ObjectTypeTable& Types() {
    static const ObjectTypeTable table = vectors::GoldenObjectTypeTable();
    return table;
}

std::size_t CountOf(const std::vector<WorldEvent>& events, WorldEventKind kind) {
    return static_cast<std::size_t>(
        std::count_if(events.begin(), events.end(),
                      [kind](const WorldEvent& e) { return e.kind == kind; }));
}

const WorldEvent* Find(const std::vector<WorldEvent>& events, WorldEventKind kind,
                       std::uint32_t creature_id) {
    for (const WorldEvent& event : events) {
        if (event.kind == kind && event.creature_id == creature_id) return &event;
    }
    return nullptr;
}

TileMap BuildWorld(const MapPosition& player, std::uint32_t player_id) {
    TileMap tiles;
    for (std::int32_t x = player.x - 20; x <= player.x + 20; ++x) {
        for (std::int32_t y = player.y - 20; y <= player.y + 20; ++y) {
            tiles[{x, y, 7}] = {vectors::PlainItem(100)};
        }
    }
    tiles[player].push_back(vectors::IntroducedCreature(player_id, 0, "Walker"));
    return tiles;
}

WorldState Apply(const TileMap& tiles, const MapPosition& player,
                 std::uint32_t player_id) {
    vectors::ServerEmitter emitter(Provider(tiles));
    emitter.FullScreen(static_cast<std::uint16_t>(player.x),
                       static_cast<std::uint16_t>(player.y),
                       static_cast<std::uint8_t>(player.z));
    const auto decoded = DecodeFullScreen(emitter.bytes(), Types());
    if (!decoded.ok()) throw Failure("fixture full screen did not decode");
    WorldState state;
    state.local_creature_id = player_id;
    ApplyFullScreen(&state, decoded.message);
    return state;
}

void TestFirstDiffDescribesTheWholeWorld() {
    const std::uint32_t player_id = 1002;
    const MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);
    WorldState state = Apply(tiles, player, player_id);

    WorldView view;
    CHECK(!view.initialised());
    const auto events = view.Diff(state);
    CHECK(view.initialised());

    CHECK(CountOf(events, WorldEventKind::LocalPlayerIdentified) == 1);
    CHECK(CountOf(events, WorldEventKind::AnchorMoved) == 1);
    CHECK(CountOf(events, WorldEventKind::TileUpserted) == state.tile_count());
    CHECK(CountOf(events, WorldEventKind::TileRemoved) == 0);
    CHECK(CountOf(events, WorldEventKind::CreatureAppeared) == 1);
    CHECK(view.anchor() == player);

    // Identity must arrive before anything that refers to it.
    CHECK(events.front().kind == WorldEventKind::LocalPlayerIdentified);
    CHECK(events.front().creature_id == player_id);
    CHECK(events.front().is_local_player);

    const WorldEvent* appeared =
        Find(events, WorldEventKind::CreatureAppeared, player_id);
    CHECK(appeared != nullptr);
    CHECK(appeared->position == player);
    CHECK(appeared->is_local_player);
    CHECK(appeared->creature_name == "Walker");
}

void TestUnchangedStateProducesNothing() {
    const std::uint32_t player_id = 1002;
    const MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);
    WorldState state = Apply(tiles, player, player_id);

    WorldView view;
    CHECK(!view.Diff(state).empty());
    // The same state again is not news. A presentation layer that rebuilt the
    // viewport on every frame would be doing so for nothing.
    CHECK(view.Diff(state).empty());
}

void TestStepEmitsOnlyWhatMoved() {
    const std::uint32_t player_id = 1002;
    MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);

    WorldState state = Apply(tiles, player, player_id);
    WorldView view;
    view.Diff(state);
    const std::size_t tiles_before = view.tile_count();

    // Walk east the way the server describes it: the creature moves, then the
    // row arrives and the anchor follows.
    const MapPosition destination = StepPosition(player, CardinalDirection::East);
    std::size_t index = 0;
    const MapTile* origin_tile = state.FindTile(player);
    CHECK(origin_tile != nullptr);
    for (std::size_t i = 0; i < origin_tile->things.size(); ++i) {
        if (origin_tile->things[i].kind == MapThingKind::Creature) index = i;
    }

    vectors::ServerEmitter move(Provider(tiles));
    move.MoveCreature(player, static_cast<std::uint8_t>(index), destination);
    auto decoded = DecodeServerUpdate(move.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    const auto after_move = view.Diff(state);
    const WorldEvent* moved = Find(after_move, WorldEventKind::CreatureMoved, player_id);
    CHECK(moved != nullptr);
    CHECK(moved->previous_position == player);
    CHECK(moved->position == destination);
    // The anchor has not moved yet: only the row moves it.
    CHECK(CountOf(after_move, WorldEventKind::AnchorMoved) == 0);
    CHECK(view.anchor() == player);

    tiles[player].pop_back();
    tiles[destination].push_back(vectors::IntroducedCreature(player_id, 0, "Walker"));
    vectors::ServerEmitter row(Provider(tiles));
    row.Row(destination.x, destination.y, destination.z, CardinalDirection::East);
    decoded = DecodeServerUpdate(row.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    const auto after_row = view.Diff(state);
    CHECK(CountOf(after_row, WorldEventKind::AnchorMoved) == 1);
    CHECK(view.anchor() == destination);
    // A step reveals one column and drops another; it must not redescribe the
    // whole viewport.
    CHECK(CountOf(after_row, WorldEventKind::TileUpserted) < tiles_before / 4);
    CHECK(CountOf(after_row, WorldEventKind::TileRemoved) > 0);
}

void TestAnotherCreatureAppearsMovesAndVanishes() {
    const std::uint32_t player_id = 1002;
    const std::uint32_t other_id = 1001;
    const MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);
    WorldState state = Apply(tiles, player, player_id);
    WorldView view;
    view.Diff(state);

    const MapPosition arrival{1002, 1000, 7};
    vectors::ServerEmitter appear(Provider(tiles));
    appear.AddField(arrival, vectors::IntroducedCreature(other_id, 0, "Test Player A"));
    auto decoded = DecodeServerUpdate(appear.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    auto events = view.Diff(state);
    const WorldEvent* appeared =
        Find(events, WorldEventKind::CreatureAppeared, other_id);
    CHECK(appeared != nullptr);
    CHECK(appeared->position == arrival);
    CHECK(appeared->creature_name == "Test Player A");
    CHECK(!appeared->is_local_player);

    const MapPosition stepped{1001, 1000, 7};
    const std::size_t stack = state.known_creatures.at(other_id).stack_position;
    vectors::ServerEmitter walk(Provider(tiles));
    walk.MoveCreature(arrival, static_cast<std::uint8_t>(stack), stepped);
    decoded = DecodeServerUpdate(walk.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    events = view.Diff(state);
    const WorldEvent* moved = Find(events, WorldEventKind::CreatureMoved, other_id);
    CHECK(moved != nullptr);
    CHECK(moved->previous_position == arrival);
    CHECK(moved->position == stepped);

    const std::size_t stack_after = state.known_creatures.at(other_id).stack_position;
    vectors::ServerEmitter leave(Provider(tiles));
    leave.DeleteField(stepped, static_cast<std::uint8_t>(stack_after));
    decoded = DecodeServerUpdate(leave.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    events = view.Diff(state);
    const WorldEvent* gone = Find(events, WorldEventKind::CreatureVanished, other_id);
    CHECK(gone != nullptr);
    CHECK(gone->position == stepped);
    CHECK(view.creature_count() == 1);
}

// The mirror keeps creatures that scrolled out of view, faithfully to the
// server. A presentation layer must not draw them, so the view reports only
// creatures standing on a stored tile.
void TestScrolledOutCreatureVanishesForPresentation() {
    const std::uint32_t player_id = 1002;
    const std::uint32_t guest_id = 1001;
    MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);
    const MapPosition guest{992, 1000, 7};
    tiles[guest].push_back(vectors::IntroducedCreature(guest_id, 0, "Guest"));

    WorldState state = Apply(tiles, player, player_id);
    WorldView view;
    view.Diff(state);
    CHECK(view.creature_count() == 2);

    // Walk east so the guest's column leaves the window.
    const MapPosition destination = StepPosition(player, CardinalDirection::East);
    tiles[player].pop_back();
    tiles[destination].push_back(vectors::IntroducedCreature(player_id, 0, "Walker"));
    vectors::ServerEmitter row(Provider(tiles));
    row.Row(destination.x, destination.y, destination.z, CardinalDirection::East);
    const auto decoded = DecodeServerUpdate(row.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    const auto events = view.Diff(state);
    CHECK(Find(events, WorldEventKind::CreatureVanished, guest_id) != nullptr);
    CHECK(view.creature_count() == 1);
    // The mirror still holds it, exactly as the server's table does.
    CHECK(state.known_creatures.count(guest_id) == 1);
}

void TestResetForgetsEverything() {
    const std::uint32_t player_id = 1002;
    const MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);
    WorldState state = Apply(tiles, player, player_id);

    WorldView view;
    const std::size_t first = view.Diff(state).size();
    CHECK(view.Diff(state).empty());

    // A reconnect must redescribe the world rather than assume the actors
    // from the previous session are still correct.
    view.Reset();
    CHECK(!view.initialised());
    CHECK(view.tile_count() == 0);
    CHECK(view.creature_count() == 0);
    CHECK(view.Diff(state).size() == first);
}

void TestCombatIsOneWorldStateDiff() {
    const std::uint32_t player_id = 1002;
    const std::uint32_t first_target = 1001;
    const std::uint32_t second_target = 1003;
    const MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);
    WorldState state = Apply(tiles, player, player_id);

    WorldView view;
    view.Diff(state);

    NoteCombatRequest(&state, first_target, false);
    auto events = view.Diff(state);
    CHECK(CountOf(events, WorldEventKind::CombatChanged) == 1);
    const WorldEvent* changed =
        Find(events, WorldEventKind::CombatChanged, first_target);
    CHECK(changed != nullptr);
    CHECK(changed->combat.target_creature_id == first_target);
    CHECK(!changed->combat.following);

    // Showing the same WorldState twice is not a second UI selection.
    CHECK(view.Diff(state).empty());

    NoteCombatRequest(&state, second_target, true);
    events = view.Diff(state);
    changed = Find(events, WorldEventKind::CombatChanged, second_target);
    CHECK(changed != nullptr);
    CHECK(changed->combat.following);

    NoteTacticsRequest(&state, AttackMode::Defensive, ChaseMode::Follow,
                       SecureMode::Enabled);
    events = view.Diff(state);
    CHECK(CountOf(events, WorldEventKind::CombatChanged) == 1);
    changed = Find(events, WorldEventKind::CombatChanged, second_target);
    CHECK(changed != nullptr);
    CHECK(changed->combat.tactics_sent);
    CHECK(changed->combat.attack_mode == 3);
    CHECK(changed->combat.chase_mode == 1);

    NoteCombatRequest(&state, 0, false);
    events = view.Diff(state);
    changed = Find(events, WorldEventKind::CombatChanged, 0);
    CHECK(changed != nullptr);
    CHECK(changed->combat.target_creature_id == 0);
    CHECK(!changed->combat.following);

    // A reconnect resets the adapter's combat mirror along with its actors.
    NoteCombatRequest(&state, first_target, false);
    view.Diff(state);
    view.Reset();
    WorldState fresh = Apply(tiles, player, player_id);
    events = view.Diff(fresh);
    CHECK(CountOf(events, WorldEventKind::CombatChanged) == 0);
}

void TestUninitialisedStateProducesNothing() {
    WorldState empty;
    WorldView view;
    CHECK(view.Diff(empty).empty());
    CHECK(!view.initialised());
}

void TestSameStackHelper() {
    const std::vector<MapThing> a{vectors::PlainItem(100), vectors::CumulativeItem(200, 5)};
    const std::vector<MapThing> b{vectors::PlainItem(100), vectors::CumulativeItem(200, 5)};
    const std::vector<MapThing> c{vectors::PlainItem(100), vectors::CumulativeItem(200, 6)};
    const std::vector<MapThing> d{vectors::PlainItem(100)};
    CHECK(SameStack(a, b));
    CHECK(!SameStack(a, c));
    CHECK(!SameStack(a, d));
    CHECK(std::string(WorldEventKindName(WorldEventKind::TileRemoved)) == "TileRemoved");
}

}  // namespace

int main() {
    try {
        TestFirstDiffDescribesTheWholeWorld();
        TestUnchangedStateProducesNothing();
        TestStepEmitsOnlyWhatMoved();
        TestAnotherCreatureAppearsMovesAndVanishes();
        TestScrolledOutCreatureVanishesForPresentation();
        TestResetForgetsEverything();
        TestCombatIsOneWorldStateDiff();
        TestUninitialisedStateProducesNothing();
        TestSameStackHelper();
        std::cout << "protocol772_worldview_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_worldview_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
