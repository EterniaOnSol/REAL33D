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

    // Still unsupported: this task did not claim containers or trade.
    //
    // 170 (SV_CMD_TALK) was in this list until CHAT-772-001 decoded it, and is
    // deliberately not here any more. It is asserted as supported below, so
    // that removing an opcode from this list cannot pass unnoticed.
    //
    // Explicitly typed: a bare braced list deduces initializer_list<int>, and
    // MSVC at /W4 rejects the narrowing that GCC accepts silently.
    // 110 and 112 were here until UNREAL-INVENTORY-CONTAINERS-001 decoded the
    // container family; what is left is trade, the text editors and the
    // quest/channel commands nothing has demonstrated.
    const std::vector<std::uint8_t> still_unsupported{125, 126, 150, 171, 174};
    for (const std::uint8_t opcode : still_unsupported) {
        const auto decoded = Decode({opcode, 1, 2, 3, 4, 5, 6, 7});
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Unsupported);
        CHECK(decoded.update.bytes_consumed == 0);
        CHECK(!IsPlayerStateCommand(opcode));
    }
    // Talk is decoded now. A well-formed one is consumed whole; a malformed one
    // fails rather than being waved through as "unsupported", which is what
    // leaving it in the list above would have quietly restored.
    {
        // SV_CMD_TALK, statement 1, sender "A", TALK_SAY, 1,2,3, text "x".
        const std::vector<std::uint8_t> say{
            170, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 'A', 0x01,
            0x01, 0x00, 0x02, 0x00, 0x03, 0x01, 0x00, 'x'};
        const auto decoded = Decode(say);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::Talk);
        CHECK(decoded.update.bytes_consumed == say.size());
        CHECK(!IsPlayerStateCommand(170));
    }

    CHECK(IsPlayerStateCommand(160));
    CHECK(IsPlayerStateCommand(30));
    CHECK(IsPlayerStateCommand(200));
    CHECK(IsPlayerStateCommand(212));

    CHECK(std::string(MapDecodeErrorName(MapDecodeError::InvalidInventorySlot))
          == "InvalidInventorySlot");
    CHECK(std::string(ServerUpdateKindName(ServerUpdateKind::PlayerData)) == "PlayerData");
}

// ------------------------------------------------------- containers (110-114)
//
// Byte layouts are transcribed from reference/game/src/sending.cc, so a test
// failing here means either the decoder or that transcription is wrong -- not
// that the server changed, which it cannot.

void PushWord(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFF));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void PushString(std::vector<std::uint8_t>& bytes, const std::string& text) {
    PushWord(bytes, static_cast<std::uint16_t>(text.size()));
    bytes.insert(bytes.end(), text.begin(), text.end());
}

