#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/player_state.h"

#include "fixtures/fullscreen_772_vectors.h"

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

const MapPosition kAnchor{32097, 32219, 7};

ServerUpdateDecodeResult Decode(const std::vector<std::uint8_t>& bytes) {
    return DecodeServerUpdate(bytes, 0, kAnchor, Types());
}

// -------------------------------------------------------------- client side

void TestClientKeepaliveCommands() {
    // CPing and CQuitGame both read nothing from the buffer.
    CHECK(BuildPingCommand() == (std::vector<std::uint8_t>{30}));
    CHECK(BuildLogoutCommand() == (std::vector<std::uint8_t>{20}));
}

// ------------------------------------------------------------ golden bytes

void TestGoldenPlayerData() {
    const TileMap empty;
    PlayerStats stats;
    stats.hitpoints = 185;
    stats.max_hitpoints = 185;
    stats.capacity = 470;
    stats.experience = 4200;
    stats.level = 8;
    stats.level_percent = 42;
    stats.mana = 90;
    stats.max_mana = 90;
    stats.magic_level = 3;
    stats.magic_level_percent = 17;
    stats.soul_points = 100;

    vectors::ServerEmitter emitter(Provider(empty));
    emitter.PlayerData(stats);
    const auto golden = vectors::HexBytes(vectors::kGoldenPlayerDataHex);
    CHECK(emitter.bytes() == golden);
    CHECK(golden.size() == 21);

    const auto decoded = Decode(golden);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::PlayerData);
    CHECK(decoded.update.bytes_consumed == golden.size());
    const PlayerStats& out = decoded.update.player_data.stats;
    CHECK(out.known);
    CHECK(out.hitpoints == 185 && out.max_hitpoints == 185);
    CHECK(out.capacity == 470);
    CHECK(out.experience == 4200);
    CHECK(out.level == 8 && out.level_percent == 42);
    CHECK(out.mana == 90 && out.max_mana == 90);
    CHECK(out.magic_level == 3 && out.magic_level_percent == 17);
    CHECK(out.soul_points == 100);
}

void TestGoldenPlayerSkills() {
    const TileMap empty;
    PlayerSkills skills;
    skills.fist = {10, 0};
    skills.club = {11, 5};
    skills.sword = {12, 10};
    skills.axe = {13, 15};
    skills.distance = {14, 20};
    skills.shielding = {15, 25};
    skills.fishing = {16, 30};

    vectors::ServerEmitter emitter(Provider(empty));
    emitter.PlayerSkillSet(skills);
    const auto golden = vectors::HexBytes(vectors::kGoldenPlayerSkillsHex);
    CHECK(emitter.bytes() == golden);
    CHECK(golden.size() == 15);

    const auto decoded = Decode(golden);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::PlayerSkills);
    const PlayerSkills& out = decoded.update.player_skills.skills;
    CHECK(out.known);
    CHECK(out.fist.level == 10 && out.fist.percent == 0);
    CHECK(out.club.level == 11 && out.club.percent == 5);
    CHECK(out.sword.level == 12 && out.sword.percent == 10);
    CHECK(out.axe.level == 13 && out.axe.percent == 15);
    CHECK(out.distance.level == 14 && out.distance.percent == 20);
    CHECK(out.shielding.level == 15 && out.shielding.percent == 25);
    CHECK(out.fishing.level == 16 && out.fishing.percent == 30);
}

void TestGoldenPlayerState() {
    const TileMap empty;
    vectors::ServerEmitter emitter(Provider(empty));
    emitter.PlayerStateFlags(0x90);
    const auto golden = vectors::HexBytes(vectors::kGoldenPlayerStateHex);
    CHECK(emitter.bytes() == golden);
    CHECK(golden.size() == 2);

    const auto decoded = Decode(golden);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::PlayerState);
    CHECK(decoded.update.player_state.flags == 0x90);

    // The bit meanings come from TPlayer::CheckState.
    PlayerState state;
    state.flags = 0x90;
    CHECK(state.has(PlayerStateFlag::ManaShield));
    CHECK(state.has(PlayerStateFlag::LogoutBlocked));
    CHECK(!state.has(PlayerStateFlag::Poisoned));
    CHECK(!state.has(PlayerStateFlag::Burning));
    CHECK(!state.has(PlayerStateFlag::Electrified));
    CHECK(!state.has(PlayerStateFlag::Drunk));
    CHECK(!state.has(PlayerStateFlag::Slowed));
    CHECK(!state.has(PlayerStateFlag::Hasted));
}

