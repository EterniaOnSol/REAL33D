#include "fusion32/protocol772/initial_world.h"

#include "fusion32/protocol772/gamelogin.h"

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

std::vector<std::uint8_t> Encode(std::uint16_t x, std::uint16_t y, std::uint8_t z,
                                 const TileMap& tiles) {
    return vectors::FullScreenEncoder::Encode(x, y, z, Provider(tiles));
}

// ---------------------------------------------------------------- object types

void TestObjectTypeTable() {
    ObjectTypeTable table;
    ObjectTypeEncoding plain;
    CHECK(table.empty());
    CHECK(table.Declare(100, plain));
    CHECK(!table.Declare(100, plain));  // duplicate
    CHECK(!table.Declare(kSkipMarkerBase, plain));
    CHECK(!table.Declare(0xFFFF, plain));
    CHECK(table.declared() == 1);
    CHECK(table.Lookup(100).known);
    CHECK(!table.Lookup(101).known);
    CHECK(table.Lookup(0xFFFE).known == false);

    ObjectTypeEncoding liquid;
    liquid.liquid_color = true;
    CHECK(liquid.extra_bytes() == 1);
    ObjectTypeEncoding cumulative;
    cumulative.cumulative = true;
    CHECK(cumulative.extra_bytes() == 1);
    CHECK(plain.extra_bytes() == 0);
}

void TestObjectsSrvLoader() {
    const std::string text =
        "# Tibia - graphical Multi-User-Dungeon\n"
        "TypeID      = 0 # map container\n"
        "Name        = \"\"\n"
        "Flags       = {Container}\n"
        "\n"
        "TypeID      = 100\n"
        "Name        = \"void\"\n"
        "Flags       = {Bank,Unmove}\n"
        "\n"
        "TypeID      = 200\n"
        "Flags       = {Cumulative,Take,Throw}\n"
        "Attributes  = {Weight=10}\n"
        "\n"
        "TypeID      = 300\n"
        "Flags       = {MultiUse,LiquidContainer,Take}\n"
        "\n"
        "TypeID      = 400\n"
        "Flags       = {LiquidPool,Unmove}\n"
        "\n"
        "TypeID      = 500\n"
        "Name        = \"a wall\"\n"
        "Flags       = {Bottom,Unpass,Unmove,Unlay}\n"
        "\n"
        "TypeID      = 600\n"
        "Name        = \"a walkable border\"\n"
        "Flags       = {Clip,Unmove}\n";
    const auto loaded = LoadObjectTypeTableFromObjectsSrv(text);
    CHECK(loaded.ok());
    CHECK(loaded.table.declared() == 7);
    CHECK(loaded.table.max_type_id() == 600);
    CHECK(loaded.table.Lookup(0).known && !loaded.table.Lookup(0).cumulative);
    CHECK(loaded.table.Lookup(100).known && loaded.table.Lookup(100).extra_bytes() == 0);
    CHECK(loaded.table.Lookup(200).cumulative && !loaded.table.Lookup(200).liquid_color);
    CHECK(loaded.table.Lookup(300).liquid_color && !loaded.table.Lookup(300).cumulative);
    CHECK(loaded.table.Lookup(400).liquid_color);
    CHECK(!loaded.table.Lookup(700).known);

    // UNPASS is read, and is independent of stack priority: a Bottom object
    // carries it while a Clip object does not, which is exactly why priority
    // must not be used as a stand-in for passability.
    CHECK(loaded.table.Lookup(500).unpass);
    CHECK(loaded.table.Lookup(500).priority == ObjectPriority::Bottom);
    CHECK(!loaded.table.Lookup(600).unpass);
    CHECK(loaded.table.Lookup(600).priority == ObjectPriority::Clip);
    // It must not disturb what the wire format actually depends on.
    CHECK(loaded.table.Lookup(500).extra_bytes() == 0);
    CHECK(!loaded.table.Lookup(100).unpass);

    // A `#` inside a quoted name must not truncate the record.
    const auto hashed = LoadObjectTypeTableFromObjectsSrv(
        "TypeID = 101\nName = \"a # b\"\nFlags = {Cumulative}\n");
    CHECK(hashed.ok());
    CHECK(hashed.table.Lookup(101).cumulative);

    const auto orphan = LoadObjectTypeTableFromObjectsSrv("Flags = {Cumulative}\n");
    CHECK(orphan.error == ObjectTypeTableError::MissingTypeId);
    CHECK(orphan.line == 1);

    const auto duplicate = LoadObjectTypeTableFromObjectsSrv(
        "TypeID = 100\nFlags = {}\nTypeID = 100\nFlags = {}\n");
    CHECK(duplicate.error == ObjectTypeTableError::DuplicateTypeId);

    const auto bad_id = LoadObjectTypeTableFromObjectsSrv("TypeID = abc\n");
    CHECK(bad_id.error == ObjectTypeTableError::InvalidTypeId);

    const auto huge_id = LoadObjectTypeTableFromObjectsSrv("TypeID = 65280\n");
    CHECK(huge_id.error == ObjectTypeTableError::TypeIdOutOfRange);

    const auto bad_flags = LoadObjectTypeTableFromObjectsSrv("TypeID = 100\nFlags = Cumulative\n");
    CHECK(bad_flags.error == ObjectTypeTableError::MalformedFlags);

    CHECK(std::string(ObjectTypeTableErrorName(ObjectTypeTableError::DuplicateTypeId))
          == "DuplicateTypeId");
}