void TestContainerCommands() {
    // SV_CMD_CONTAINER: number, type, name, capacity, parent flag, count, items.
    {
        std::vector<std::uint8_t> bytes{kServerCommandContainer, 0};
        PushWord(bytes, 1987);
        PushString(bytes, "bag");
        bytes.push_back(8);   // capacity
        bytes.push_back(0);   // no parent
        bytes.push_back(2);   // two objects
        PushWord(bytes, 101);  // plain
        PushWord(bytes, 200);  // cumulative in the golden table
        bytes.push_back(17);   // so an amount byte follows

        const auto decoded = Decode(bytes);
        CHECK(decoded.error == MapDecodeError::None);
        CHECK(decoded.update.kind == ServerUpdateKind::Container);
        CHECK(decoded.update.bytes_consumed == bytes.size());
        const ContainerUpdate& update = decoded.update.container;
        CHECK(update.kind == ContainerUpdateKind::Opened);
        CHECK(update.container == 0);
        CHECK(update.type_id == 1987);
        CHECK(update.name == "bag");
        CHECK(update.capacity == 8);
        CHECK(!update.has_parent);
        CHECK(update.items.size() == 2);
        CHECK(update.items[0].type_id == 101);
        CHECK(update.items[1].type_id == 200);
        CHECK(update.items[1].has_amount);
        CHECK(update.items[1].amount == 17);

        // And it reaches WorldState in that order.
        WorldState state;
        ApplyServerUpdate(&state, decoded.update, Types());
        CHECK(state.containers[0].open);
        CHECK(state.containers[0].name == "bag");
        CHECK(state.containers[0].objects.size() == 2);
        CHECK(state.containers[0].objects[0].type_id == 101);

        // A create prepends, a change replaces in place, a delete removes.
        const auto created = Decode({kServerCommandCreateInContainer, 0, 0x66, 0x00});
        CHECK(created.error == MapDecodeError::None);
        CHECK(created.update.container.kind == ContainerUpdateKind::Created);
        ApplyServerUpdate(&state, created.update, Types());
        CHECK(state.containers[0].objects.size() == 3);
        CHECK(state.containers[0].objects[0].type_id == 102);
        CHECK(state.containers[0].objects[1].type_id == 101);

        const auto changed = Decode({kServerCommandChangeInContainer, 0, 1, 0x65, 0x00});
        CHECK(changed.error == MapDecodeError::None);
        CHECK(changed.update.container.slot == 1);
        ApplyServerUpdate(&state, changed.update, Types());
        CHECK(state.containers[0].objects.size() == 3);
        CHECK(state.containers[0].objects[1].type_id == 101);

        const auto deleted = Decode({kServerCommandDeleteInContainer, 0, 0});
        CHECK(deleted.error == MapDecodeError::None);
        CHECK(deleted.update.container.kind == ContainerUpdateKind::Deleted);
        ApplyServerUpdate(&state, deleted.update, Types());
        CHECK(state.containers[0].objects.size() == 2);
        CHECK(state.containers[0].objects[0].type_id == 101);

        // Closing clears the entry rather than leaving stale contents behind.
        const auto closed = Decode({kServerCommandCloseContainer, 0});
        CHECK(closed.error == MapDecodeError::None);
        CHECK(closed.update.container.kind == ContainerUpdateKind::Closed);
        ApplyServerUpdate(&state, closed.update, Types());
        CHECK(!state.containers[0].open);
        CHECK(state.containers[0].objects.empty());
    }

    // A container number past the end of CONTAINER_FIRST..CONTAINER_LAST.
    {
        const auto decoded = Decode({kServerCommandCloseContainer, 16});
        CHECK(decoded.error == MapDecodeError::InvalidContainerNumber);
    }
    // A count larger than the server would ever clamp to.
    {
        std::vector<std::uint8_t> bytes{kServerCommandContainer, 0};
        PushWord(bytes, 1987);
        PushString(bytes, "bag");
        bytes.push_back(8);
        bytes.push_back(0);
        bytes.push_back(37);  // MAX_OBJECTS_PER_CONTAINER is 36
        const auto decoded = Decode(bytes);
        CHECK(decoded.error == MapDecodeError::InvalidContainerSlot);
    }
    // Truncated mid-item: reported, not partially applied.
    {
        std::vector<std::uint8_t> bytes{kServerCommandContainer, 0};
        PushWord(bytes, 1987);
        PushString(bytes, "bag");
        bytes.push_back(8);
        bytes.push_back(0);
        bytes.push_back(1);
        bytes.push_back(0x65);  // half a type id
        const auto decoded = Decode(bytes);
        CHECK(decoded.error != MapDecodeError::None);
    }

    CHECK(IsPlayerStateCommand(kServerCommandContainer));
    CHECK(IsPlayerStateCommand(kServerCommandDeleteInContainer));
    CHECK(std::string(ServerUpdateKindName(ServerUpdateKind::Container)) == "Container");
    CHECK(std::string(MapDecodeErrorName(MapDecodeError::InvalidContainerNumber))
          == "InvalidContainerNumber");
}

void TestInventoryReachesWorldState() {
    WorldState state;

    // SendBodyInventory is one SV_CMD_SET_INVENTORY per occupied slot, which
    // is how equipment arrives at login. Slot 1 is the head.
    const auto worn = Decode({kServerCommandSetInventory, 1, 0x65, 0x00});
    CHECK(worn.error == MapDecodeError::None);
    ApplyServerUpdate(&state, worn.update, Types());
    CHECK(state.inventory[1].occupied);
    CHECK(state.inventory[1].item.type_id == 101);

    // Slots are addressed by their own number, so nothing else moved.
    CHECK(!state.inventory[2].occupied);

    const auto removed = Decode({kServerCommandDeleteInventory, 1});
    CHECK(removed.error == MapDecodeError::None);
    ApplyServerUpdate(&state, removed.update, Types());
    CHECK(!state.inventory[1].occupied);
    CHECK(state.inventory[1].item.type_id == 0);
}

