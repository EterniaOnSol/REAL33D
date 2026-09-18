#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/movement_ledger.h"
#include "fusion32/protocol772/talk_command.h"
#include "fusion32/protocol772/talk_speaker.h"

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

// Provider captures its argument by reference, so a temporary would dangle.
// Commands that carry no map data still need a provider; this gives them one
// backed by storage that outlives the emitter.
vectors::FullScreenEncoder::TileProvider NoTiles() {
    static const TileMap empty;
    return Provider(empty);
}

const ObjectTypeTable& Types() {
    static const ObjectTypeTable table = vectors::GoldenObjectTypeTable();
    return table;
}

// A compact identity for a stacked thing, used to compare an incrementally
// updated WorldState against a freshly emitted full screen.
std::string ThingIdentity(const MapThing& thing) {
    if (thing.kind == MapThingKind::Item) {
        std::string text = "item:" + std::to_string(thing.item.type_id);
        if (thing.item.has_liquid_color) {
            text += ":l" + std::to_string(thing.item.liquid_color);
        }
        if (thing.item.has_amount) {
            text += ":n" + std::to_string(thing.item.amount);
        }
        return text;
    }
    return "creature:" + std::to_string(thing.creature.creature_id);
}

std::map<MapPosition, std::vector<std::string>> Snapshot(const WorldState& state) {
    std::map<MapPosition, std::vector<std::string>> snapshot;
    for (const MapFloor& floor : state.floors) {
        for (const MapTile& tile : floor.tiles) {
            std::vector<std::string> stack;
            for (const MapThing& thing : tile.things) stack.push_back(ThingIdentity(thing));
            snapshot[tile.position] = std::move(stack);
        }
    }
    return snapshot;
}

std::map<MapPosition, std::vector<std::string>> Snapshot(const FullScreenMessage& message) {
    std::map<MapPosition, std::vector<std::string>> snapshot;
    for (const MapFloor& floor : message.floors) {
        for (const MapTile& tile : floor.tiles) {
            std::vector<std::string> stack;
            for (const MapThing& thing : tile.things) stack.push_back(ThingIdentity(thing));
            snapshot[tile.position] = std::move(stack);
        }
    }
    return snapshot;
}

// --------------------------------------------------------------- client side

void TestClientCommands() {
    CHECK(WalkCommandOpcode(CardinalDirection::North) == 101);
    CHECK(WalkCommandOpcode(CardinalDirection::East) == 102);
    CHECK(WalkCommandOpcode(CardinalDirection::South) == 103);
    CHECK(WalkCommandOpcode(CardinalDirection::West) == 104);
    CHECK(TurnCommandOpcode(CardinalDirection::North) == 111);
    CHECK(TurnCommandOpcode(CardinalDirection::West) == 114);

    // CGoDirection takes no payload: a cardinal walk is exactly one byte.
    CHECK(BuildWalkCommand(CardinalDirection::East) == (std::vector<std::uint8_t>{102}));
    CHECK(BuildTurnCommand(CardinalDirection::South) == (std::vector<std::uint8_t>{113}));
    CHECK(BuildStopCommand() == (std::vector<std::uint8_t>{105}));

    // The offsets receiving.cc::ReceiveData passes to CGoDirection.
    const MapPosition origin{100, 100, 7};
    CHECK(StepPosition(origin, CardinalDirection::North) == (MapPosition{100, 99, 7}));
    CHECK(StepPosition(origin, CardinalDirection::East) == (MapPosition{101, 100, 7}));
    CHECK(StepPosition(origin, CardinalDirection::South) == (MapPosition{100, 101, 7}));
    CHECK(StepPosition(origin, CardinalDirection::West) == (MapPosition{99, 100, 7}));

    CHECK(std::string(CardinalDirectionName(CardinalDirection::West)) == "West");
}

// ------------------------------------------------------------ golden bytes

void TestGoldenRows() {
    const TileMap empty;
    vectors::ServerEmitter east(Provider(empty));
    east.Row(1000, 1000, 7, CardinalDirection::East);
    const auto golden_east = vectors::HexBytes(vectors::kGoldenRowEastEmptyHex);
    CHECK(east.bytes() == golden_east);
    CHECK(golden_east.size() == 3);

    auto decoded = DecodeServerUpdate(golden_east, 0, {999, 1000, 7}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::Row);
    CHECK(decoded.update.row.direction == CardinalDirection::East);
    CHECK(decoded.update.resulting_anchor == (MapPosition{1000, 1000, 7}));
    CHECK(decoded.update.bytes_consumed == golden_east.size());
    CHECK(decoded.update.row.floors.size() == 8);
    CHECK(std::string(decoded.update.name) == "SV_CMD_ROW_EAST");

    vectors::ServerEmitter north(Provider(empty));
    north.Row(1000, 1000, 7, CardinalDirection::North);
    const auto golden_north = vectors::HexBytes(vectors::kGoldenRowNorthEmptyHex);
    CHECK(north.bytes() == golden_north);

    decoded = DecodeServerUpdate(golden_north, 0, {1000, 1001, 7}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.row.direction == CardinalDirection::North);
    CHECK(decoded.update.resulting_anchor == (MapPosition{1000, 1000, 7}));
}

void TestRowCoversTheRevealedEdgeOnly() {
    // One tile on the newly revealed east column and one just inside it, which
    // the row must not describe.
    TileMap tiles;
    tiles[{1009, 994, 7}] = {vectors::PlainItem(100)};  // MaxX of the stepped window
    tiles[{1008, 994, 7}] = {vectors::PlainItem(101)};
    vectors::ServerEmitter emitter(Provider(tiles));
    emitter.Row(1000, 1000, 7, CardinalDirection::East);

    const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, {999, 1000, 7}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.row.floors[0].tiles.size() == 1);
    CHECK(decoded.update.row.floors[0].tiles[0].position == (MapPosition{1009, 994, 7}));
    // 8 floors of a single 14-field column.
    std::size_t described = 0;
    std::size_t total = 0;
    for (const MapFloor& floor : decoded.update.row.floors) {
        described += floor.tiles.size();
        total += 14;
    }
    CHECK(described == 1);
    CHECK(total == 112);
}

void TestGoldenFloorChanges() {
    const TileMap empty;

    vectors::ServerEmitter down(Provider(empty));
    down.Floors(999, 999, 8, false);
    const auto golden_down = vectors::HexBytes(vectors::kGoldenFloorDownEmptyHex);
    CHECK(down.bytes() == golden_down);
    CHECK(golden_down.size() == 7);

    auto decoded = DecodeServerUpdate(golden_down, 0, {1000, 1000, 7}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::FloorChange);
    CHECK(!decoded.update.floor_change.going_up);
    CHECK(decoded.update.resulting_anchor == (MapPosition{999, 999, 8}));
    CHECK(decoded.update.floor_change.floors.size() == 3);
    CHECK(decoded.update.floor_change.floors[0].z == 8);
    CHECK(decoded.update.floor_change.floors[2].z == 10);

    vectors::ServerEmitter up(Provider(empty));
    up.Floors(1000, 1000, 7, true);
    const auto golden_up = vectors::HexBytes(vectors::kGoldenFloorUpEmptyHex);
    CHECK(up.bytes() == golden_up);
    CHECK(golden_up.size() == 13);

    decoded = DecodeServerUpdate(golden_up, 0, {999, 999, 8}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.resulting_anchor == (MapPosition{1000, 1000, 7}));
    CHECK(decoded.update.floor_change.floors.size() == 6);
    CHECK(decoded.update.floor_change.floors[0].z == 5);
    CHECK(decoded.update.floor_change.floors[5].z == 0);

    // Above the surface the client already holds every floor, so SendFloors
    // emits nothing but the opcode.
    vectors::ServerEmitter none(Provider(empty));
    none.Floors(1000, 1000, 6, true);
    const auto golden_none = vectors::HexBytes(vectors::kGoldenFloorUpNoFloorsHex);
    CHECK(none.bytes() == golden_none);
    CHECK(golden_none.size() == 1);

    decoded = DecodeServerUpdate(golden_none, 0, {999, 999, 7}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.floor_change.floors.empty());
    CHECK(decoded.update.bytes_consumed == 1);
    CHECK(decoded.update.resulting_anchor == (MapPosition{1000, 1000, 6}));

    CHECK(FloorChangeRange(7, true).count == 6);
    CHECK(FloorChangeRange(8, false).count == 3);
    CHECK(FloorChangeRange(9, false).count == 1);
    CHECK(FloorChangeRange(9, true).count == 1);
    CHECK(FloorChangeRange(6, true).count == 0);
    CHECK(FloorChangeRange(14, false).count == 0);
}

void TestGoldenMoveCreature() {
    const TileMap empty;
    vectors::ServerEmitter emitter(Provider(empty));
    emitter.MoveCreature({1000, 1000, 7}, 1, {1001, 1000, 7});
    const auto golden = vectors::HexBytes(vectors::kGoldenMoveCreatureHex);
    CHECK(emitter.bytes() == golden);
    // opcode + origin position (5) + stack index + destination position (5).
    CHECK(golden.size() == 12);

    const auto decoded = DecodeServerUpdate(golden, 0, {1000, 1000, 7}, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::MoveCreature);
    CHECK(decoded.update.move_creature.origin == (MapPosition{1000, 1000, 7}));
    CHECK(decoded.update.move_creature.origin_stack_index == 1);
    CHECK(decoded.update.move_creature.destination == (MapPosition{1001, 1000, 7}));
    CHECK(decoded.update.bytes_consumed == 12);
    // SV_CMD_MOVE_CREATURE never advances the viewport; only rows do.
    CHECK(decoded.update.resulting_anchor == (MapPosition{1000, 1000, 7}));
}