void TestGoldenEffectsAndAmbient() {
    const TileMap empty;

    vectors::ServerEmitter ambient(Provider(empty));
    ambient.Ambient(40, 215);
    const auto golden_ambient = vectors::HexBytes(vectors::kGoldenAmbientHex);
    CHECK(ambient.bytes() == golden_ambient);
    auto decoded = Decode(golden_ambient);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::Ambient);
    CHECK(decoded.update.ambient.brightness == 40);
    CHECK(decoded.update.ambient.color == 215);
    CHECK(decoded.update.bytes_consumed == 3);

    vectors::ServerEmitter graphical(Provider(empty));
    graphical.GraphicalEffect(kAnchor, 12);
    const auto golden_graphical = vectors::HexBytes(vectors::kGoldenGraphicalEffectHex);
    CHECK(graphical.bytes() == golden_graphical);
    CHECK(golden_graphical.size() == 7);
    decoded = Decode(golden_graphical);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::GraphicalEffect);
    CHECK(decoded.update.graphical_effect.position == kAnchor);
    CHECK(decoded.update.graphical_effect.effect == 12);

    vectors::ServerEmitter missile(Provider(empty));
    missile.MissileEffect(kAnchor, {32100, 32219, 7}, 3);
    const auto golden_missile = vectors::HexBytes(vectors::kGoldenMissileEffectHex);
    CHECK(missile.bytes() == golden_missile);
    CHECK(golden_missile.size() == 12);
    decoded = Decode(golden_missile);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::MissileEffect);
    CHECK(decoded.update.missile_effect.origin == kAnchor);
    CHECK(decoded.update.missile_effect.destination == (MapPosition{32100, 32219, 7}));
    CHECK(decoded.update.missile_effect.effect == 3);

    vectors::ServerEmitter textual(Provider(empty));
    textual.TextualEffect(kAnchor, 180, "42");
    decoded = Decode(textual.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::TextualEffect);
    CHECK(decoded.update.textual_effect.color == 180);
    CHECK(decoded.update.textual_effect.text == "42");
    CHECK(decoded.update.bytes_consumed == textual.bytes().size());

    vectors::ServerEmitter mark(Provider(empty));
    mark.MarkCreature(1001, 5);
    decoded = Decode(mark.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::MarkCreature);
    CHECK(decoded.update.mark_creature.creature_id == 1001);
    CHECK(decoded.update.mark_creature.color == 5);
    CHECK(decoded.update.bytes_consumed == 6);
}

void TestGoldenZeroPayloadCommands() {
    const TileMap empty;
    vectors::ServerEmitter ping(Provider(empty));
    ping.Ping();
    CHECK(ping.bytes() == vectors::HexBytes(vectors::kGoldenPingHex));
    auto decoded = Decode(ping.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::Ping);
    CHECK(decoded.update.bytes_consumed == 1);

    vectors::ServerEmitter clear(Provider(empty));
    clear.ClearTarget();
    CHECK(clear.bytes() == vectors::HexBytes(vectors::kGoldenClearTargetHex));
    decoded = Decode(clear.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::ClearTarget);
    CHECK(decoded.update.bytes_consumed == 1);
}

