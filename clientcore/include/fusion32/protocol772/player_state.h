#ifndef FUSION32_PROTOCOL772_PLAYER_STATE_H
#define FUSION32_PROTOCOL772_PLAYER_STATE_H

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/worldstate.h"

#include <cstdint>
#include <string>

namespace fusion32::protocol772 {

// Server command values decoded by this layer. Source:
// reference/game/src/connections.hh, enum ServerCommand.
constexpr std::uint8_t kServerCommandPing = 30;
constexpr std::uint8_t kServerCommandSetInventory = 120;
constexpr std::uint8_t kServerCommandDeleteInventory = 121;
constexpr std::uint8_t kServerCommandAmbient = 130;
constexpr std::uint8_t kServerCommandGraphicalEffect = 131;
constexpr std::uint8_t kServerCommandTextualEffect = 132;
constexpr std::uint8_t kServerCommandMissileEffect = 133;
constexpr std::uint8_t kServerCommandMarkCreature = 134;
constexpr std::uint8_t kServerCommandCreatureHealth = 140;
constexpr std::uint8_t kServerCommandCreatureLight = 141;
constexpr std::uint8_t kServerCommandCreatureOutfit = 142;
constexpr std::uint8_t kServerCommandCreatureSpeed = 143;
constexpr std::uint8_t kServerCommandCreatureSkull = 144;
constexpr std::uint8_t kServerCommandCreatureParty = 145;
constexpr std::uint8_t kServerCommandPlayerData = 160;
constexpr std::uint8_t kServerCommandPlayerSkills = 161;
constexpr std::uint8_t kServerCommandPlayerState = 162;
constexpr std::uint8_t kServerCommandClearTarget = 163;
constexpr std::uint8_t kServerCommandOutfitDialog = 200;
constexpr std::uint8_t kServerCommandBuddyData = 210;
constexpr std::uint8_t kServerCommandBuddyOnline = 211;
constexpr std::uint8_t kServerCommandBuddyOffline = 212;

// Client keepalive. Source: reference/game/src/receiving.cc::CPing, which reads
// nothing from the buffer, so the payload is the opcode alone.
constexpr std::uint8_t kClientCommandPing = 30;
constexpr std::uint8_t kClientCommandLogout = 20;

// Source: reference/game/src/enums.hh, enum InventorySlot. INVENTORY_FIRST is
// the head slot and INVENTORY_LAST the ammo slot.
constexpr std::uint8_t kInventoryFirstSlot = 1;
constexpr std::uint8_t kInventoryLastSlot = 10;

// ------------------------------------------------------------------ payloads

// Source: reference/game/src/sending.cc::SendGraphicalEffect.
struct GraphicalEffectUpdate {
    MapPosition position;
    std::uint8_t effect = 0;
};

// Source: reference/game/src/sending.cc::SendTextualEffect.
struct TextualEffectUpdate {
    MapPosition position;
    std::uint8_t color = 0;
    std::string text;
};

// Source: reference/game/src/sending.cc::SendMissileEffect.
struct MissileEffectUpdate {
    MapPosition origin;
    MapPosition destination;
    std::uint8_t effect = 0;
};

// Source: reference/game/src/sending.cc::SendMarkCreature.
struct MarkCreatureUpdate {
    std::uint32_t creature_id = 0;
    std::uint8_t color = 0;
};

// The five single-attribute creature updates. Source:
// reference/game/src/sending.cc::SendCreatureHealth, SendCreatureLight,
// SendCreatureOutfit, SendCreatureSpeed, SendCreatureSkull, SendCreatureParty.
enum class CreatureAttribute {
    Health,
    Light,
    Outfit,
    Speed,
    Skull,
    Party,
};

const char* CreatureAttributeName(CreatureAttribute attribute) noexcept;

struct CreatureAttributeUpdate {
    CreatureAttribute attribute = CreatureAttribute::Health;
    std::uint32_t creature_id = 0;
    std::uint8_t health_percent = 0;
    std::uint8_t light_brightness = 0;
    std::uint8_t light_color = 0;
    OutfitDescriptor outfit;
    std::uint16_t speed = 0;
    std::uint8_t playerkilling_mark = 0;
    std::uint8_t party_mark = 0;
};

struct AmbientUpdate {
    std::uint8_t brightness = 0;
    std::uint8_t color = 0;
};

struct PlayerDataUpdate {
    PlayerStats stats;
};

struct PlayerSkillsUpdate {
    PlayerSkills skills;
};

struct PlayerStateUpdate {
    std::uint8_t flags = 0;
};

// Source: reference/game/src/sending.cc::SendSetInventory and
// SendDeleteInventory. Decoded so a frame can be walked past them; inventory
// semantics are out of scope and nothing is stored.
struct InventoryUpdate {
    std::uint8_t slot = 0;
    bool cleared = false;  // true for SV_CMD_DELETE_INVENTORY
    ItemThing item;
};

// Source: reference/game/src/sending.cc::SendOutfit(TConnection*). The server
// offers the outfit chooser on a character's very first login, from
// reference/game/src/crplayer.cc line 221, and the selectable range depends on
// the player's sex and premium right.
struct OutfitDialogUpdate {
    OutfitDescriptor current;
    std::uint16_t first_outfit = 0;
    std::uint16_t last_outfit = 0;
};

// Source: reference/game/src/sending.cc::SendBuddyData and SendBuddyStatus.
// Decoded for length only; the buddy list is out of scope.
struct BuddyUpdate {
    std::uint32_t character_id = 0;
    bool has_name = false;
    std::string name;
    bool online = false;
};

// -------------------------------------------------------------- decode helpers
//
// Each reads one command body, with the scanner positioned just past the
// opcode. They report failure through the scanner, so a truncated command is
// always an explicit MapDecodeError and never a partially applied record.

bool DecodeGraphicalEffect(MapScanner* scanner, GraphicalEffectUpdate* output);
bool DecodeTextualEffect(MapScanner* scanner, TextualEffectUpdate* output);
bool DecodeMissileEffect(MapScanner* scanner, MissileEffectUpdate* output);
bool DecodeMarkCreature(MapScanner* scanner, MarkCreatureUpdate* output);
bool DecodeCreatureAttribute(MapScanner* scanner, std::uint8_t opcode,
                             CreatureAttributeUpdate* output);
bool DecodeAmbient(MapScanner* scanner, AmbientUpdate* output);
bool DecodePlayerData(MapScanner* scanner, PlayerDataUpdate* output);
bool DecodePlayerSkills(MapScanner* scanner, PlayerSkillsUpdate* output);
bool DecodePlayerState(MapScanner* scanner, PlayerStateUpdate* output);
bool DecodeInventory(MapScanner* scanner, std::uint8_t opcode,
                     const ObjectTypeTable& types, InventoryUpdate* output);
bool DecodeBuddy(MapScanner* scanner, std::uint8_t opcode, BuddyUpdate* output);
bool DecodeOutfitDialog(MapScanner* scanner, OutfitDialogUpdate* output);

// True for every opcode this layer knows how to size and decode.
bool IsPlayerStateCommand(std::uint8_t opcode) noexcept;

}  // namespace fusion32::protocol772

#endif