void TestGoldenRejectedStep() {
    const TileMap empty;
    vectors::ServerEmitter emitter(Provider(empty));
    emitter.Message(23, "Sorry, not possible.");
    emitter.Snapback(1);
    const auto golden = vectors::HexBytes(vectors::kGoldenRejectedStepHex);
    CHECK(emitter.bytes() == golden);

    const auto message = DecodeServerUpdate(golden, 0, {1000, 1000, 7}, Types());
    CHECK(message.ok());
    CHECK(message.update.kind == ServerUpdateKind::Message);
    CHECK(message.update.message.mode == 23);
    CHECK(message.update.message.text == "Sorry, not possible.");

    const auto snapback = DecodeServerUpdate(golden, message.update.bytes_consumed,
                                             {1000, 1000, 7}, Types());
    CHECK(snapback.ok());
    CHECK(snapback.update.kind == ServerUpdateKind::Snapback);
    CHECK(snapback.update.snapback.direction == 1);
    CHECK(message.update.bytes_consumed + snapback.update.bytes_consumed == golden.size());
}

// ------------------------------------------------------------ stack priority

void TestStackPriority() {
    using P = ObjectPriority;
    // Append path: Bank, Clip, Bottom and Top land after their own class.
    CHECK(MapStackInsertIndex({}, P::Bank) == 0);
    CHECK(MapStackInsertIndex({P::Bank}, P::Clip) == 1);
    CHECK(MapStackInsertIndex({P::Bank, P::Low}, P::Clip) == 1);
    CHECK(MapStackInsertIndex({P::Bank, P::Clip, P::Low}, P::Bottom) == 2);
    CHECK(MapStackInsertIndex({P::Bank, P::Creature, P::Low}, P::Top) == 1);

    // Non-append path: creatures and low objects go before their own class.
    CHECK(MapStackInsertIndex({P::Bank}, P::Creature) == 1);
    CHECK(MapStackInsertIndex({P::Bank, P::Creature}, P::Creature) == 1);
    CHECK(MapStackInsertIndex({P::Bank, P::Creature, P::Low}, P::Creature) == 1);
    // Creature outranks Low, so a low object entering the field lands above any
    // creature already standing on it.
    CHECK(MapStackInsertIndex({P::Bank, P::Creature, P::Low}, P::Low) == 2);
    CHECK(MapStackInsertIndex({P::Bank, P::Top}, P::Low) == 2);
    CHECK(std::string(ObjectPriorityName(P::Creature)) == "Creature");

    CHECK(ThingPriority(vectors::PlainItem(100), Types()) == P::Bank);
    CHECK(ThingPriority(vectors::PlainItem(101), Types()) == P::Low);
    CHECK(ThingPriority(vectors::PlainItem(400), Types()) == P::Top);
    CHECK(ThingPriority(vectors::KnownCreature(1, 0), Types()) == P::Creature);
}

// ---------------------------------------------------------------- end to end

// Builds a small world: every viewport field has a ground object, some fields
// carry extra objects, and the player plus one static creature stand on it.
TileMap BuildWorld(const MapPosition& player, std::uint32_t player_id) {
    TileMap tiles;
    for (std::int32_t x = player.x - 24; x <= player.x + 24; ++x) {
        for (std::int32_t y = player.y - 24; y <= player.y + 24; ++y) {
            std::vector<MapThing> stack{vectors::PlainItem(100)};
            if (((x + y) % 7) == 0) stack.push_back(vectors::CumulativeItem(200, 5));
            if (((x * 3 + y) % 11) == 0) stack.push_back(vectors::LiquidItem(300, 2));
            if (((x + y * 2) % 13) == 0) stack.push_back(vectors::PlainItem(400));
            tiles[{x, y, 7}] = std::move(stack);
        }
    }
    // The player and a bystander, inserted exactly where PlaceObject would.
    std::vector<MapThing>& player_tile = tiles[{player.x, player.y, 7}];
    std::vector<ObjectPriority> priorities;
    for (const MapThing& thing : player_tile) {
        priorities.push_back(ThingPriority(thing, Types()));
    }
    player_tile.insert(
        player_tile.begin()
            + static_cast<std::ptrdiff_t>(
                  MapStackInsertIndex(priorities, ObjectPriority::Creature)),
        vectors::IntroducedCreature(player_id, 0, "Walker"));
    return tiles;
}

void InsertCreature(TileMap* tiles, const MapPosition& position, const MapThing& creature) {
    std::vector<MapThing>& stack = (*tiles)[position];
    std::vector<ObjectPriority> priorities;
    for (const MapThing& thing : stack) priorities.push_back(ThingPriority(thing, Types()));
    stack.insert(stack.begin()
                     + static_cast<std::ptrdiff_t>(
                           MapStackInsertIndex(priorities, ObjectPriority::Creature)),
                 creature);
}

void RemoveCreature(TileMap* tiles, const MapPosition& position, std::uint32_t creature_id) {
    std::vector<MapThing>& stack = (*tiles)[position];
    for (std::size_t i = 0; i < stack.size(); ++i) {
        if (stack[i].kind == MapThingKind::Creature
            && stack[i].creature.creature_id == creature_id) {
            stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
    throw Failure("creature absent from reference tile");
}

std::size_t CreatureStackIndex(const TileMap& tiles, const MapPosition& position,
                               std::uint32_t creature_id) {
    const auto found = tiles.find(position);
    if (found == tiles.end()) throw Failure("reference tile missing");
    for (std::size_t i = 0; i < found->second.size(); ++i) {
        if (found->second[i].kind == MapThingKind::Creature
            && found->second[i].creature.creature_id == creature_id) {
            return i;
        }
    }
    throw Failure("creature absent from reference tile");
}

// Walks the player one cardinal step, emitting exactly what the server would:
// SV_CMD_MOVE_CREATURE from AnnounceMovingCreature, then the SV_CMD_ROW_* that
// NotifyGo produces once it has advanced the player's position.
void StepWorld(TileMap* tiles, WorldState* state, MapPosition* player,
               std::uint32_t player_id, CardinalDirection direction) {
    const MapPosition origin = *player;
    const MapPosition destination = StepPosition(origin, direction);
    const std::size_t origin_index = CreatureStackIndex(*tiles, origin, player_id);

    vectors::ServerEmitter move(Provider(*tiles));
    move.MoveCreature(origin, static_cast<std::uint8_t>(origin_index), destination);
    const auto move_decoded = DecodeServerUpdate(move.bytes(), 0, state->viewport_anchor,
                                                 Types());
    CHECK(move_decoded.ok());
    const auto move_applied = ApplyServerUpdate(state, move_decoded.update, Types());
    CHECK(move_applied.clean());

    // The reference world moves the creature only now, exactly as operate.cc
    // calls MoveObject between AnnounceMovingCreature and NotifyGo.
    MapThing creature = (*tiles)[origin][origin_index];
    RemoveCreature(tiles, origin, player_id);
    InsertCreature(tiles, destination, creature);

    vectors::ServerEmitter row(Provider(*tiles));
    row.Row(destination.x, destination.y, destination.z, direction);
    const auto row_decoded = DecodeServerUpdate(row.bytes(), 0, state->viewport_anchor,
                                                Types());
    CHECK(row_decoded.ok());
    CHECK(row_decoded.update.resulting_anchor == destination);
    const auto row_applied = ApplyServerUpdate(state, row_decoded.update, Types());
    CHECK(row_applied.clean());

    *player = destination;
}

void ExpectMatchesFreshFullScreen(const TileMap& tiles, const WorldState& state,
                                  const MapPosition& player) {
    vectors::ServerEmitter fresh(Provider(tiles));
    fresh.FullScreen(static_cast<std::uint16_t>(player.x),
                     static_cast<std::uint16_t>(player.y),
                     static_cast<std::uint8_t>(player.z));
    const auto decoded = DecodeFullScreen(fresh.bytes(), Types());
    CHECK(decoded.ok());
    CHECK(Snapshot(state) == Snapshot(decoded.message));
    CHECK(state.tile_count() == decoded.message.described_tiles);
    CHECK(state.thing_count() == decoded.message.thing_count);
}

void TestWalkMatchesFreshFullScreen() {
    const std::uint32_t player_id = 0x0100AABB;
    MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());

    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    CHECK(state.viewport_anchor == player);
    CHECK(state.viewport_synchronized());
    ExpectMatchesFreshFullScreen(tiles, state, player);

    const CardinalDirection walk[] = {
        CardinalDirection::East,  CardinalDirection::East,
        CardinalDirection::South, CardinalDirection::South,
        CardinalDirection::South, CardinalDirection::West,
        CardinalDirection::North, CardinalDirection::East,
        CardinalDirection::North, CardinalDirection::West,
        CardinalDirection::West,  CardinalDirection::North,
    };
    for (const CardinalDirection direction : walk) {
        StepWorld(&tiles, &state, &player, player_id, direction);
        CHECK(state.viewport_anchor == player);
        CHECK(state.viewport_synchronized());
        CHECK(state.known_creatures.at(player_id).position == player);
        ExpectMatchesFreshFullScreen(tiles, state, player);
    }

    // Twelve steps later the walk has returned to its origin.
    CHECK(player == (MapPosition{1000, 1000, 7}));
    CHECK(state.tile_count() == screen.message.described_tiles);
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{player_id}));
}