void TestGoldenInventoryAndBuddy() {
    const TileMap empty;

    ItemThing item;
    item.type_id = 200;
    item.has_amount = true;
    item.amount = 40;
    vectors::ServerEmitter inventory(Provider(empty));
    inventory.SetInventory(3, item);
    const auto golden_inventory = vectors::HexBytes(vectors::kGoldenSetInventoryHex);
    CHECK(inventory.bytes() == golden_inventory);
    CHECK(golden_inventory.size() == 5);
    auto decoded = Decode(golden_inventory);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::Inventory);
    CHECK(decoded.update.inventory.slot == 3);
    CHECK(!decoded.update.inventory.cleared);
    CHECK(decoded.update.inventory.item.type_id == 200);
    CHECK(decoded.update.inventory.item.has_amount);
    CHECK(decoded.update.inventory.item.amount == 40);

    vectors::ServerEmitter remove(Provider(empty));
    remove.DeleteInventory(10);
    decoded = Decode(remove.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.inventory.slot == 10);
    CHECK(decoded.update.inventory.cleared);
    CHECK(decoded.update.bytes_consumed == 2);

    vectors::ServerEmitter buddy(Provider(empty));
    buddy.BuddyData(1002, "Test Player B", false);
    const auto golden_buddy = vectors::HexBytes(vectors::kGoldenBuddyDataHex);
    CHECK(buddy.bytes() == golden_buddy);
    CHECK(golden_buddy.size() == 21);
    decoded = Decode(golden_buddy);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::Buddy);
    CHECK(decoded.update.buddy.character_id == 1002);
    CHECK(decoded.update.buddy.has_name);
    CHECK(decoded.update.buddy.name == "Test Player B");
    CHECK(!decoded.update.buddy.online);

    // The outfit chooser the server offers on a character's first login.
    OutfitDescriptor current;
    current.outfit_id = 128;
    current.colors = {78, 69, 58, 76};
    vectors::ServerEmitter outfit(Provider(empty));
    outfit.OutfitDialog(current, 128, 131);
    const auto golden_outfit = vectors::HexBytes(vectors::kGoldenOutfitDialogHex);
    CHECK(outfit.bytes() == golden_outfit);
    CHECK(golden_outfit.size() == 11);
    decoded = Decode(golden_outfit);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::OutfitDialog);
    CHECK(decoded.update.outfit_dialog.current.outfit_id == 128);
    CHECK((decoded.update.outfit_dialog.current.colors
           == std::array<std::uint8_t, 4>{78, 69, 58, 76}));
    CHECK(decoded.update.outfit_dialog.first_outfit == 128);
    CHECK(decoded.update.outfit_dialog.last_outfit == 131);
    CHECK(decoded.update.bytes_consumed == 11);

    vectors::ServerEmitter status(Provider(empty));
    status.BuddyStatus(1002, true);
    decoded = Decode(status.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.buddy.character_id == 1002);
    CHECK(!decoded.update.buddy.has_name);
    CHECK(decoded.update.buddy.online);
    CHECK(decoded.update.bytes_consumed == 5);
}

// -------------------------------------------------------- creature updates

void TestCreatureAttributeUpdates() {
    const TileMap empty;
    vectors::ServerEmitter light(Provider(empty));
    light.CreatureLight(1001, 0, 0);
    const auto golden_light = vectors::HexBytes(vectors::kGoldenCreatureLightHex);
    CHECK(light.bytes() == golden_light);
    CHECK(golden_light.size() == 7);

    auto decoded = Decode(golden_light);
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::CreatureAttribute);
    CHECK(decoded.update.creature_attribute.attribute == CreatureAttribute::Light);
    CHECK(decoded.update.creature_attribute.creature_id == 1001);

    struct Case {
        const char* name;
        std::vector<std::uint8_t> bytes;
        CreatureAttribute attribute;
        std::size_t size;
    };
    std::vector<Case> cases;
    {
        vectors::ServerEmitter e(Provider(empty));
        e.CreatureHealth(1001, 73);
        cases.push_back({"health", e.bytes(), CreatureAttribute::Health, 6});
    }
    {
        vectors::ServerEmitter e(Provider(empty));
        OutfitDescriptor outfit;
        outfit.outfit_id = 129;
        outfit.colors = {1, 2, 3, 4};
        e.CreatureOutfit(1001, outfit);
        cases.push_back({"outfit", e.bytes(), CreatureAttribute::Outfit, 11});
    }
    {
        vectors::ServerEmitter e(Provider(empty));
        e.CreatureSpeed(1001, 320);
        cases.push_back({"speed", e.bytes(), CreatureAttribute::Speed, 7});
    }
    {
        vectors::ServerEmitter e(Provider(empty));
        e.CreatureSkull(1001, 3);
        cases.push_back({"skull", e.bytes(), CreatureAttribute::Skull, 6});
    }
    {
        vectors::ServerEmitter e(Provider(empty));
        e.CreatureParty(1001, 4);
        cases.push_back({"party", e.bytes(), CreatureAttribute::Party, 6});
    }
    for (const Case& entry : cases) {
        const auto result = Decode(entry.bytes);
        CHECK(result.ok());
        CHECK(result.update.kind == ServerUpdateKind::CreatureAttribute);
        CHECK(result.update.creature_attribute.attribute == entry.attribute);
        CHECK(result.update.creature_attribute.creature_id == 1001);
        CHECK(result.update.bytes_consumed == entry.size);
        CHECK(entry.bytes.size() == entry.size);
    }

    CHECK(std::string(CreatureAttributeName(CreatureAttribute::Party)) == "Party");
}