// ------------------------------------------------------------ golden fixtures

void TestGoldenEmptySurface() {
    const auto golden = vectors::HexBytes(vectors::kGoldenEmptySurfaceHex);
    CHECK(golden.size() == 22);

    const TileMap empty;
    CHECK(Encode(1000, 1000, 7, empty) == golden);

    const auto result = DecodeFullScreen(golden, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.bytes_consumed == golden.size());
    CHECK(result.remaining_bytes.empty());
    CHECK(result.message.window.player_position == (MapPosition{1000, 1000, 7}));
    CHECK(result.message.window.min_x == 992);
    CHECK(result.message.window.min_y == 994);
    CHECK(result.message.first_floor == 7);
    CHECK(result.message.last_floor == 0);
    CHECK(result.message.floor_step == -1);
    CHECK(result.message.floors.size() == 8);
    CHECK(result.message.described_tiles == 0);
    CHECK(result.message.skipped_tiles == 2016);
    CHECK(result.message.scanned_positions() == 2016);
    for (std::size_t i = 0; i < result.message.floors.size(); ++i) {
        CHECK(result.message.floors[i].z == static_cast<std::int32_t>(7 - i));
        CHECK(result.message.floors[i].offset == static_cast<std::int32_t>(i));
        CHECK(result.message.floors[i].tiles.empty());
    }
}

void TestGoldenEmptyUnderground() {
    const auto golden = vectors::HexBytes(vectors::kGoldenEmptyUndergroundHex);
    CHECK(golden.size() == 16);

    const TileMap empty;
    CHECK(Encode(1000, 1000, 8, empty) == golden);

    const auto result = DecodeFullScreen(golden, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.floors.size() == 5);
    CHECK(result.message.first_floor == 6);
    CHECK(result.message.last_floor == 10);
    CHECK(result.message.floor_step == 1);
    CHECK(result.message.skipped_tiles == 1260);
    CHECK(result.message.floors[0].z == 6 && result.message.floors[0].offset == 2);
    CHECK(result.message.floors[2].z == 8 && result.message.floors[2].offset == 0);
    CHECK(result.message.floors[4].z == 10 && result.message.floors[4].offset == -2);
}