void TestKnownCreatureMirrorRetainsScrolledOutCreatures() {
    const std::uint32_t player_id = 51;
    const std::uint32_t guest_id = 52;
    MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);
    // A bystander on the western edge of the window, which walking east will
    // scroll out of view.
    const MapPosition guest{992, 1000, 7};
    InsertCreature(&tiles, guest, vectors::IntroducedCreature(guest_id, 0, "Guest"));

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    CHECK(state.visible_creature_ids()
          == (std::vector<std::uint32_t>{player_id, guest_id}));

    StepWorld(&tiles, &state, &player, player_id, CardinalDirection::East);

    // The guest's field left the window, so it is gone from the map, but the
    // mirror keeps it exactly as TConnection::KnownCreatureTable does.
    CHECK(state.FindTile(guest) == nullptr);
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{player_id}));
    CHECK(state.known_creatures.count(guest_id) == 1);
    ExpectMatchesFreshFullScreen(tiles, state, player);
}

void TestPlayerStackPositionFollowsPriority() {
    const std::uint32_t player_id = 7;
    MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());

    StepWorld(&tiles, &state, &player, player_id, CardinalDirection::East);

    const MapTile* tile = state.FindTile(player);
    CHECK(tile != nullptr);
    // The ground stays underneath and the creature sits above it but below any
    // Top object, exactly as PlaceObject orders them.
    CHECK(tile->things[0].kind == MapThingKind::Item);
    CHECK(tile->things[0].item.type_id == 100);
    const std::size_t index = state.known_creatures.at(player_id).stack_position;
    CHECK(tile->things[index].kind == MapThingKind::Creature);
    CHECK(tile->things[index].creature.creature_id == player_id);
    CHECK(index == CreatureStackIndex(tiles, player, player_id));
}

void TestRejectedStepLeavesWorldUntouched() {
    const std::uint32_t player_id = 42;
    MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());

    const auto before = Snapshot(state);
    const MapPosition anchor_before = state.viewport_anchor;

    // A blocked walk throws MOVENOTPOSSIBLE, and SendResult answers with the
    // failure message followed by a snapback. No map command is emitted.
    const auto golden = vectors::HexBytes(vectors::kGoldenRejectedStepHex);
    std::size_t at = 0;
    while (at < golden.size()) {
        const auto decoded = DecodeServerUpdate(golden, at, state.viewport_anchor, Types());
        CHECK(decoded.ok());
        const auto applied = ApplyServerUpdate(&state, decoded.update, Types());
        CHECK(applied.clean());
        at += decoded.update.bytes_consumed;
    }
    CHECK(at == golden.size());

    CHECK(Snapshot(state) == before);
    CHECK(state.viewport_anchor == anchor_before);
    CHECK(state.viewport_synchronized());
    CHECK(state.known_creatures.at(player_id).position == player);
    // Only the confirmed facing changed.
    CHECK(state.known_creatures.at(player_id).direction == 1);
}

void TestFloorChangeKeepsOnlyVisibleFloors() {
    const std::uint32_t player_id = 5;
    const MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);
    // Give the underground floors something to describe.
    for (std::int32_t z = 8; z <= 10; ++z) {
        for (std::int32_t x = 980; x <= 1020; ++x) {
            for (std::int32_t y = 980; y <= 1020; ++y) {
                tiles[{x, y, z}] = {vectors::PlainItem(100)};
            }
        }
    }

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    CHECK(state.floors.size() == 8);

    vectors::ServerEmitter down(Provider(tiles));
    down.Floors(999, 999, 8, false);
    const auto decoded = DecodeServerUpdate(down.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());

    CHECK(state.viewport_anchor == (MapPosition{999, 999, 8}));
    // Underground the viewport spans PlayerZ-2 .. PlayerZ+2, so floors 6..10.
    CHECK(state.floors.size() == 5);
    std::vector<std::int32_t> zs;
    for (const MapFloor& floor : state.floors) zs.push_back(floor.z);
    CHECK(zs == (std::vector<std::int32_t>{6, 7, 8, 9, 10}));
    for (const MapFloor& floor : state.floors) {
        for (const MapTile& tile : floor.tiles) {
            const std::int32_t offset = state.viewport_anchor.z - floor.z;
            const std::int32_t min_x = state.viewport_anchor.x - kTerminalOffsetX + offset;
            const std::int32_t min_y = state.viewport_anchor.y - kTerminalOffsetY + offset;
            CHECK(tile.position.x >= min_x && tile.position.x < min_x + kTerminalWidth);
            CHECK(tile.position.y >= min_y && tile.position.y < min_y + kTerminalHeight);
        }
    }
}

// --------------------------------------------------------------- field edits

void TestFieldCommands() {
    const std::uint32_t player_id = 9;
    const MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);
    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());

    const MapPosition target{1002, 1001, 7};
    const std::size_t before = state.FindTile(target)->things.size();

    // AddField carries no stack index; PlaceObject decides where it lands.
    vectors::ServerEmitter add(Provider(tiles));
    add.AddField(target, vectors::PlainItem(400));
    auto decoded = DecodeServerUpdate(add.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::AddField);
    CHECK(decoded.update.add_field.position == target);
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    const MapTile* tile = state.FindTile(target);
    CHECK(tile->things.size() == before + 1);
    // A Top object appends after Bank/Clip/Bottom/Top but before creatures and
    // low objects.
    std::size_t expected = 0;
    for (const MapThing& thing : tile->things) {
        if (ThingPriority(thing, Types()) > ObjectPriority::Top) break;
        expected += 1;
    }
    CHECK(ThingPriority(tile->things[expected - 1], Types()) == ObjectPriority::Top);

    vectors::ServerEmitter change(Provider(tiles));
    change.ChangeField(target, 0, vectors::PlainItem(102));
    decoded = DecodeServerUpdate(change.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.change_field.stack_index == 0);
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.FindTile(target)->things[0].item.type_id == 102);

    vectors::ServerEmitter remove(Provider(tiles));
    remove.DeleteField(target, 0);
    decoded = DecodeServerUpdate(remove.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.FindTile(target)->things.size() == before);
    CHECK(state.FindTile(target)->things[0].item.type_id != 102);

    // FieldData replaces the whole stack.
    vectors::ServerEmitter field(Provider(tiles));
    field.FieldData(target);
    decoded = DecodeServerUpdate(field.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::FieldData);
    CHECK(decoded.update.field_data.position == target);
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.FindTile(target)->things.size() == tiles.at(target).size());
}

void TestDeleteFieldRemovesCreature() {
    const std::uint32_t player_id = 11;
    const std::uint32_t other_id = 12;
    const MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);
    const MapPosition guest{1003, 1000, 7};
    InsertCreature(&tiles, guest, vectors::IntroducedCreature(other_id, 0, "Guest"));

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    CHECK(state.known_creatures.count(other_id) == 1);

    const std::size_t index = CreatureStackIndex(tiles, guest, other_id);
    vectors::ServerEmitter remove(Provider(tiles));
    remove.DeleteField(guest, static_cast<std::uint8_t>(index));
    const auto decoded = DecodeServerUpdate(remove.bytes(), 0, state.viewport_anchor,
                                            Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    // The creature leaves the map but stays in the mirror, because the server
    // sends this command both when a creature scrolls out of view and when it
    // is destroyed, and TConnection::KnownCreatureTable only frees an entry in
    // ~TCreature or when NewKnownCreature reuses the slot.
    CHECK(state.FindTile(guest) == nullptr
          || [&] {
                 for (const MapThing& thing : state.FindTile(guest)->things) {
                     if (thing.kind == MapThingKind::Creature
                         && thing.creature.creature_id == other_id) {
                         return false;
                     }
                 }
                 return true;
             }());
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{player_id}));
    CHECK(state.known_creatures.count(other_id) == 1);
    CHECK(state.known_creatures.count(player_id) == 1);
}

// ------------------------------------------------------------ negative cases

void TestNegativeCases() {
    const MapPosition anchor{1000, 1000, 7};

    // Every truncation of each golden command must fail explicitly.
    const std::vector<std::vector<std::uint8_t>> goldens{
        vectors::HexBytes(vectors::kGoldenRowEastEmptyHex),
        vectors::HexBytes(vectors::kGoldenFloorDownEmptyHex),
        vectors::HexBytes(vectors::kGoldenMoveCreatureHex),
    };
    for (const auto& golden : goldens) {
        for (std::size_t length = 1; length < golden.size(); ++length) {
            const std::vector<std::uint8_t> partial(
                golden.begin(), golden.begin() + static_cast<std::ptrdiff_t>(length));
            const auto decoded = DecodeServerUpdate(partial, 0, anchor, Types());
            CHECK(!decoded.ok());
            CHECK(decoded.error == MapDecodeError::Truncated);
        }
    }

    CHECK(DecodeServerUpdate({}, 0, anchor, Types()).error == MapDecodeError::Truncated);
    CHECK(DecodeServerUpdate(vectors::HexBytes(vectors::kGoldenRowEastEmptyHex), 0, anchor,
                             ObjectTypeTable{}).error == MapDecodeError::EmptyObjectTypeTable);

    // An opcode this layer does not decode consumes nothing and is named.
    // Containers are out of scope for both this task and PLAYERSTATE-772-001.
    const auto unsupported = DecodeServerUpdate({110, 1, 2, 3, 4, 5, 6}, 0, anchor, Types());
    CHECK(unsupported.ok());
    CHECK(unsupported.update.kind == ServerUpdateKind::Unsupported);
    CHECK(unsupported.update.bytes_consumed == 0);
    CHECK(std::string(unsupported.update.name) == "SV_CMD_CONTAINER");
    CHECK(std::string(ServerUpdateKindName(ServerUpdateKind::Row)) == "Row");

    // Stack indexes at or beyond MAX_OBJECTS_PER_POINT cannot be produced by
    // SendChangeField, SendDeleteField or SendMoveCreature.
    auto bad = vectors::HexBytes(vectors::kGoldenMoveCreatureHex);
    bad[6] = 10;  // the origin stack index
    CHECK(DecodeServerUpdate(bad, 0, anchor, Types()).error
          == MapDecodeError::InvalidStackIndex);

    const TileMap empty;
    vectors::ServerEmitter remove(Provider(empty));
    remove.DeleteField({1000, 1000, 7}, 200);
    CHECK(DecodeServerUpdate(remove.bytes(), 0, anchor, Types()).error
          == MapDecodeError::InvalidStackIndex);

    // A row that would step off the addressable floors.
    auto row = vectors::HexBytes(vectors::kGoldenFloorUpEmptyHex);
    CHECK(DecodeServerUpdate(row, 0, {1000, 1000, 0}, Types()).error
          == MapDecodeError::InvalidPlayerFloor);
}