// ----------------------------------------------------------- application

WorldState LoggedInWorld(std::uint32_t player_id, TileMap* tiles) {
    const MapPosition player{1000, 1000, 7};
    for (std::int32_t x = player.x - 20; x <= player.x + 20; ++x) {
        for (std::int32_t y = player.y - 20; y <= player.y + 20; ++y) {
            (*tiles)[{x, y, 7}] = {vectors::PlainItem(100)};
        }
    }
    std::vector<MapThing>& stack = (*tiles)[player];
    stack.push_back(vectors::IntroducedCreature(player_id, 0, "Walker"));

    vectors::ServerEmitter screen(Provider(*tiles));
    screen.FullScreen(1000, 1000, 7);
    const auto decoded = DecodeFullScreen(screen.bytes(), Types());
    if (!decoded.ok()) throw Failure("fixture full screen did not decode");
    WorldState state;
    state.local_creature_id = player_id;
    ApplyFullScreen(&state, decoded.message);
    return state;
}

void TestApplicationUpdatesOnlyDemonstratedState() {
    const std::uint32_t player_id = 1001;
    TileMap tiles;
    WorldState state = LoggedInWorld(player_id, &tiles);
    const TileMap empty;

    CHECK(!state.stats.known);
    CHECK(!state.skills.known);
    CHECK(!state.state.known);
    CHECK(!state.ambient_light.known);

    vectors::ServerEmitter burst(Provider(empty));
    burst.Ambient(40, 215);
    burst.PlayerStateFlags(0x10);
    auto decoded = Decode(burst.bytes());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.ambient_light.known);
    CHECK(state.ambient_light.brightness == 40 && state.ambient_light.color == 215);

    decoded = DecodeServerUpdate(burst.bytes(), decoded.update.bytes_consumed, kAnchor,
                                 Types());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.state.known && state.state.has(PlayerStateFlag::ManaShield));

    // Creature attribute updates land on the mirror.
    vectors::ServerEmitter health(Provider(empty));
    health.CreatureHealth(player_id, 61);
    decoded = Decode(health.bytes());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.known_creatures.at(player_id).health_percent == 61);

    vectors::ServerEmitter speed(Provider(empty));
    speed.CreatureSpeed(player_id, 320);
    decoded = Decode(speed.bytes());
    CHECK(decoded.ok());
    CHECK(ApplyServerUpdate(&state, decoded.update, Types()).clean());
    CHECK(state.known_creatures.at(player_id).speed == 320);

    // An update for a creature the mirror never met is reported, not applied.
    vectors::ServerEmitter stranger(Provider(empty));
    stranger.CreatureSkull(999999, 4);
    decoded = Decode(stranger.bytes());
    CHECK(decoded.ok());
    const auto applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(!applied.clean());
    CHECK(applied.anomalies[0].kind == WorldStateAnomalyKind::UnknownCreatureReference);
    CHECK(applied.anomalies[0].creature_id == 999999);
    CHECK(state.known_creatures.count(999999) == 0);

    // Effects, inventory and buddy carry no demonstrated WorldState semantics,
    // so applying them must change nothing at all.
    const std::size_t tiles_before = state.tile_count();
    const std::size_t things_before = state.thing_count();
    const std::size_t creatures_before = state.known_creatures.size();
    vectors::ServerEmitter inert(Provider(empty));
    inert.GraphicalEffect({1000, 1000, 7}, 12);
    inert.TextualEffect({1000, 1000, 7}, 180, "7");
    inert.MissileEffect({1000, 1000, 7}, {1002, 1000, 7}, 3);
    inert.MarkCreature(player_id, 5);
    ItemThing item;
    item.type_id = 101;
    inert.SetInventory(3, item);
    inert.BuddyData(1002, "Test Player B", false);
    inert.ClearTarget();
    inert.Ping();
    std::size_t at = 0;
    std::size_t applied_count = 0;
    while (at < inert.bytes().size()) {
        const auto one = DecodeServerUpdate(inert.bytes(), at, state.viewport_anchor,
                                            Types());
        CHECK(one.ok());
        CHECK(one.update.kind != ServerUpdateKind::Unsupported);
        CHECK(ApplyServerUpdate(&state, one.update, Types()).clean());
        at += one.update.bytes_consumed;
        applied_count += 1;
    }
    CHECK(at == inert.bytes().size());
    CHECK(applied_count == 8);
    CHECK(state.tile_count() == tiles_before);
    CHECK(state.thing_count() == things_before);
    CHECK(state.known_creatures.size() == creatures_before);
}