void TestMoveObjectCommand() {
    // CMoveObject reads origin word/word/byte, type word, stack byte,
    // destination word/word/byte, count byte.
    const auto onMap = MoveEndpoint::OnMap(MapPosition{32097, 32219, 7});
    const auto inBag = MoveEndpoint::InContainer(0, 3);
    const auto worn = MoveEndpoint::InInventory(1);

    CHECK(onMap.x == 32097 && onMap.y == 32219 && onMap.z == 7);
    CHECK(inBag.x == kSpecialCoordinateX);
    CHECK(inBag.y == 64);  // CONTAINER_FIRST + 0
    CHECK(inBag.z == 3);
    CHECK(worn.x == kSpecialCoordinateX);
    CHECK(worn.y == 1 && worn.z == 0);

    const auto command = BuildMoveObjectCommand(inBag, 101, 3, worn, 1);
    const std::vector<std::uint8_t> expected{
        kClientCommandMoveObject,
        0xFF, 0xFF,  // origin x, special
        0x40, 0x00,  // origin y, container 0
        0x03,        // origin z, slot 3
        0x65, 0x00,  // type id 101
        0x03,        // stack index
        0xFF, 0xFF,  // destination x, special
        0x01, 0x00,  // destination y, inventory slot 1
        0x00,        // destination z
        0x01,        // count
    };
    CHECK(command == expected);
}

void TestUseCommands() {
    const auto inBag = MoveEndpoint::InContainer(0, 2);
    const auto worn = MoveEndpoint::InInventory(3);
    const auto onMap = MoveEndpoint::OnMap(MapPosition{32097, 32219, 7});

    // CUseObject: origin word/word/byte, type word, stack byte, container byte.
    // The last byte is the open-container slot to show a container in, not
    // padding: CUseObject refuses the command when it is out of range.
    {
        const auto command = BuildUseObjectCommand(worn, 2853, 0, 0);
        const std::vector<std::uint8_t> expected{
            kClientCommandUseObject,
            0xFF, 0xFF,  // special coordinate
            0x03, 0x00,  // inventory slot 3
            0x00,        // z
            0x25, 0x0B,  // type id 2853
            0x00,        // stack index
            0x00,        // open as container 0
        };
        CHECK(command == expected);
    }
    // A second container opens into the next free slot, which is how a bag
    // inside a bag ends up as its own window rather than replacing the first.
    {
        const auto command = BuildUseObjectCommand(inBag, 2854, 2, 1);
        CHECK(command.size() == 10);
        CHECK(command[0] == kClientCommandUseObject);
        CHECK(command[3] == 64);   // CONTAINER_FIRST + 0
        CHECK(command[5] == 2);    // slot 2 inside it
        CHECK(command[8] == 2);    // stack index
        CHECK(command[9] == 1);    // opened as container 1
    }

    // CUseTwoObjects: both ends carry a full object reference.
    {
        const auto command = BuildUseTwoObjectsCommand(worn, 101, 0, onMap, 102, 1);
        const std::vector<std::uint8_t> expected{
            kClientCommandUseTwoObjects,
            0xFF, 0xFF, 0x03, 0x00, 0x00,  // the object: inventory slot 3
            0x65, 0x00, 0x00,              // type 101, stack 0
            0x61, 0x7D, 0xDB, 0x7D, 0x07,  // the target: 32097, 32219, 7
            0x66, 0x00, 0x01,              // type 102, stack 1
        };
        CHECK(command == expected);
    }

    // CUseOnCreature: the target is a creature id, so it survives the creature
    // moving between the click and the command arriving.
    {
        const auto command = BuildUseOnCreatureCommand(worn, 101, 0, 0x40000102u);
        CHECK(command.size() == 13);
        CHECK(command[0] == kClientCommandUseOnCreature);
        CHECK(command[9] == 0x02);
        CHECK(command[10] == 0x01);
        CHECK(command[11] == 0x00);
        CHECK(command[12] == 0x40);
    }
}