void TestApplicationAnomalies() {
    const std::uint32_t player_id = 21;
    const MapPosition player{1000, 1000, 7};
    const TileMap tiles = BuildWorld(player, player_id);
    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    const auto before = Snapshot(state);

    // A move whose origin does not hold a creature at that index.
    vectors::ServerEmitter move(Provider(tiles));
    move.MoveCreature({1002, 1002, 7}, 0, {1003, 1002, 7});
    auto decoded = DecodeServerUpdate(move.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    auto applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(!applied.clean());
    CHECK(applied.anomalies[0].kind == WorldStateAnomalyKind::MoveOriginMismatch);
    CHECK(Snapshot(state) == before);

    // A change addressing a stack index the tile does not hold.
    vectors::ServerEmitter change(Provider(tiles));
    change.ChangeField({1002, 1002, 7}, 9, vectors::PlainItem(101));
    decoded = DecodeServerUpdate(change.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(!applied.clean());
    CHECK(applied.anomalies[0].kind == WorldStateAnomalyKind::StackIndexOutOfRange);
    CHECK(Snapshot(state) == before);

    // A field far outside the viewport.
    vectors::ServerEmitter add(Provider(tiles));
    add.AddField({1500, 1500, 7}, vectors::PlainItem(101));
    decoded = DecodeServerUpdate(add.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(!applied.clean());
    CHECK(applied.anomalies[0].kind == WorldStateAnomalyKind::FieldOutsideViewport);
    CHECK(Snapshot(state) == before);

    CHECK(std::string(WorldStateAnomalyKindName(
              WorldStateAnomalyKind::ViewportDesynchronized)) == "ViewportDesynchronized");
    CHECK(ApplyServerUpdate(nullptr, decoded.update, Types()).clean());
}

// The second-client lifecycle the vertical slice depends on: another player
// appears in view, walks, disconnects and comes back, all while this client
// stays connected and stationary.
void TestSecondPlayerLifecycle() {
    const std::uint32_t self_id = 1002;
    const std::uint32_t other_id = 1001;  // a player keeps its CharacterID
    const MapPosition self{1000, 1000, 7};
    TileMap tiles = BuildWorld(self, self_id);

    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = self_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{self_id}));
    const MapPosition anchor_before = state.viewport_anchor;

    // 1. The other player logs in nearby. Create ends in
    // AnnounceChangedObject(OBJECT_CREATED), so this client gets ADD_FIELD
    // carrying a word-97 descriptor with the name.
    const MapPosition arrival{1002, 1000, 7};
    vectors::ServerEmitter appear(Provider(tiles));
    appear.AddField(arrival, vectors::IntroducedCreature(other_id, 0, "Test Player A"));
    auto decoded = DecodeServerUpdate(appear.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{other_id, self_id}));
    CHECK(state.known_creatures.at(other_id).name == "Test Player A");
    CHECK(state.known_creatures.at(other_id).position == arrival);
    CHECK(state.FindTile(arrival) != nullptr);

    // 2. The other player walks one field west, towards this client.
    const MapPosition stepped{1001, 1000, 7};
    const std::size_t stack = state.known_creatures.at(other_id).stack_position;
    vectors::ServerEmitter walk(Provider(tiles));
    walk.MoveCreature(arrival, static_cast<std::uint8_t>(stack), stepped);
    decoded = DecodeServerUpdate(walk.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.known_creatures.at(other_id).position == stepped);
    CHECK(state.FindTile(stepped) != nullptr);
    // The vacated field keeps its ground but no longer holds the creature.
    const MapTile* vacated = state.FindTile(arrival);
    CHECK(vacated != nullptr);
    for (const MapThing& thing : vacated->things) CHECK(thing.kind != MapThingKind::Creature);
    // This client did not move.
    CHECK(state.viewport_anchor == anchor_before);
    CHECK(state.viewport_synchronized());

    // 3. The other player disconnects. Delete ends in
    // AnnounceChangedObject(OBJECT_DELETED), so this client gets DELETE_FIELD.
    const std::size_t stack_after = state.known_creatures.at(other_id).stack_position;
    vectors::ServerEmitter leave(Provider(tiles));
    leave.DeleteField(stepped, static_cast<std::uint8_t>(stack_after));
    decoded = DecodeServerUpdate(leave.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    // No ghost on the map. The mirror keeps the entry, because this same
    // command also means "scrolled out of view" and the server's own table only
    // frees a slot in ~TCreature or when NewKnownCreature reuses it. Dropping it
    // would make a later word-98 or word-99 reappearance unrecognisable.
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{self_id}));
    CHECK(state.known_creatures.count(other_id) == 1);
    CHECK(state.FindTile(stepped) != nullptr);
    for (const MapThing& thing : state.FindTile(stepped)->things) {
        CHECK(thing.kind != MapThingKind::Creature);
    }

    // A creature that merely scrolled out of view and returns is announced with
    // a word-99 descriptor carrying no name, which the retained mirror entry
    // makes recognisable.
    vectors::ServerEmitter glimpse(Provider(tiles));
    glimpse.AddField(stepped, vectors::KnownCreature(other_id, 1));
    auto seen_again = DecodeServerUpdate(glimpse.bytes(), 0, state.viewport_anchor, Types());
    CHECK(seen_again.ok());
    CHECK(ApplyServerUpdate(&state, seen_again.update, Types()).clean());
    CHECK(state.known_creatures.at(other_id).name == "Test Player A");
    CHECK(state.known_creatures.at(other_id).position == stepped);

    // Put it back out of view for the relog step below.
    const std::size_t glimpsed_stack = state.known_creatures.at(other_id).stack_position;
    vectors::ServerEmitter leave_again(Provider(tiles));
    leave_again.DeleteField(stepped, static_cast<std::uint8_t>(glimpsed_stack));
    decoded = DecodeServerUpdate(leave_again.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{self_id}));

    // 4. The other player logs back in. ~TCreature marked this client's slot
    // free without clearing its creature id, so NewKnownCreature reuses that
    // very slot and the word-97 descriptor evicts the same id it introduces.
    // That must not be read as evicting an unknown creature.
    MapThing relog = vectors::IntroducedCreature(other_id, other_id, "Test Player A");
    vectors::ServerEmitter back(Provider(tiles));
    back.AddField(arrival, relog);
    decoded = DecodeServerUpdate(back.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    const auto applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(applied.clean());
    CHECK(state.visible_creature_ids() == (std::vector<std::uint32_t>{other_id, self_id}));
    CHECK(state.known_creatures.count(other_id) == 1);
    CHECK(state.known_creatures.at(other_id).position == arrival);
    CHECK(state.known_creatures.at(other_id).name == "Test Player A");
    CHECK(state.viewport_anchor == anchor_before);
    CHECK(state.viewport_synchronized());

    // A word-97 evicting a genuinely different creature is still reported.
    MapThing stranger = vectors::IntroducedCreature(1003, 999999, "Ghost");
    vectors::ServerEmitter evict(Provider(tiles));
    evict.AddField({1000, 1002, 7}, stranger);
    decoded = DecodeServerUpdate(evict.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    const auto evicted = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(!evicted.clean());
    CHECK(evicted.anomalies[0].kind == WorldStateAnomalyKind::UnknownEvictedCreature);
    CHECK(evicted.anomalies[0].creature_id == 999999);
}

void TestViewportDesynchronizationIsVisible() {
    const std::uint32_t player_id = 31;
    MapPosition player{1000, 1000, 7};
    TileMap tiles = BuildWorld(player, player_id);
    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    state.local_creature_id = player_id;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    CHECK(state.viewport_synchronized());

    // Applying the row without its paired creature move leaves the anchor ahead
    // of the player, which the state reports rather than hides.
    vectors::ServerEmitter row(Provider(tiles));
    row.Row(1001, 1000, 7, CardinalDirection::East);
    const auto decoded = DecodeServerUpdate(row.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.viewport_anchor == (MapPosition{1001, 1000, 7}));
    CHECK(!state.viewport_synchronized());
}

// ---------------------------------------------------------------- talk
//
// The emitter is asserted against hand-computed bytes before any structural
// test leans on it, the same way the rest of this suite treats its fixtures.

void TestGoldenTalk() {
    // SendTalk(..., StatementID=1, Sender="Bob", TALK_SAY, x=32097, y=32218,
    //          z=7, Text="hi")
    //
    //   AA           SV_CMD_TALK = 170
    //   01 00 00 00  StatementID, quad little-endian
    //   03 00 42 6F 62   Sender "Bob": word length 3, then 'B' 'o' 'b'
    //   01           TALK_SAY
    //   61 7D        x = 32097  (0x7D61)
    //   DA 7D        y = 32218  (0x7DDA)
    //   07           z
    //   02 00 68 69  Text "hi": word length 2, then 'h' 'i'
    vectors::ServerEmitter emitter(NoTiles());
    emitter.TalkPositional(1, "Bob", static_cast<std::uint8_t>(TalkMode::Say),
                           MapPosition{32097, 32218, 7}, "hi");
    const std::vector<std::uint8_t> expected{
        0xAA, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x42, 0x6F, 0x62,
        0x01, 0x61, 0x7D, 0xDA, 0x7D, 0x07, 0x02, 0x00, 0x68, 0x69};
    CHECK(emitter.bytes() == expected);

    // Channel form: no coordinates, a word channel instead.
    //   AA 07 00 00 00  StatementID 7
    //   03 00 41 6D 79  "Amy"
    //   05              TALK_CHANNEL_CALL
    //   04 00           channel 4
    //   02 00 79 6F     "yo"
    vectors::ServerEmitter channel(NoTiles());
    channel.TalkChannel(7, "Amy", static_cast<std::uint8_t>(TalkMode::ChannelCall),
                        4, "yo");
    const std::vector<std::uint8_t> expected_channel{
        0xAA, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x41, 0x6D, 0x79,
        0x05, 0x04, 0x00, 0x02, 0x00, 0x79, 0x6F};
    CHECK(channel.bytes() == expected_channel);

    // Plain form with the one conditional quad.
    //   AA 09 00 00 00  StatementID 9
    //   02 00 47 4D     "GM"
    //   06              TALK_GAMEMASTER_REQUEST
    //   2A 00 00 00     Data 42
    //   01 00 3F        "?"
    vectors::ServerEmitter request(NoTiles());
    request.TalkPlain(9, "GM", static_cast<std::uint8_t>(TalkMode::GamemasterRequest),
                      "?", 42);
    const std::vector<std::uint8_t> expected_request{
        0xAA, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x47, 0x4D,
        0x06, 0x2A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x3F};
    CHECK(request.bytes() == expected_request);
}

// Every mode each overload accepts, decoded into the semantics the caller sees.
// The assertions never mention an opcode.
void TestTalkFormsDecode() {
    const MapPosition anchor{32097, 32218, 7};
    const ObjectTypeTable& types = Types();

    struct PositionalCase {
        TalkMode mode;
    };
    for (const PositionalCase& c : {PositionalCase{TalkMode::Say},
                                    PositionalCase{TalkMode::Whisper},
                                    PositionalCase{TalkMode::Yell},
                                    PositionalCase{TalkMode::AnimalLow},
                                    PositionalCase{TalkMode::AnimalLoud}}) {
        vectors::ServerEmitter emitter(NoTiles());
        emitter.TalkPositional(11, "Speaker", static_cast<std::uint8_t>(c.mode),
                               MapPosition{32100, 32200, 6}, "text here");
        const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, anchor, types);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Talk);
        CHECK(decoded.update.talk.layout == TalkLayout::Positional);
        CHECK(decoded.update.talk.statement_id == 11);
        CHECK(decoded.update.talk.speaker == "Speaker");
        CHECK(decoded.update.talk.text == "text here");
        CHECK(decoded.update.talk.has_position);
        CHECK(decoded.update.talk.position.x == 32100);
        CHECK(decoded.update.talk.position.y == 32200);
        CHECK(decoded.update.talk.position.z == 6);
        CHECK(!decoded.update.talk.has_channel);
        CHECK(!decoded.update.talk.has_request_data);
        CHECK(decoded.update.bytes_consumed == emitter.bytes().size());
    }

    for (const TalkMode mode : {TalkMode::ChannelCall, TalkMode::GamemasterChannelCall,
                                TalkMode::HighlightChannelCall}) {
        vectors::ServerEmitter emitter(NoTiles());
        emitter.TalkChannel(12, "Caller", static_cast<std::uint8_t>(mode), 9, "in channel");
        const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, anchor, types);
        CHECK(decoded.ok());
        CHECK(decoded.update.talk.layout == TalkLayout::Channel);
        CHECK(decoded.update.talk.has_channel);
        CHECK(decoded.update.talk.channel == 9);
        CHECK(decoded.update.talk.speaker == "Caller");
        CHECK(!decoded.update.talk.has_position);
        CHECK(decoded.update.bytes_consumed == emitter.bytes().size());
    }

    // The anonymous channel mode carries an empty speaker by design, not by
    // omitting the field. Decoding must see a present-but-empty string.
    {
        vectors::ServerEmitter emitter(NoTiles());
        emitter.TalkChannel(13, "Hidden",
                            static_cast<std::uint8_t>(TalkMode::AnonymousChannelCall),
                            5, "anon");
        const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, anchor, types);
        CHECK(decoded.ok());
        CHECK(decoded.update.talk.layout == TalkLayout::Channel);
        CHECK(decoded.update.talk.speaker.empty());
        CHECK(decoded.update.talk.channel == 5);
        CHECK(decoded.update.talk.text == "anon");
        CHECK(decoded.update.bytes_consumed == emitter.bytes().size());
    }

    for (const TalkMode mode : {TalkMode::PrivateMessage, TalkMode::GamemasterAnswer,
                                TalkMode::PlayerAnswer, TalkMode::GamemasterBroadcast,
                                TalkMode::GamemasterMessage}) {
        vectors::ServerEmitter emitter(NoTiles());
        emitter.TalkPlain(14, "Sender", static_cast<std::uint8_t>(mode), "private words");
        const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, anchor, types);
        CHECK(decoded.ok());
        CHECK(decoded.update.talk.layout == TalkLayout::Plain);
        CHECK(!decoded.update.talk.has_position);
        CHECK(!decoded.update.talk.has_channel);
        // Only GAMEMASTER_REQUEST carries the quad, so these must not claim it.
        CHECK(!decoded.update.talk.has_request_data);
        CHECK(decoded.update.talk.text == "private words");
        CHECK(decoded.update.bytes_consumed == emitter.bytes().size());
    }

    {
        vectors::ServerEmitter emitter(NoTiles());
        emitter.TalkPlain(15, "GM",
                          static_cast<std::uint8_t>(TalkMode::GamemasterRequest),
                          "help me", 4242);
        const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, anchor, types);
        CHECK(decoded.ok());
        CHECK(decoded.update.talk.layout == TalkLayout::Plain);
        CHECK(decoded.update.talk.has_request_data);
        CHECK(decoded.update.talk.request_data == 4242);
        CHECK(decoded.update.talk.text == "help me");
        CHECK(decoded.update.bytes_consumed == emitter.bytes().size());
    }
}