// ------------------------------------------------- whole-burst consumption

// Mirrors the order reference/game/src/crplayer.cc lines 199-213 emits after a
// successful game login, which is the burst the live client actually receives.
void TestLoginBurstIsConsumedWhole() {
    const std::uint32_t player_id = 1001;
    TileMap tiles;
    WorldState state = LoggedInWorld(player_id, &tiles);

    vectors::ServerEmitter burst(Provider(tiles));
    burst.FullScreen(1000, 1000, 7);
    burst.GraphicalEffect({1000, 1000, 7}, 12);
    ItemThing armor;
    armor.type_id = 101;
    burst.SetInventory(4, armor);
    ItemThing ammo;
    ammo.type_id = 200;
    ammo.has_amount = true;
    ammo.amount = 25;
    burst.SetInventory(10, ammo);
    burst.Ambient(40, 215);
    burst.CreatureLight(player_id, 0, 0);
    PlayerSkills skills;
    skills.fist = {10, 0};
    burst.PlayerSkillSet(skills);
    PlayerStats stats;
    stats.hitpoints = 185;
    stats.max_hitpoints = 185;
    stats.level = 8;
    burst.PlayerData(stats);
    burst.PlayerStateFlags(0x00);
    burst.BuddyData(1002, "Test Player B", false);
    // A character's very first login gets the welcome message and the outfit
    // chooser instead of the last-visit line (crplayer.cc lines 211-222).
    burst.Message(20, "Welcome to Tibia! Please choose your outfit.");
    OutfitDescriptor current;
    current.outfit_id = 128;
    current.colors = {78, 69, 58, 76};
    burst.OutfitDialog(current, 128, 131);
    burst.Ping();

    WorldState fresh;
    fresh.local_creature_id = player_id;
    std::size_t at = 0;
    std::vector<ServerUpdateKind> seen;
    while (at < burst.bytes().size()) {
        const auto one = DecodeServerUpdate(burst.bytes(), at, fresh.viewport_anchor,
                                            Types());
        CHECK(one.ok());
        CHECK(one.update.kind != ServerUpdateKind::Unsupported);
        CHECK(one.update.bytes_consumed > 0);
        CHECK(ApplyServerUpdate(&fresh, one.update, Types()).clean());
        seen.push_back(one.update.kind);
        at += one.update.bytes_consumed;
    }
    // No residual bytes: the whole burst was consumed.
    CHECK(at == burst.bytes().size());
    CHECK(seen.size() == 13);
    CHECK(seen.front() == ServerUpdateKind::FullScreen);
    CHECK(seen.back() == ServerUpdateKind::Ping);
    CHECK(seen[seen.size() - 2] == ServerUpdateKind::OutfitDialog);

    CHECK(fresh.map_initialized);
    CHECK(fresh.viewport_anchor == (MapPosition{1000, 1000, 7}));
    CHECK(fresh.stats.known && fresh.stats.hitpoints == 185 && fresh.stats.level == 8);
    CHECK(fresh.skills.known && fresh.skills.fist.level == 10);
    CHECK(fresh.state.known && fresh.state.flags == 0);
    CHECK(fresh.ambient_light.known && fresh.ambient_light.brightness == 40);
    CHECK(fresh.known_creatures.count(player_id) == 1);
}