void TestGoldenPopulatedTile() {
    const auto golden = vectors::HexBytes(vectors::kGoldenPopulatedTileHex);

    TileMap tiles;
    tiles[{992, 994, 7}] = {
        vectors::PlainItem(100),
        vectors::IntroducedCreature(0x04030201U, 0, "Rook"),
        vectors::CumulativeItem(200, 37),
        vectors::LiquidItem(300, 5),
    };
    CHECK(Encode(1000, 1000, 7, tiles) == golden);

    const auto result = DecodeFullScreen(golden, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.described_tiles == 1);
    CHECK(result.message.skipped_tiles == 2015);
    CHECK(result.message.thing_count == 4);
    CHECK(result.message.creature_count == 1);
    CHECK(result.message.bytes_consumed == golden.size());

    const MapFloor& surface = result.message.floors[0];
    CHECK(surface.z == 7 && surface.tiles.size() == 1);
    const MapTile& tile = surface.tiles[0];
    CHECK(tile.position == (MapPosition{992, 994, 7}));
    CHECK(tile.things.size() == 4);

    // Stack order is preserved: index == stack position.
    CHECK(tile.things[0].kind == MapThingKind::Item);
    CHECK(tile.things[0].item.type_id == 100);
    CHECK(!tile.things[0].item.has_amount && !tile.things[0].item.has_liquid_color);

    CHECK(tile.things[1].kind == MapThingKind::Creature);
    const CreatureThing& creature = tile.things[1].creature;
    CHECK(creature.kind == CreatureDescriptorKind::Introduced);
    CHECK(creature.creature_id == 0x04030201U);
    CHECK(creature.evicts_slot && creature.removed_creature_id == 0);
    CHECK(creature.has_name && creature.name == "Rook");
    CHECK(creature.has_descriptor);
    CHECK(creature.health_percent == 100);
    CHECK(creature.direction == 2);
    CHECK(creature.outfit.outfit_id == 128 && !creature.outfit.disguised_as_object);
    CHECK((creature.outfit.colors == std::array<std::uint8_t, 4>{78, 69, 58, 76}));
    CHECK(creature.light_brightness == 0 && creature.light_color == 0);
    CHECK(creature.speed == 220);
    CHECK(creature.playerkilling_mark == 0 && creature.party_mark == 0);

    CHECK(tile.things[2].kind == MapThingKind::Item);
    CHECK(tile.things[2].item.type_id == 200);
    CHECK(tile.things[2].item.has_amount && tile.things[2].item.amount == 37);
    CHECK(!tile.things[2].item.has_liquid_color);

    CHECK(tile.things[3].kind == MapThingKind::Item);
    CHECK(tile.things[3].item.type_id == 300);
    CHECK(tile.things[3].item.has_liquid_color && tile.things[3].item.liquid_color == 5);
    CHECK(!tile.things[3].item.has_amount);
}

// ------------------------------------------------------- structural behaviour

void TestFloorOffsetsAndScanOrder() {
    // One tile on the first scanned position of three different floors. The
    // server adds (PlayerZ - PointZ) to both x and y before reading the map.
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::PlainItem(100)};   // floor 7, offset 0
    tiles[{993, 995, 6}] = {vectors::PlainItem(101)};   // floor 6, offset 1
    tiles[{999, 1001, 0}] = {vectors::PlainItem(102)};  // floor 0, offset 7
    const auto bytes = Encode(1000, 1000, 7, tiles);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.described_tiles == 3);
    CHECK(result.message.floors[0].tiles.size() == 1);
    CHECK(result.message.floors[0].tiles[0].position == (MapPosition{992, 994, 7}));
    CHECK(result.message.floors[1].tiles.size() == 1);
    CHECK(result.message.floors[1].tiles[0].position == (MapPosition{993, 995, 6}));
    CHECK(result.message.floors[7].tiles.size() == 1);
    CHECK(result.message.floors[7].tiles[0].position == (MapPosition{999, 1001, 0}));
    CHECK(result.message.described_tiles + result.message.skipped_tiles == 2016);
}

void TestAdjacentTilesEmitZeroSkip() {
    // Two consecutive scanned positions: y is the inner loop, so (x, y+1)
    // directly follows (x, y). The separator must be the (0, 0xFF) marker.
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::PlainItem(100)};
    tiles[{992, 995, 7}] = {vectors::PlainItem(101)};
    const auto bytes = Encode(1000, 1000, 7, tiles);
    CHECK(bytes[6] == 100 && bytes[7] == 0);
    CHECK(bytes[8] == 0x00 && bytes[9] == 0xFF);
    CHECK(bytes[10] == 101 && bytes[11] == 0);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.described_tiles == 2);
    CHECK(result.message.described_tiles + result.message.skipped_tiles == 2016);
}