void TestTalkEmptyAndLongText() {
    const MapPosition anchor{32097, 32218, 7};
    const ObjectTypeTable& types = Types();

    // An empty text is a zero-length string, not an absent field.
    vectors::ServerEmitter empty(NoTiles());
    empty.TalkPositional(1, "A", static_cast<std::uint8_t>(TalkMode::Say),
                         MapPosition{32097, 32218, 7}, "");
    const auto decoded_empty = DecodeServerUpdate(empty.bytes(), 0, anchor, types);
    CHECK(decoded_empty.ok());
    CHECK(decoded_empty.update.talk.text.empty());
    CHECK(decoded_empty.update.bytes_consumed == empty.bytes().size());

    // An empty speaker too, which is what the ANIMAL modes actually send:
    // moveuse.cc passes "" as the sender.
    vectors::ServerEmitter animal(NoTiles());
    animal.TalkPositional(0, "", static_cast<std::uint8_t>(TalkMode::AnimalLow),
                          MapPosition{32097, 32218, 7}, "Zzzzzzt");
    const auto decoded_animal = DecodeServerUpdate(animal.bytes(), 0, anchor, types);
    CHECK(decoded_animal.ok());
    CHECK(decoded_animal.update.talk.speaker.empty());
    CHECK(decoded_animal.update.talk.text == "Zzzzzzt");
    CHECK(decoded_animal.update.bytes_consumed == animal.bytes().size());

    const std::string long_text(600, 'x');
    vectors::ServerEmitter longer(NoTiles());
    longer.TalkPositional(2, "A", static_cast<std::uint8_t>(TalkMode::Yell),
                          MapPosition{32097, 32218, 7}, long_text);
    const auto decoded_long = DecodeServerUpdate(longer.bytes(), 0, anchor, types);
    CHECK(decoded_long.ok());
    CHECK(decoded_long.update.talk.text.size() == 600);
    CHECK(decoded_long.update.bytes_consumed == longer.bytes().size());
}