// ------------------------------------------------------------ negative cases

void TestNegativeCases() {
    const TileMap empty;
    const std::vector<std::vector<std::uint8_t>> goldens{
        vectors::HexBytes(vectors::kGoldenPlayerDataHex),
        vectors::HexBytes(vectors::kGoldenPlayerSkillsHex),
        vectors::HexBytes(vectors::kGoldenPlayerStateHex),
        vectors::HexBytes(vectors::kGoldenAmbientHex),
        vectors::HexBytes(vectors::kGoldenGraphicalEffectHex),
        vectors::HexBytes(vectors::kGoldenMissileEffectHex),
        vectors::HexBytes(vectors::kGoldenCreatureLightHex),
        vectors::HexBytes(vectors::kGoldenSetInventoryHex),
        vectors::HexBytes(vectors::kGoldenBuddyDataHex),
        vectors::HexBytes(vectors::kGoldenOutfitDialogHex),
    };
    for (const auto& golden : goldens) {
        for (std::size_t length = 1; length < golden.size(); ++length) {
            const std::vector<std::uint8_t> partial(
                golden.begin(), golden.begin() + static_cast<std::ptrdiff_t>(length));
            const auto decoded = Decode(partial);
            CHECK(!decoded.ok());
            CHECK(decoded.error == MapDecodeError::Truncated);
            CHECK(decoded.error_offset <= partial.size());
        }
    }

    // An inventory slot outside INVENTORY_FIRST..INVENTORY_LAST.
    auto bad_slot = vectors::HexBytes(vectors::kGoldenSetInventoryHex);
    bad_slot[1] = 0;
    CHECK(Decode(bad_slot).error == MapDecodeError::InvalidInventorySlot);
    bad_slot[1] = 11;
    CHECK(Decode(bad_slot).error == MapDecodeError::InvalidInventorySlot);

    // An inventory item naming a server-internal container type.
    auto bad_item = vectors::HexBytes(vectors::kGoldenSetInventoryHex);
    bad_item[2] = 99;
    bad_item[3] = 0;
    CHECK(Decode(bad_item).error == MapDecodeError::ReservedObjectTypeId);

    // An inventory item the type table does not declare.
    auto unknown_item = vectors::HexBytes(vectors::kGoldenSetInventoryHex);
    unknown_item[2] = 0x39;
    unknown_item[3] = 0x05;  // 1337
    CHECK(Decode(unknown_item).error == MapDecodeError::UnknownObjectTypeId);

    // Still unsupported: this task did not claim chat, containers or trade.
    for (const std::uint8_t opcode : {110, 112, 125, 150, 170, 171, 174}) {
        const auto decoded = Decode({opcode, 1, 2, 3, 4, 5, 6, 7});
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Unsupported);
        CHECK(decoded.update.bytes_consumed == 0);
        CHECK(!IsPlayerStateCommand(opcode));
    }
    CHECK(IsPlayerStateCommand(160));
    CHECK(IsPlayerStateCommand(30));
    CHECK(IsPlayerStateCommand(200));
    CHECK(IsPlayerStateCommand(212));

    CHECK(std::string(MapDecodeErrorName(MapDecodeError::InvalidInventorySlot))
          == "InvalidInventorySlot");
    CHECK(std::string(ServerUpdateKindName(ServerUpdateKind::PlayerData)) == "PlayerData");
}

}  // namespace

int main() {
    try {
        TestClientKeepaliveCommands();
        TestGoldenPlayerData();
        TestGoldenPlayerSkills();
        TestGoldenPlayerState();
        TestGoldenEffectsAndAmbient();
        TestGoldenZeroPayloadCommands();
        TestGoldenInventoryAndBuddy();
        TestCreatureAttributeUpdates();
        TestApplicationUpdatesOnlyDemonstratedState();
        TestLoginBurstIsConsumedWhole();
        TestNegativeCases();
        std::cout << "protocol772_player_state_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_player_state_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}