void TestSkipRunCrossesFloorBoundary() {
    // The server's `Skip` counter is file-scope and reset once before the floor
    // loop, so a run of empty tiles spans the floor change.
    TileMap tiles;
    tiles[{992 + 13, 994 + 13, 7}] = {vectors::PlainItem(100)};  // late on floor 7
    tiles[{993, 995, 6}] = {vectors::PlainItem(101)};            // first on floor 6
    const auto bytes = Encode(1000, 1000, 7, tiles);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.floors[0].tiles.size() == 1);
    CHECK(result.message.floors[1].tiles.size() == 1);
    CHECK(result.message.floors[1].tiles[0].position == (MapPosition{993, 995, 6}));
    CHECK(result.message.described_tiles + result.message.skipped_tiles == 2016);
}

void TestLongSkipRunSplitsMarkers() {
    // 2015 empty positions after the first described tile force SkipFlush to
    // emit seven saturated markers plus a remainder.
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::PlainItem(100)};
    const auto bytes = Encode(1000, 1000, 7, tiles);
    std::size_t saturated = 0;
    for (std::size_t i = 8; i + 1 < bytes.size(); i += 2) {
        if (bytes[i] == 0xFF && bytes[i + 1] == 0xFF) saturated += 1;
    }
    CHECK(saturated == 7);
    CHECK(bytes[bytes.size() - 2] == 223 && bytes[bytes.size() - 1] == 0xFF);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.skipped_tiles == 2015);
}

void TestTenObjectsPerTileAccepted() {
    TileMap tiles;
    std::vector<MapThing> stack;
    for (std::size_t i = 0; i < kMapObjectsPerPointLimit; ++i) {
        stack.push_back(vectors::PlainItem(100));
    }
    tiles[{992, 994, 7}] = stack;
    const auto bytes = Encode(1000, 1000, 7, tiles);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.floors[0].tiles[0].things.size() == kMapObjectsPerPointLimit);
}

void TestCreatureVariants() {
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::IntroducedCreature(1001, 0, "Alpha")};
    tiles[{992, 995, 7}] = {vectors::OutdatedCreature(1001)};
    tiles[{992, 996, 7}] = {vectors::KnownCreature(1001, 3)};
    const auto bytes = Encode(1000, 1000, 7, tiles);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    CHECK(result.message.creature_count == 3);
    const auto& floor = result.message.floors[0];
    CHECK(floor.tiles[0].things[0].creature.kind == CreatureDescriptorKind::Introduced);
    CHECK(floor.tiles[0].things[0].creature.name == "Alpha");
    CHECK(floor.tiles[1].things[0].creature.kind == CreatureDescriptorKind::Outdated);
    CHECK(floor.tiles[1].things[0].creature.has_descriptor);
    CHECK(!floor.tiles[1].things[0].creature.has_name);
    CHECK(floor.tiles[2].things[0].creature.kind == CreatureDescriptorKind::Known);
    CHECK(!floor.tiles[2].things[0].creature.has_descriptor);
    CHECK(floor.tiles[2].things[0].creature.direction == 3);
}

void TestOutfitObjectDisguise() {
    MapThing thing = vectors::IntroducedCreature(2002, 0, "Ghost");
    thing.creature.outfit.outfit_id = 0;
    thing.creature.outfit.object_type = 1234;
    TileMap tiles;
    tiles[{992, 994, 7}] = {thing};
    const auto bytes = Encode(1000, 1000, 7, tiles);

    const auto result = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(result.ok());
    const OutfitDescriptor& outfit =
        result.message.floors[0].tiles[0].things[0].creature.outfit;
    CHECK(outfit.outfit_id == 0);
    CHECK(outfit.disguised_as_object);
    CHECK(outfit.object_type == 1234);
}

// ------------------------------------------------------------ negative cases