// The case that matters most for stream health: a talk command must consume
// exactly its own bytes, leaving the next command correctly aligned.
void TestTalkKeepsTheStreamAligned() {
    const MapPosition anchor{32097, 32218, 7};
    const ObjectTypeTable& types = Types();

    // One of each form, then an ordinary world command.
    vectors::ServerEmitter emitter(NoTiles());
    emitter.TalkPositional(1, "Bob", static_cast<std::uint8_t>(TalkMode::Say),
                           MapPosition{32097, 32218, 7}, "hello");
    emitter.TalkChannel(2, "Amy", static_cast<std::uint8_t>(TalkMode::ChannelCall),
                        3, "channel text");
    emitter.TalkPlain(3, "GM", static_cast<std::uint8_t>(TalkMode::GamemasterRequest),
                      "req", 77);
    emitter.MoveCreature(MapPosition{32097, 32218, 7}, 1, MapPosition{32098, 32218, 7});
    emitter.Ping();

    std::size_t at = 0;
    const auto first = DecodeServerUpdate(emitter.bytes(), at, anchor, types);
    CHECK(first.ok() && first.update.kind == ServerUpdateKind::Talk);
    CHECK(first.update.talk.has_position);
    at += first.update.bytes_consumed;

    const auto second = DecodeServerUpdate(emitter.bytes(), at, anchor, types);
    CHECK(second.ok() && second.update.kind == ServerUpdateKind::Talk);
    CHECK(second.update.talk.channel == 3);
    at += second.update.bytes_consumed;

    const auto third = DecodeServerUpdate(emitter.bytes(), at, anchor, types);
    CHECK(third.ok() && third.update.kind == ServerUpdateKind::Talk);
    CHECK(third.update.talk.request_data == 77);
    at += third.update.bytes_consumed;

    // The world command after three talks must decode normally.
    const auto move = DecodeServerUpdate(emitter.bytes(), at, anchor, types);
    CHECK(move.ok());
    CHECK(move.update.kind == ServerUpdateKind::MoveCreature);
    CHECK(move.update.move_creature.destination.x == 32098);
    at += move.update.bytes_consumed;

    const auto ping = DecodeServerUpdate(emitter.bytes(), at, anchor, types);
    CHECK(ping.ok() && ping.update.kind == ServerUpdateKind::Ping);
    at += ping.update.bytes_consumed;

    // Exactly consumed: no residual bytes.
    CHECK(at == emitter.bytes().size());
}

void TestTalkNegativeCases() {
    const MapPosition anchor{32097, 32218, 7};
    const ObjectTypeTable& types = Types();

    vectors::ServerEmitter emitter(NoTiles());
    emitter.TalkPositional(1, "Bob", static_cast<std::uint8_t>(TalkMode::Say),
                           MapPosition{32097, 32218, 7}, "hello");
    const std::vector<std::uint8_t> complete = emitter.bytes();

    // Every truncation must fail rather than over-read, at every length.
    for (std::size_t length = 1; length < complete.size(); ++length) {
        const std::vector<std::uint8_t> partial(complete.begin(),
                                                complete.begin() + static_cast<long>(length));
        const auto decoded = DecodeServerUpdate(partial, 0, anchor, types);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::Truncated);
    }

    // A mode no SendTalk overload accepts. ANONYMOUS_BROADCAST (13) and
    // ANONYMOUS_MESSAGE (15) are declared in enums.hh but unreachable, so the
    // tail is unknown and guessing one would desynchronise the stream.
    for (const std::uint8_t mode : std::vector<std::uint8_t>{0, 13, 15, 18, 200}) {
        vectors::ServerEmitter bad(NoTiles());
        bad.TalkPlain(1, "Bob", mode, "text");
        const auto decoded = DecodeServerUpdate(bad.bytes(), 0, anchor, types);
        CHECK(!decoded.ok());
        CHECK(decoded.error == MapDecodeError::UnknownTalkMode);
        CHECK(decoded.update.bytes_consumed == 0);
    }

    // A text whose declared length runs past the buffer.
    std::vector<std::uint8_t> overlong = complete;
    overlong[overlong.size() - 7] = 0xFF;
    overlong[overlong.size() - 6] = 0xFF;
    const auto decoded_overlong = DecodeServerUpdate(overlong, 0, anchor, types);
    CHECK(!decoded_overlong.ok());
    CHECK(decoded_overlong.error == MapDecodeError::Truncated);
}

// The layout classification is source truth in table form, so assert it
// directly rather than only through decoded packets.
void TestTalkLayoutClassification() {
    CHECK(TalkLayoutForMode(1) == TalkLayout::Positional);
    CHECK(TalkLayoutForMode(2) == TalkLayout::Positional);
    CHECK(TalkLayoutForMode(3) == TalkLayout::Positional);
    CHECK(TalkLayoutForMode(16) == TalkLayout::Positional);
    CHECK(TalkLayoutForMode(17) == TalkLayout::Positional);

    CHECK(TalkLayoutForMode(5) == TalkLayout::Channel);
    CHECK(TalkLayoutForMode(10) == TalkLayout::Channel);
    CHECK(TalkLayoutForMode(12) == TalkLayout::Channel);
    CHECK(TalkLayoutForMode(14) == TalkLayout::Channel);

    CHECK(TalkLayoutForMode(4) == TalkLayout::Plain);
    CHECK(TalkLayoutForMode(6) == TalkLayout::Plain);
    CHECK(TalkLayoutForMode(7) == TalkLayout::Plain);
    CHECK(TalkLayoutForMode(8) == TalkLayout::Plain);
    CHECK(TalkLayoutForMode(9) == TalkLayout::Plain);
    CHECK(TalkLayoutForMode(11) == TalkLayout::Plain);

    // Declared in enums.hh, accepted by no overload.
    CHECK(TalkLayoutForMode(13) == TalkLayout::Unsupported);
    CHECK(TalkLayoutForMode(15) == TalkLayout::Unsupported);
    // SendMessage's modes travel under a different opcode entirely.
    CHECK(TalkLayoutForMode(18) == TalkLayout::Unsupported);
    CHECK(TalkLayoutForMode(23) == TalkLayout::Unsupported);

    // SendMessage's modes are named by their own function: asking TalkModeName
    // about them would answer "UnknownTalkMode" about a perfectly known mode.
    CHECK(std::string(MessageModeName(20)) == "LoginMessage");
    CHECK(std::string(MessageModeName(23)) == "FailureMessage");
    CHECK(std::string(MessageModeName(1)) == "UnknownMessageMode");
    CHECK(TalkLayoutForMode(0) == TalkLayout::Unsupported);
}

// Talk carries no world state, so applying it must leave WorldState untouched.
void TestTalkDoesNotTouchWorldState() {
    // A real world, built the way the rest of this suite builds one.
    TileMap tiles;
    tiles[{1000, 1000, 7}] = {vectors::PlainItem(100)};
    tiles[{1001, 1000, 7}] = {vectors::PlainItem(101)};
    vectors::ServerEmitter initial(Provider(tiles));
    initial.FullScreen(1000, 1000, 7);
    const auto screen = DecodeFullScreen(initial.bytes(), Types());
    CHECK(screen.ok());

    WorldState state;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    const std::size_t tiles_before = state.tile_count();
    const MapPosition anchor_before = state.viewport_anchor;
    const std::size_t creatures_before = state.visible_creature_ids().size();

    vectors::ServerEmitter emitter(NoTiles());
    emitter.TalkPositional(1, "Bob", static_cast<std::uint8_t>(TalkMode::Say),
                           MapPosition{1000, 1000, 7}, "hello");
    const auto decoded = DecodeServerUpdate(emitter.bytes(), 0, state.viewport_anchor, Types());
    CHECK(decoded.ok());
    const auto applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(applied.anomalies.empty());

    CHECK(state.tile_count() == tiles_before);
    CHECK(state.visible_creature_ids().size() == creatures_before);
    CHECK(state.viewport_anchor.x == anchor_before.x);
    CHECK(state.viewport_anchor.y == anchor_before.y);
    CHECK(state.viewport_anchor.z == anchor_before.z);
}

// ------------------------------------------------------ outgoing talk

// Golden bytes for the ordinary case, hand-computed from CTalk's reads.
void TestGoldenSayCommand() {
    //   96        CL_CMD_TALK = 150
    //   01        TALK_SAY
    //   04 00     text length 4
    //   68 6F 6C 61   "hola"
    const auto say = BuildSayCommand("hola");
    CHECK(say.ok());
    const std::vector<std::uint8_t> expected{0x96, 0x01, 0x04, 0x00, 0x68, 0x6F, 0x6C, 0x61};
    CHECK(say.payload == expected);

    // An addressed mode inserts the addressee between mode and text.
    //   96 04     CL_CMD_TALK, TALK_PRIVATE_MESSAGE
    //   01 00 42  "B"
    //   02 00 68 69  "hi"
    const auto whisper_to = BuildTalkCommand(
        static_cast<std::uint8_t>(TalkMode::PrivateMessage), "hi", "B");
    CHECK(whisper_to.ok());
    const std::vector<std::uint8_t> expected_pm{
        0x96, 0x04, 0x01, 0x00, 0x42, 0x02, 0x00, 0x68, 0x69};
    CHECK(whisper_to.payload == expected_pm);

    // A channel mode inserts a word instead.
    //   96 05     CL_CMD_TALK, TALK_CHANNEL_CALL
    //   03 00     channel 3
    //   02 00 79 6F  "yo"
    const auto channel = BuildTalkCommand(
        static_cast<std::uint8_t>(TalkMode::ChannelCall), "yo", "", 3);
    CHECK(channel.ok());
    const std::vector<std::uint8_t> expected_channel{
        0x96, 0x05, 0x03, 0x00, 0x02, 0x00, 0x79, 0x6F};
    CHECK(channel.payload == expected_channel);
}

