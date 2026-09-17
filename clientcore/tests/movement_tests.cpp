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
    CHECK(state.known_creatures.count(other_id) == 0);
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
    const auto unsupported = DecodeServerUpdate({141, 1, 2, 3, 4, 5, 6}, 0, anchor, Types());
    CHECK(unsupported.ok());
    CHECK(unsupported.update.kind == ServerUpdateKind::Unsupported);
    CHECK(unsupported.update.bytes_consumed == 0);
    CHECK(std::string(unsupported.update.name) == "SV_CMD_CREATURE_LIGHT");
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
        TestPlayerStackPositionFollowsPriority();
        TestRejectedStepLeavesWorldUntouched();
        TestFloorChangeKeepsOnlyVisibleFloors();
        TestFieldCommands();
        TestDeleteFieldRemovesCreature();
        TestNegativeCases();
        TestApplicationAnomalies();
        TestViewportDesynchronizationIsVisible();
        std::cout << "protocol772_movement_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_movement_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