void TestNegativeCases() {
    const ObjectTypeTable types = vectors::GoldenObjectTypeTable();
    const auto golden = vectors::HexBytes(vectors::kGoldenPopulatedTileHex);

    // Empty table: item lengths are undecidable.
    CHECK(DecodeFullScreen(golden, ObjectTypeTable{}).error
          == MapDecodeError::EmptyObjectTypeTable);

    // Wrong opcode and empty payload.
    std::vector<std::uint8_t> wrong = golden;
    wrong[0] = 101;
    CHECK(DecodeFullScreen(wrong, types).error == MapDecodeError::NotFullScreen);
    CHECK(DecodeFullScreen({}, types).error == MapDecodeError::Truncated);

    // Every truncation of the golden message must fail explicitly, never
    // silently succeed on a short buffer.
    for (std::size_t length = 1; length < golden.size(); ++length) {
        const std::vector<std::uint8_t> partial(golden.begin(),
                                                golden.begin() + static_cast<std::ptrdiff_t>(length));
        const auto result = DecodeFullScreen(partial, types);
        CHECK(!result.ok());
        CHECK(result.error == MapDecodeError::Truncated);
        CHECK(result.error_offset <= partial.size());
    }

    // Floor above the addressable maximum.
    std::vector<std::uint8_t> bad_floor = golden;
    bad_floor[5] = 16;
    CHECK(DecodeFullScreen(bad_floor, types).error == MapDecodeError::InvalidPlayerFloor);

    // Server-internal container type id on the map.
    std::vector<std::uint8_t> reserved = golden;
    reserved[6] = 5;
    reserved[7] = 0;
    CHECK(DecodeFullScreen(reserved, types).error == MapDecodeError::ReservedObjectTypeId);

    // Type id the table does not declare.
    std::vector<std::uint8_t> unknown = golden;
    unknown[6] = 0x39;
    unknown[7] = 0x05;  // 1337
    const auto unknown_result = DecodeFullScreen(unknown, types);
    CHECK(unknown_result.error == MapDecodeError::UnknownObjectTypeId);
    CHECK(unknown_result.error_offset == 6);

    // Eleven objects in one tile without a terminating marker.
    TileMap tiles;
    std::vector<MapThing> stack;
    for (std::size_t i = 0; i < kMapObjectsPerPointLimit; ++i) {
        stack.push_back(vectors::PlainItem(100));
    }
    tiles[{992, 994, 7}] = stack;
    std::vector<std::uint8_t> overfull = Encode(1000, 1000, 7, tiles);
    overfull.insert(overfull.begin() + 6, {100, 0});
    CHECK(DecodeFullScreen(overfull, types).error == MapDecodeError::TooManyObjectsInTile);

    // A skip run that outlives the window: the golden empty screen drains
    // exactly 2016 positions, so widening its last marker by one leaves the
    // run unfinished when the scan ends.
    std::vector<std::uint8_t> overrun = vectors::HexBytes(vectors::kGoldenEmptySurfaceHex);
    CHECK(DecodeFullScreen(overrun, types).ok());
    CHECK(overrun[overrun.size() - 2] == 223);
    overrun[overrun.size() - 2] = 224;
    CHECK(DecodeFullScreen(overrun, types).error == MapDecodeError::SkipRunExceedsWindow);

    // A creature name longer than TCreature::Name can hold.
    MapThing long_name = vectors::IntroducedCreature(3003, 0, std::string(30, 'x'));
    TileMap named;
    named[{992, 994, 7}] = {long_name};
    const auto long_bytes = Encode(1000, 1000, 7, named);
    CHECK(DecodeFullScreen(long_bytes, types).error == MapDecodeError::CreatureNameTooLong);

    MapThing limit_name = vectors::IntroducedCreature(3003, 0, std::string(29, 'x'));
    TileMap at_limit;
    at_limit[{992, 994, 7}] = {limit_name};
    CHECK(DecodeFullScreen(Encode(1000, 1000, 7, at_limit), types).ok());

    CHECK(std::string(MapDecodeErrorName(MapDecodeError::Truncated)) == "Truncated");
}

// --------------------------------------------------------------- world state