// The client's accepted mode set is NOT the server's, and the difference runs
// both ways. Getting this wrong by reusing the incoming table would send modes
// the server discards and refuse modes it accepts.
void TestClientTalkModeSetDiffersFromTheServers() {
    // Accepted by CTalk and emitted by SendTalk: the common ground.
    for (const std::uint8_t mode : std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14}) {
        CHECK(IsClientTalkMode(mode));
    }

    // Accepted by CTalk, emitted by no SendTalk overload. Send-only: a client
    // may say these and can never be told them.
    CHECK(IsClientTalkMode(13));   // TALK_ANONYMOUS_BROADCAST
    CHECK(IsClientTalkMode(15));   // TALK_ANONYMOUS_MESSAGE
    CHECK(TalkLayoutForMode(13) == TalkLayout::Unsupported);
    CHECK(TalkLayoutForMode(15) == TalkLayout::Unsupported);

    // Emitted by SendTalk, rejected by CTalk. Receive-only: the server may tell
    // a client these and a client may never send them.
    CHECK(!IsClientTalkMode(12));  // TALK_HIGHLIGHT_CHANNELCALL
    CHECK(!IsClientTalkMode(16));  // TALK_ANIMAL_LOW
    CHECK(!IsClientTalkMode(17));  // TALK_ANIMAL_LOUD
    CHECK(TalkLayoutForMode(12) == TalkLayout::Channel);
    CHECK(TalkLayoutForMode(16) == TalkLayout::Positional);
    CHECK(TalkLayoutForMode(17) == TalkLayout::Positional);

    // Neither: SendMessage's modes and plain nonsense.
    CHECK(!IsClientTalkMode(0));
    CHECK(!IsClientTalkMode(18));
    CHECK(!IsClientTalkMode(23));
    CHECK(!IsClientTalkMode(200));
}

void TestOutgoingTalkFieldSelection() {
    // Addressee only for the four addressed modes.
    CHECK(TalkModeNeedsAddressee(4));
    CHECK(TalkModeNeedsAddressee(7));
    CHECK(TalkModeNeedsAddressee(11));
    CHECK(TalkModeNeedsAddressee(15));
    CHECK(!TalkModeNeedsAddressee(1));
    CHECK(!TalkModeNeedsAddressee(5));

    // Channel only for the three channel modes.
    CHECK(TalkModeNeedsChannel(5));
    CHECK(TalkModeNeedsChannel(10));
    CHECK(TalkModeNeedsChannel(14));
    CHECK(!TalkModeNeedsChannel(1));
    CHECK(!TalkModeNeedsChannel(4));

    // A field the mode does not carry must not reach the wire, however
    // insistently the caller supplies it.
    const auto say = BuildTalkCommand(static_cast<std::uint8_t>(TalkMode::Say),
                                      "hola", "ignored", 99);
    CHECK(say.ok());
    CHECK(say.payload == BuildSayCommand("hola").payload);
}

// Each refusal mirrors a check CTalk performs, so the player is told rather
// than the server silently discarding the command.
void TestOutgoingTalkRefusals() {
    CHECK(BuildSayCommand("").error == TalkBuildError::EmptyText);
    CHECK(BuildSayCommand(std::string(kMaxTalkTextLength + 1, 'x')).error
          == TalkBuildError::TextTooLong);
    CHECK(BuildSayCommand("two\nlines").error == TalkBuildError::TextContainsNewline);
    CHECK(BuildTalkCommand(12, "x").error == TalkBuildError::UnsupportedMode);

    // Exactly at the limit is fine: char Text[256] holds 255 plus a terminator.
    const auto longest = BuildSayCommand(std::string(kMaxTalkTextLength, 'x'));
    CHECK(longest.ok());
    CHECK(longest.payload.size() == 2 + 2 + kMaxTalkTextLength);

    // An addressed mode without an addressee is refused, and the limit is the
    // server's char Addressee[30] minus its terminator.
    const std::uint8_t pm = static_cast<std::uint8_t>(TalkMode::PrivateMessage);
    CHECK(BuildTalkCommand(pm, "hi").error == TalkBuildError::MissingAddressee);
    CHECK(BuildTalkCommand(pm, "hi", std::string(kMaxTalkAddresseeLength + 1, 'n')).error
          == TalkBuildError::AddresseeTooLong);
    CHECK(BuildTalkCommand(pm, "hi", std::string(kMaxTalkAddresseeLength, 'n')).ok());

    // A refused command yields no bytes at all: nothing half-built escapes.
    CHECK(BuildSayCommand("").payload.empty());
}

// ------------------------------------------------- talk speaker resolution

namespace {

// One creature to place, kept small so the tests read as scenarios.
struct PlacedCreature {
    std::uint32_t id = 0;
    std::string name;
    MapPosition position;
};

PlacedCreature Speaker(std::uint32_t id, const std::string& name,
                       const MapPosition& at) {
    return PlacedCreature{id, name, at};
}

// Builds a world holding the given creatures by emitting a real FULLSCREEN and
// decoding it, so resolution is tested against WorldState as the client
// actually builds it rather than against a hand-made stub.
WorldState WorldWithCreatures(const std::vector<PlacedCreature>& creatures) {
    TileMap tiles;
    for (const PlacedCreature& creature : creatures) {
        tiles[creature.position] = {
            vectors::PlainItem(100),
            vectors::IntroducedCreature(creature.id, 0, creature.name)};
    }
    vectors::ServerEmitter emitter(Provider(tiles));
    const MapPosition& anchor = creatures.front().position;
    emitter.FullScreen(static_cast<std::uint16_t>(anchor.x),
                       static_cast<std::uint16_t>(anchor.y),
                       static_cast<std::uint8_t>(anchor.z));
    const auto screen = DecodeFullScreen(emitter.bytes(), Types());
    CHECK(screen.ok());
    WorldState state;
    CHECK(ApplyFullScreen(&state, screen.message).clean());
    return state;
}

TalkUpdate PositionalTalk(const std::string& speaker, const MapPosition& at,
                          const std::string& text) {
    TalkUpdate talk;
    talk.mode = static_cast<std::uint8_t>(TalkMode::Say);
    talk.layout = TalkLayout::Positional;
    talk.speaker = speaker;
    talk.has_position = true;
    talk.position = at;
    talk.text = text;
    return talk;
}

}  // namespace

void TestTalkSpeakerResolvesByNameAndPosition() {
    const MapPosition here{1000, 1000, 7};
    const MapPosition there{1001, 1000, 7};
    const WorldState state = WorldWithCreatures({
        Speaker(1001, "Test Player A", here),
        Speaker(1002, "Test Player B", there),
    });

    const auto resolved = ResolveTalkSpeaker(state, PositionalTalk("Test Player A", here, "hola"));
    CHECK(resolved.outcome == TalkSpeakerOutcome::Resolved);
    CHECK(resolved.creature_id == 1001);
    CHECK(resolved.candidates == 1);

    // The other creature, at its own field.
    const auto other = ResolveTalkSpeaker(state, PositionalTalk("Test Player B", there, "hi"));
    CHECK(other.outcome == TalkSpeakerOutcome::Resolved);
    CHECK(other.creature_id == 1002);
}

// The failure that matters: speech must never appear above the wrong creature.
void TestTalkSpeakerNeverPicksTheWrongCreature() {
    const MapPosition here{1000, 1000, 7};
    const MapPosition there{1001, 1000, 7};
    const WorldState state = WorldWithCreatures({
        Speaker(1001, "Test Player A", here),
        Speaker(1002, "Test Player B", there),
    });

    // Right name, wrong position: the name alone must not be enough.
    const auto moved = ResolveTalkSpeaker(state, PositionalTalk("Test Player A", there, "x"));
    CHECK(moved.outcome == TalkSpeakerOutcome::NoMatch);
    CHECK(moved.creature_id == 0);

    // Right position, wrong name: the position alone must not be enough when a
    // name is present on both sides.
    const auto renamed = ResolveTalkSpeaker(state, PositionalTalk("Somebody Else", here, "x"));
    CHECK(renamed.outcome == TalkSpeakerOutcome::NoMatch);
    CHECK(renamed.creature_id == 0);

    // A field nobody occupies.
    const auto empty = ResolveTalkSpeaker(
        state, PositionalTalk("Test Player A", MapPosition{1005, 1005, 7}, "x"));
    CHECK(empty.outcome == TalkSpeakerOutcome::NoMatch);
}

// The ANIMAL modes send an empty sender: moveuse.cc passes "". Position alone
// must then be allowed to decide.
void TestTalkSpeakerAcceptsAnEmptySenderByPosition() {
    const MapPosition here{1000, 1000, 7};
    const WorldState state = WorldWithCreatures({Speaker(1001, "a snake", here)});

    TalkUpdate animal = PositionalTalk("", here, "Zzzzzzt");
    animal.mode = static_cast<std::uint8_t>(TalkMode::AnimalLow);
    const auto resolved = ResolveTalkSpeaker(state, animal);
    CHECK(resolved.outcome == TalkSpeakerOutcome::Resolved);
    CHECK(resolved.creature_id == 1001);
}

void TestTalkSpeakerIgnoresNonPositionalForms() {
    const WorldState state = WorldWithCreatures({Speaker(1001, "A", MapPosition{1000, 1000, 7})});

    TalkUpdate channel;
    channel.mode = static_cast<std::uint8_t>(TalkMode::ChannelCall);
    channel.layout = TalkLayout::Channel;
    channel.speaker = "A";
    channel.has_channel = true;
    channel.channel = 3;
    channel.text = "in a channel";
    const auto resolved = ResolveTalkSpeaker(state, channel);
    CHECK(resolved.outcome == TalkSpeakerOutcome::NotPositional);
    CHECK(resolved.creature_id == 0);
}