// The four combat commands, byte for byte against receiving.cc.
void TestCombatCommands() {
    // CAttack reads one quad and hands it to SetAttackDest. Little endian,
    // like every other quad on this wire.
    {
        const auto command = BuildAttackCommand(0x40000102u);
        const std::vector<std::uint8_t> expected{
            kClientCommandAttack, 0x02, 0x01, 0x00, 0x40,
        };
        CHECK(command == expected);
        CHECK(command.size() == 5);
    }

    // CL_CMD_FOLLOW is the same body under a different opcode: one handler,
    // told apart by the bool receiving.cc passes.
    {
        const auto command = BuildFollowCommand(0x40000102u);
        CHECK(command.size() == 5);
        CHECK(command[0] == kClientCommandFollow);
        for (std::size_t i = 1; i < command.size(); ++i) {
            CHECK(command[i] == BuildAttackCommand(0x40000102u)[i]);
        }
    }

    // Target zero is the documented cancel, not a malformed command.
    {
        const auto command = BuildAttackCommand(0);
        const std::vector<std::uint8_t> expected{
            kClientCommandAttack, 0x00, 0x00, 0x00, 0x00,
        };
        CHECK(command == expected);
    }

    // CCancel reads nothing at all.
    CHECK(BuildCancelCommand() == (std::vector<std::uint8_t>{190}));

    // CSetTactics reads three bytes, in this order.
    {
        const auto command = BuildSetTacticsCommand(
            AttackMode::Offensive, ChaseMode::Follow, SecureMode::Enabled);
        const std::vector<std::uint8_t> expected{
            kClientCommandSetTactics, 0x01, 0x01, 0x01,
        };
        CHECK(command == expected);
    }
    {
        const auto command = BuildSetTacticsCommand(
            AttackMode::Defensive, ChaseMode::Stand, SecureMode::Disabled);
        const std::vector<std::uint8_t> expected{
            kClientCommandSetTactics, 0x03, 0x00, 0x00,
        };
        CHECK(command == expected);
    }

    // The enum values are the server's own, and CSetTactics rejects anything
    // else outright. Asserted at compile time: a runtime CHECK on a constant
    // is a constant conditional, which MSVC refuses at /W4 /WX.
    static_assert(static_cast<std::uint8_t>(AttackMode::Offensive) == 1, "");
    static_assert(static_cast<std::uint8_t>(AttackMode::Balanced) == 2, "");
    static_assert(static_cast<std::uint8_t>(AttackMode::Defensive) == 3, "");
    static_assert(static_cast<std::uint8_t>(ChaseMode::Stand) == 0, "");
    static_assert(static_cast<std::uint8_t>(ChaseMode::Follow) == 1, "");
    static_assert(kClientCommandAttack == 161, "");
    static_assert(kClientCommandFollow == 162, "");
    static_assert(kClientCommandSetTactics == 160, "");
    static_assert(kClientCommandCancel == 190, "");
}

// Who holds the target, and what takes it away.
void TestCombatStateFollowsTheServer() {
    WorldState state;
    state.local_creature_id = 0x40000001u;

    CHECK(state.combat.target_creature_id == 0);
    CHECK(!state.combat.following);
    CHECK(!state.combat.tactics_sent);

    // A request records the target, because an accepted one is the single
    // thing Fusion32 never says anything about.
    NoteCombatRequest(&state, 0x40000102u, false);
    CHECK(state.combat.target_creature_id == 0x40000102u);
    CHECK(!state.combat.following);

    // Switching target replaces it rather than adding to it: TCombat holds one
    // AttackDest.
    NoteCombatRequest(&state, 0x40000103u, false);
    CHECK(state.combat.target_creature_id == 0x40000103u);

    // Following the same creature is a different state, not the same one.
    NoteCombatRequest(&state, 0x40000103u, true);
    CHECK(state.combat.target_creature_id == 0x40000103u);
    CHECK(state.combat.following);

    // SV_CMD_CLEAR_TARGET is the only thing the server sends about a target,
    // and it means every way one can end.
    const TileMap empty;
    vectors::ServerEmitter clear(Provider(empty));
    clear.ClearTarget();
    const auto decoded = Decode(clear.bytes());
    CHECK(decoded.ok());
    CHECK(decoded.update.kind == ServerUpdateKind::ClearTarget);
    const auto applied = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(applied.anomalies.empty());
    CHECK(state.combat.target_creature_id == 0);
    CHECK(!state.combat.following);

    // Zero clears, and so does the player's own id: SetAttackDest reads both
    // as "stop attacking".
    NoteCombatRequest(&state, 0x40000102u, false);
    NoteCombatRequest(&state, 0, false);
    CHECK(state.combat.target_creature_id == 0);
    NoteCombatRequest(&state, 0x40000102u, true);
    NoteCombatRequest(&state, state.local_creature_id, true);
    CHECK(state.combat.target_creature_id == 0);
    CHECK(!state.combat.following);

    // Tactics are a record of what was sent and are marked as such, because
    // no server command carries them back.
    NoteTacticsRequest(&state, AttackMode::Offensive, ChaseMode::Follow,
                       SecureMode::Disabled);
    CHECK(state.combat.tactics_sent);
    CHECK(state.combat.attack_mode == 1);
    CHECK(state.combat.chase_mode == 1);
    CHECK(state.combat.secure_mode == 0);

    // Clearing a target leaves the tactics alone: they are separate server
    // state and SendClearTarget says nothing about them.
    NoteCombatRequest(&state, 0x40000102u, false);
    const auto applied_again = ApplyServerUpdate(&state, decoded.update, Types());
    CHECK(applied_again.anomalies.empty());
    CHECK(state.combat.target_creature_id == 0);
    CHECK(state.combat.tactics_sent);
    CHECK(state.combat.attack_mode == 1);
}

}  // namespace

int main() {
    try {
        TestClientKeepaliveCommands();
        TestCombatCommands();
        TestCombatStateFollowsTheServer();
        TestContainerCommands();
        TestInventoryReachesWorldState();
        TestMoveObjectCommand();
        TestUseCommands();
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