void TestApplyFullScreen() {
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::PlainItem(100),
                            vectors::IntroducedCreature(7001, 0, "Alpha")};
    tiles[{992, 995, 7}] = {vectors::IntroducedCreature(7002, 0, "Beta")};
    const auto bytes = Encode(1000, 1000, 7, tiles);
    const auto decoded = DecodeFullScreen(bytes, vectors::GoldenObjectTypeTable());
    CHECK(decoded.ok());

    WorldState state;
    const auto applied = ApplyFullScreen(&state, decoded.message);
    CHECK(applied.clean());
    CHECK(state.map_initialized);
    CHECK(state.window.player_position == (MapPosition{1000, 1000, 7}));
    CHECK(state.tile_count() == 2);
    CHECK(state.thing_count() == 3);
    CHECK(state.known_creatures.size() == 2);
    CHECK(state.known_creatures.at(7001).name == "Alpha");
    CHECK(state.known_creatures.at(7001).stack_position == 1);
    CHECK(state.known_creatures.at(7001).position == (MapPosition{992, 994, 7}));
    CHECK(state.known_creatures.at(7001).has_descriptor);
    CHECK(state.known_creatures.at(7002).stack_position == 0);
    CHECK(state.FindTile({992, 994, 7}) != nullptr);
    CHECK(state.FindTile({992, 994, 7})->things.size() == 2);
    CHECK(state.FindTile({1, 1, 1}) == nullptr);

    // A second fullscreen that introduces a creature by evicting a known slot.
    TileMap next;
    next[{992, 994, 7}] = {vectors::IntroducedCreature(7003, 7001, "Gamma")};
    const auto second = DecodeFullScreen(Encode(1000, 1000, 7, next),
                                         vectors::GoldenObjectTypeTable());
    CHECK(second.ok());
    const auto reapplied = ApplyFullScreen(&state, second.message);
    CHECK(reapplied.clean());
    CHECK(state.known_creatures.count(7001) == 0);
    CHECK(state.known_creatures.count(7003) == 1);
    CHECK(state.tile_count() == 1);
}

void TestApplyReportsAnomalies() {
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::KnownCreature(9001, 1)};
    tiles[{992, 995, 7}] = {vectors::OutdatedCreature(9002)};
    tiles[{992, 996, 7}] = {vectors::IntroducedCreature(9003, 9999, "Delta")};
    const auto decoded = DecodeFullScreen(Encode(1000, 1000, 7, tiles),
                                          vectors::GoldenObjectTypeTable());
    CHECK(decoded.ok());

    WorldState state;
    const auto applied = ApplyFullScreen(&state, decoded.message);
    CHECK(!applied.clean());
    CHECK(applied.anomalies.size() == 3);
    CHECK(applied.anomalies[0].kind == WorldStateAnomalyKind::UnknownCreatureReference);
    CHECK(applied.anomalies[0].creature_id == 9001);
    CHECK(applied.anomalies[1].kind == WorldStateAnomalyKind::UnknownCreatureReference);
    CHECK(applied.anomalies[1].creature_id == 9002);
    CHECK(applied.anomalies[2].kind == WorldStateAnomalyKind::UnknownEvictedCreature);
    CHECK(applied.anomalies[2].creature_id == 9999);
    // Nothing is dropped: the referenced creatures still enter the mirror.
    CHECK(state.known_creatures.size() == 3);
    CHECK(!state.known_creatures.at(9001).has_descriptor);
    CHECK(state.known_creatures.at(9002).has_descriptor);
    CHECK(std::string(WorldStateAnomalyKindName(applied.anomalies[2].kind))
          == "UnknownEvictedCreature");
    CHECK(std::string(CreatureDescriptorKindName(CreatureDescriptorKind::Known)) == "Known");

    CHECK(ApplyFullScreen(nullptr, decoded.message).clean());
}

// ------------------------------------------------------- initial world facade