// A creature that has scrolled out of view stays in the known-creature mirror
// but has no actor to speak above, so it must not resolve.
void TestTalkSpeakerIgnoresCreaturesThatAreNotVisible() {
    const MapPosition here{1000, 1000, 7};
    WorldState state = WorldWithCreatures({Speaker(1001, "Test Player A", here)});

    // Present in the mirror, absent from the map: exactly the state the mirror
    // is designed to hold after a creature scrolls out.
    CreatureRecord gone;
    gone.creature_id = 1077;
    gone.has_descriptor = true;
    gone.name = "Far Away";
    gone.position = MapPosition{1200, 1200, 7};
    state.known_creatures[gone.creature_id] = gone;

    const auto resolved = ResolveTalkSpeaker(
        state, PositionalTalk("Far Away", MapPosition{1200, 1200, 7}, "x"));
    CHECK(resolved.outcome == TalkSpeakerOutcome::NoMatch);
    CHECK(resolved.candidates == 0);
}

// A step this client asked for is counted as ours only when the server puts us
// on the field we asked for. Everything else is external.
void TestLedgerAcceptsOnlyTheStepThatWasAskedFor() {
    MovementLedger ledger;
    const MapPosition start{32097, 32218, 7};

    const std::uint32_t north = ledger.NoteRequestSent(CardinalDirection::North, 1.0);
    CHECK(north == 1);
    CHECK(ledger.outstanding() == 1);

    const auto accepted = ledger.NoteLocalMove(start, StepPosition(start, CardinalDirection::North), 1.2);
    CHECK(accepted.cause == LocalMoveCause::SelfWalkAccepted);
    CHECK(accepted.request_id == north);
    CHECK(accepted.direction == CardinalDirection::North);
    CHECK(ledger.outstanding() == 0);
    CHECK(ledger.counts().requested == 1);
    CHECK(ledger.counts().accepted == 1);
    CHECK(ledger.counts().external == 0);
}

// The regression this whole ledger exists for.
//
// reference/game/src/cract.cc TCreature::Move displaces whoever stands on the
// destination field, so another player walking into this one pushes them. A
// live session recorded eight such moves against zero requests, and the counter
// then in use called them accepted steps. It must be impossible to reach that
// conclusion again.
void TestLedgerNeverCountsAPushAsOurOwnStep() {
    MovementLedger ledger;
    MapPosition at{32098, 32216, 7};

    // Nothing was ever requested; another player pushes us eight fields north.
    for (int i = 0; i < 8; ++i) {
        const MapPosition pushed{at.x, at.y - 1, at.z};
        const auto outcome = ledger.NoteLocalMove(at, pushed, 1.0 + i);
        CHECK(outcome.cause == LocalMoveCause::ExternalRelocation);
        CHECK(outcome.request_id == 0);
        at = pushed;
    }

    CHECK(ledger.counts().requested == 0);
    CHECK(ledger.counts().accepted == 0);
    CHECK(ledger.counts().rejected == 0);
    CHECK(ledger.counts().external == 8);
}

// A push while a walk is outstanding must not consume that walk: the request is
// still unanswered, and the displacement is still not ours.
void TestLedgerPushDoesNotConsumeAnOutstandingWalk() {
    MovementLedger ledger;
    const MapPosition at{32100, 32200, 7};

    ledger.NoteRequestSent(CardinalDirection::East, 1.0);
    // Pushed west while waiting for the answer to an eastward walk.
    const auto pushed = ledger.NoteLocalMove(at, MapPosition{at.x - 1, at.y, at.z}, 1.1);
    CHECK(pushed.cause == LocalMoveCause::ExternalRelocation);
    CHECK(ledger.outstanding() == 1);
    CHECK(ledger.counts().accepted == 0);
    CHECK(ledger.counts().external == 1);

    // The walk is then answered from the field we were pushed to.
    const MapPosition after{at.x - 1, at.y, at.z};
    const auto accepted = ledger.NoteLocalMove(after, StepPosition(after, CardinalDirection::East), 1.4);
    CHECK(accepted.cause == LocalMoveCause::SelfWalkAccepted);
    CHECK(ledger.counts().accepted == 1);
    CHECK(ledger.counts().external == 1);
}

// Observed live: after refusing a step the server re-announces the position,
// which arrives as a move from a field to itself. Counting that as an external
// relocation inflated a number the evidence relies on.
void TestLedgerIgnoresAMoveThatDoesNotMove() {
    MovementLedger ledger;
    const MapPosition at{32096, 32200, 7};

    const auto still = ledger.NoteLocalMove(at, at, 1.0);
    CHECK(still.cause == LocalMoveCause::NoMovement);
    CHECK(ledger.counts().external == 0);
    CHECK(ledger.counts().accepted == 0);

    // And it must not consume an outstanding request either.
    ledger.NoteRequestSent(CardinalDirection::North, 2.0);
    CHECK(ledger.NoteLocalMove(at, at, 2.1).cause == LocalMoveCause::NoMovement);
    CHECK(ledger.outstanding() == 1);
    CHECK(ledger.counts().external == 0);
}

void TestLedgerRejectionAndExpiry() {
    MovementLedger ledger;

    const std::uint32_t blocked = ledger.NoteRequestSent(CardinalDirection::South, 1.0);
    const auto refused = ledger.NoteSnapback(1.3);
    CHECK(refused.matched);
    CHECK(refused.request_id == blocked);
    // The refusal reports the direction that was actually asked for, so a
    // rejection cannot be written down against a direction nobody requested.
    CHECK(refused.direction == CardinalDirection::South);
    CHECK(ledger.counts().rejected == 1);
    CHECK(ledger.counts().accepted == 0);
    CHECK(ledger.outstanding() == 0);

    // A snapback with nothing outstanding is still counted, and reports no
    // request, because inventing one would be worse than admitting surprise.
    const auto unmatched = ledger.NoteSnapback(1.5);
    CHECK(!unmatched.matched);
    CHECK(unmatched.request_id == 0);
    CHECK(ledger.counts().rejected == 2);

    // An answer that never arrives must not mis-attribute a later move.
    ledger.NoteRequestSent(CardinalDirection::West, 2.0);
    CHECK(ledger.ExpireBefore(5.0) == 1);
    CHECK(ledger.counts().unanswered == 1);
    CHECK(ledger.outstanding() == 0);

    const MapPosition at{32100, 32200, 7};
    const auto later = ledger.NoteLocalMove(at, MapPosition{at.x - 1, at.y, at.z}, 6.0);
    CHECK(later.cause == LocalMoveCause::ExternalRelocation);
}

// Ordering: the server answers a connection's commands in order, so the oldest
// outstanding request is the only candidate.
void TestLedgerMatchesRequestsInOrder() {
    MovementLedger ledger;
    MapPosition at{32100, 32200, 7};

    const std::uint32_t first = ledger.NoteRequestSent(CardinalDirection::North, 1.0);
    const std::uint32_t second = ledger.NoteRequestSent(CardinalDirection::North, 1.1);
    CHECK(second == first + 1);

    for (const std::uint32_t expected : {first, second}) {
        const MapPosition next = StepPosition(at, CardinalDirection::North);
        const auto outcome = ledger.NoteLocalMove(at, next, 1.5);
        CHECK(outcome.cause == LocalMoveCause::SelfWalkAccepted);
        CHECK(outcome.request_id == expected);
        at = next;
    }
    CHECK(ledger.counts().accepted == 2);
    CHECK(ledger.counts().external == 0);
}

}  // namespace

int main() {
    try {
        TestClientCommands();
        TestGoldenRows();
        TestRowCoversTheRevealedEdgeOnly();
        TestGoldenFloorChanges();
        TestGoldenMoveCreature();
        TestGoldenRejectedStep();
        TestStackPriority();
        TestWalkMatchesFreshFullScreen();
        TestKnownCreatureMirrorRetainsScrolledOutCreatures();
        TestSecondPlayerLifecycle();
        TestPlayerStackPositionFollowsPriority();
        TestRejectedStepLeavesWorldUntouched();
        TestFloorChangeKeepsOnlyVisibleFloors();
        TestFieldCommands();
        TestDeleteFieldRemovesCreature();
        TestNegativeCases();
        TestApplicationAnomalies();
        TestViewportDesynchronizationIsVisible();
        TestGoldenTalk();
        TestTalkFormsDecode();
        TestTalkEmptyAndLongText();
        TestTalkKeepsTheStreamAligned();
        TestTalkNegativeCases();
        TestTalkLayoutClassification();
        TestTalkDoesNotTouchWorldState();
        TestGoldenSayCommand();
        TestClientTalkModeSetDiffersFromTheServers();
        TestOutgoingTalkFieldSelection();
        TestOutgoingTalkRefusals();
        TestTalkSpeakerResolvesByNameAndPosition();
        TestTalkSpeakerNeverPicksTheWrongCreature();
        TestTalkSpeakerAcceptsAnEmptySenderByPosition();
        TestTalkSpeakerIgnoresNonPositionalForms();
        TestTalkSpeakerIgnoresCreaturesThatAreNotVisible();
        TestLedgerAcceptsOnlyTheStepThatWasAskedFor();
        TestLedgerNeverCountsAPushAsOurOwnStep();
        TestLedgerPushDoesNotConsumeAnOutstandingWalk();
        TestLedgerIgnoresAMoveThatDoesNotMove();
        TestLedgerRejectionAndExpiry();
        TestLedgerMatchesRequestsInOrder();
        std::cout << "protocol772_movement_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_movement_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