void TestDecodeInitialWorld() {
    const ObjectTypeTable types = vectors::GoldenObjectTypeTable();
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::PlainItem(100)};
    std::vector<std::uint8_t> payload = Encode(1000, 1000, 7, tiles);
    const std::size_t fullscreen_size = payload.size();

    // The 7.72 login path commits several commands into one encrypted frame:
    // SendFullScreen is followed by GraphicalEffect, inventory, ambience and
    // more. Those bytes must survive untouched.
    const std::vector<std::uint8_t> trailing{130, 0x05, 0xD0, 0x07, 0x00, 0x00};
    payload.insert(payload.end(), trailing.begin(), trailing.end());

    const auto result = DecodeInitialWorld(payload, types);
    CHECK(result.ok());
    CHECK(result.status == InitialWorldStatus::FullScreenDecoded);
    CHECK(result.fullscreen.message.bytes_consumed == fullscreen_size);
    CHECK(result.fullscreen.remaining_bytes == trailing);
    CHECK(result.has_trailing_command);
    CHECK(result.trailing_command.opcode == 130);
    CHECK(std::string(result.trailing_command.name) == "SV_CMD_AMBIENTE");
    CHECK(result.trailing_command.offset == fullscreen_size);

    const auto not_fullscreen = DecodeInitialWorld({131, 1, 2, 3}, types);
    CHECK(not_fullscreen.status == InitialWorldStatus::NoFullScreen);
    CHECK(not_fullscreen.fullscreen.remaining_bytes == (std::vector<std::uint8_t>{131, 1, 2, 3}));
    CHECK(not_fullscreen.has_trailing_command);
    CHECK(std::string(not_fullscreen.trailing_command.name) == "SV_CMD_GRAPHICAL_EFFECT");

    const auto nothing = DecodeInitialWorld({}, types);
    CHECK(nothing.status == InitialWorldStatus::NoFullScreen);
    CHECK(!nothing.has_trailing_command);

    std::vector<std::uint8_t> broken = Encode(1000, 1000, 7, tiles);
    broken.resize(broken.size() - 2);
    const auto failed = DecodeInitialWorld(broken, types);
    CHECK(failed.status == InitialWorldStatus::DecodeFailed);
    CHECK(failed.fullscreen.error == MapDecodeError::Truncated);
    CHECK(std::string(InitialWorldStatusName(failed.status)) == "DecodeFailed");

    CHECK(std::string(ServerCommandName(100)) == "SV_CMD_FULLSCREEN");
    CHECK(std::string(ServerCommandName(109)) == "SV_CMD_MOVE_CREATURE");
    CHECK(std::string(ServerCommandName(212)) == "SV_CMD_BUDDY_OFFLINE");
    CHECK(std::string(ServerCommandName(250)) == "Unknown");
}

// Ties the new layer to the existing Game Login handoff: the bytes that
// ParseGameInitialMessage preserves for a recognized-unparsed FULLSCREEN are
// exactly what DecodeInitialWorld consumes.
void TestGameLoginHandoffBoundary() {
    const ObjectTypeTable types = vectors::GoldenObjectTypeTable();
    TileMap tiles;
    tiles[{992, 994, 7}] = {vectors::PlainItem(100)};
    const std::vector<std::uint8_t> fullscreen = Encode(1000, 1000, 7, tiles);

    std::vector<std::uint8_t> payload{10, 0x01, 0x02, 0x03, 0x04, 0xE8, 0x03, 0x01};
    payload.insert(payload.end(), fullscreen.begin(), fullscreen.end());

    XteaDecodeResult decoded;
    decoded.error = CryptoError::None;
    decoded.message = payload;
    const auto initial = ParseGameInitialMessage(decoded);
    CHECK(initial.status == GameMessageStatus::FullScreenUnparsed);
    CHECK(initial.init_game_received);
    CHECK(initial.creature_id == 0x04030201U);
    CHECK(initial.unparsed_bytes == fullscreen);

    const auto world = DecodeInitialWorld(initial.unparsed_bytes, types);
    CHECK(world.ok());
    CHECK(world.fullscreen.message.described_tiles == 1);
    CHECK(world.fullscreen.remaining_bytes.empty());
    CHECK(!world.has_trailing_command);
}

}  // namespace

int main() {
    try {
        TestObjectTypeTable();
        TestObjectsSrvLoader();
        TestGoldenEmptySurface();
        TestGoldenEmptyUnderground();
        TestGoldenPopulatedTile();
        TestFloorOffsetsAndScanOrder();
        TestAdjacentTilesEmitZeroSkip();
        TestSkipRunCrossesFloorBoundary();
        TestLongSkipRunSplitsMarkers();
        TestTenObjectsPerTileAccepted();
        TestCreatureVariants();
        TestOutfitObjectDisguise();
        TestNegativeCases();
        TestApplyFullScreen();
        TestApplyReportsAnomalies();
        TestDecodeInitialWorld();
        TestGameLoginHandoffBoundary();
        std::cout << "protocol772_initial_world_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_initial_world_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
