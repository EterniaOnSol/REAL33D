#ifndef FUSION32_PROTOCOL772_WORLDSTATE_H
#define FUSION32_PROTOCOL772_WORLDSTATE_H

#include "fusion32/protocol772/object_types.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace fusion32::protocol772 {

// Terminal geometry. Source: reference/game/src/connections.cc lines 219-222,
// assigned once in the TConnection constructor and never reassigned anywhere
// else in the tree.
constexpr std::int32_t kTerminalOffsetX = 8;
constexpr std::int32_t kTerminalOffsetY = 6;
constexpr std::int32_t kTerminalWidth = 18;
constexpr std::int32_t kTerminalHeight = 14;

// Source: reference/game/src/connections.hh, TKnownCreature KnownCreatureTable[150].
constexpr std::size_t kKnownCreatureTableSize = 150;

// Source: reference/game/src/cr.hh, TCreature::Name is char[30].
constexpr std::size_t kCreatureNameLimit = 29;

struct MapPosition {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    bool operator==(const MapPosition& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const MapPosition& other) const noexcept { return !(*this == other); }
    bool operator<(const MapPosition& other) const noexcept {
        if (z != other.z) return z < other.z;
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

// Source: reference/game/src/sending.cc::SendItem.
struct ItemThing {
    std::uint16_t type_id = 0;
    bool has_liquid_color = false;
    std::uint8_t liquid_color = 0;
    bool has_amount = false;
    std::uint8_t amount = 0;
};

// Source: reference/game/src/sending.cc::SendOutfit and cr.hh::TOutfit, whose
// union makes the two branches mutually exclusive.
struct OutfitDescriptor {
    std::uint16_t outfit_id = 0;
    bool disguised_as_object = false;
    std::uint16_t object_type = 0;
    std::array<std::uint8_t, 4> colors{};
};

enum class CreatureDescriptorKind {
    Known,      // word 99: already up to date on this connection
    Outdated,   // word 98: known id, full descriptor follows
    Introduced, // word 97: new slot, evicts `removed_creature_id`
};

// Source: reference/game/src/sending.cc::SendMapObject, creature branch.
struct CreatureThing {
    CreatureDescriptorKind kind = CreatureDescriptorKind::Known;
    std::uint32_t creature_id = 0;
    bool evicts_slot = false;
    std::uint32_t removed_creature_id = 0;
    bool has_name = false;
    std::string name;
    bool has_descriptor = false;
    std::uint8_t health_percent = 0;
    std::uint8_t direction = 0;
    OutfitDescriptor outfit;
    std::uint8_t light_brightness = 0;
    std::uint8_t light_color = 0;
    std::uint16_t speed = 0;
    std::uint8_t playerkilling_mark = 0;
    std::uint8_t party_mark = 0;
};

enum class MapThingKind { Item, Creature };

struct MapThing {
    MapThingKind kind = MapThingKind::Item;
    ItemThing item;
    CreatureThing creature;
};

// Stack order is the server's object linked list order; the index in `things`
// is the stack position. Source: reference/game/src/sending.cc::SendMapPoint
// walking `Obj.getNextObject()`.
struct MapTile {
    MapPosition position;
    std::vector<MapThing> things;
};

struct MapFloor {
    std::int32_t z = 0;
    std::int32_t offset = 0;  // PlayerZ - z, added to both x and y
    std::vector<MapTile> tiles;  // described tiles only, in emission order
};

struct MapWindow {
    MapPosition player_position;
    std::int32_t min_x = 0;  // before the per-floor offset
    std::int32_t min_y = 0;
    std::int32_t width = kTerminalWidth;
    std::int32_t height = kTerminalHeight;
};

// Source: reference/game/src/sending.cc::SendPlayerData.
struct PlayerStats {
    bool known = false;
    std::uint16_t hitpoints = 0;
    std::uint16_t max_hitpoints = 0;
    std::uint16_t capacity = 0;  // free capacity in whole oz
    std::uint32_t experience = 0;
    std::uint16_t level = 0;
    std::uint8_t level_percent = 0;
    std::uint16_t mana = 0;
    std::uint16_t max_mana = 0;
    std::uint8_t magic_level = 0;
    std::uint8_t magic_level_percent = 0;
    std::uint8_t soul_points = 0;
};

// Source: reference/game/src/sending.cc::SendPlayerSkills, in emission order.
struct PlayerSkill {
    std::uint8_t level = 0;
    std::uint8_t percent = 0;
};

struct PlayerSkills {
    bool known = false;
    PlayerSkill fist;
    PlayerSkill club;
    PlayerSkill sword;
    PlayerSkill axe;
    PlayerSkill distance;
    PlayerSkill shielding;
    PlayerSkill fishing;
};

// Source: reference/game/src/crplayer.cc::TPlayer::CheckState, lines 1213-1247.
enum class PlayerStateFlag : std::uint8_t {
    Poisoned = 0x01,
    Burning = 0x02,
    Electrified = 0x04,
    Drunk = 0x08,
    ManaShield = 0x10,
    Slowed = 0x20,
    Hasted = 0x40,
    LogoutBlocked = 0x80,
};

struct PlayerState {
    bool known = false;
    std::uint8_t flags = 0;

    bool has(PlayerStateFlag flag) const noexcept {
        return (flags & static_cast<std::uint8_t>(flag)) != 0;
    }
};

// Source: reference/game/src/sending.cc::SendAmbiente.
struct AmbientLight {
    bool known = false;
    std::uint8_t brightness = 0;
    std::uint8_t color = 0;
};

// One body slot. `occupied` is false for a slot the server has never filled or
// has emptied with SV_CMD_DELETE_INVENTORY, which is a different thing from an
// item whose type id happens to be zero.
struct InventorySlot {
    bool occupied = false;
    ItemThing item;
};

// One container the player has open, mirroring what SendContainer described.
//
// The order of `objects` is the server's own: SendContainer walks the
// container's object list from `GetFirstContainerObject` forward, and
// SendCreateInContainer prepends. So index 0 is the front of that list and the
// slot indices carried by SV_CMD_CHANGE_IN_CONTAINER and
// SV_CMD_DELETE_IN_CONTAINER address this vector directly.
struct OpenContainer {
    bool open = false;
    std::uint16_t type_id = 0;
    std::string name;
    std::uint8_t capacity = 0;
    // True when the container sits inside another, which is what the classic
    // client's "go up" arrow needs. The server does not say which one.
    bool has_parent = false;
    std::vector<ItemThing> objects;
};

/**
 * The combat half of the player's state: who is being attacked or followed.
 *
 * Fusion32 keeps this in `TCombat` as `AttackDest` plus a `Following` flag,
 * and the wire carries it asymmetrically. The client names a target with
 * CL_CMD_ATTACK or CL_CMD_FOLLOW; the server answers **only when it refuses or
 * revokes**, with SV_CMD_CLEAR_TARGET and, where there is a reason to give, a
 * failure message. There is no acknowledgement of a target it accepted.
 *
 * So `target_creature_id` is what this client asked for and the server has not
 * since cleared. That is the whole of what the protocol supports knowing, and
 * it is recorded here rather than in a widget so one place owns it. Everything
 * that clears it server side -- the target dying, leaving the eight-field
 * range, ceasing to exist, a protection zone, secure mode, logout -- routes
 * through `TCombat::StopAttack(0)`, which is the one place SendClearTarget is
 * called from. See reference/game/src/crcombat.cc.
 */
struct CombatState {
    // Zero when nothing is targeted. Fusion32 uses zero the same way: an
    // AttackDest of 0 is "no target", and a CL_CMD_ATTACK carrying 0 is how a
    // client asks for that.
    std::uint32_t target_creature_id = 0;

    // Which of the two commands named it. `TCombat::Following` decides whether
    // the server strikes the target or merely walks after it, and the two are
    // mutually exclusive: naming a target with the other command replaces it.
    bool following = false;

    // The tactics this client last sent with CL_CMD_SET_TACTICS.
    //
    // NOT server state. Fusion32 has no command that reports tactics back, so
    // nothing here was ever confirmed; `tactics_sent` is false until this
    // client has put a CL_CMD_SET_TACTICS on the wire, and after that these
    // are a record of the request and not of the server's opinion.
    bool tactics_sent = false;
    std::uint8_t attack_mode = 2;  // ATTACK_MODE_BALANCED
    std::uint8_t chase_mode = 0;   // CHASE_MODE_NONE
    std::uint8_t secure_mode = 1;  // SECURE_MODE_ENABLED
};

// What the decoded FULLSCREEN establishes about a creature, and nothing more.
struct CreatureRecord {
    std::uint32_t creature_id = 0;
    bool has_descriptor = false;
    std::string name;
    std::uint8_t health_percent = 0;
    std::uint8_t direction = 0;
    OutfitDescriptor outfit;
    std::uint8_t light_brightness = 0;
    std::uint8_t light_color = 0;
    std::uint16_t speed = 0;
    std::uint8_t playerkilling_mark = 0;
    std::uint8_t party_mark = 0;
    MapPosition position;
    std::size_t stack_position = 0;
};

enum class WorldStateAnomalyKind {
    // The viewport anchor drifted away from the local player's creature
    // position. After a completed step the two must agree, so a mismatch means
    // a row, floor change or creature move was lost.
    ViewportDesynchronized,
    // SV_CMD_CHANGE_FIELD or SV_CMD_DELETE_FIELD named a stack index the local
    // tile does not hold.
    StackIndexOutOfRange,
    // SV_CMD_MOVE_CREATURE named an origin tile or stack index that does not
    // hold the creature it claims to move.
    MoveOriginMismatch,
    // A field command addressed a position outside the current viewport.
    FieldOutsideViewport,
    // A word-98 or word-99 descriptor named a creature the client had never
    // been introduced to. The server only emits those for entries present in
    // its own KnownCreatureTable, so the local mirror is out of step.
    UnknownCreatureReference,
    // A word-97 descriptor evicted a slot the local mirror did not hold.
    UnknownEvictedCreature,
    // More live creatures than the server-side table can hold.
    KnownCreatureTableOverflow,
};

struct WorldStateAnomaly {
    WorldStateAnomalyKind kind = WorldStateAnomalyKind::UnknownCreatureReference;
    std::uint32_t creature_id = 0;
    MapPosition position;
};

struct WorldState {
    bool map_initialized = false;
    MapWindow window;
    std::vector<MapFloor> floors;
    std::map<std::uint32_t, CreatureRecord> known_creatures;

    // The position the server's viewport is currently centred on. Source:
    // reference/game/src/cract.cc::TCreature::NotifyGo updates the player's
    // posx/posy/posz one axis at a time and only then calls SendRow or
    // SendFloors, so the anchor advances with those commands and never with
    // SV_CMD_MOVE_CREATURE.
    MapPosition viewport_anchor;

    // The creature this connection controls, learned from SV_CMD_INIT_GAME.
    std::uint32_t local_creature_id = 0;

    // Local player condition, each carried by its own server command. `known`
    // stays false until the corresponding command has actually arrived.
    PlayerStats stats;
    PlayerSkills skills;
    PlayerState state;
    AmbientLight ambient_light;

    // What the player is wearing, indexed by InventorySlot value, so index 0
    // is unused and INVENTORY_FIRST..INVENTORY_LAST address it directly. Doing
    // it this way rather than packing from zero means a slot number off the
    // wire needs no arithmetic before it is trusted.
    std::array<InventorySlot, 11> inventory{};

    // The containers the player has open, indexed by the server's own
    // container number. A closed one is simply `open == false`; the entry is
    // kept so a number that comes back stays at the same index.
    std::array<OpenContainer, 16> containers{};

    // Who the player is attacking or following, and the tactics last asked
    // for. See CombatState: the server confirms nothing and revokes loudly.
    CombatState combat;

    const MapTile* FindTile(const MapPosition& position) const noexcept;
    MapTile* FindTile(const MapPosition& position) noexcept;
    std::size_t tile_count() const noexcept;
    std::size_t thing_count() const noexcept;

    // The window implied by the current anchor, which is what the server used
    // when it built the last row or floor command.
    MapWindow AnchoredWindow() const noexcept;

    // True when the anchor and the local player's creature agree. Meaningful
    // only once local_creature_id is known and a step has completed.
    bool viewport_synchronized() const noexcept;

    // Creature ids currently standing on a stored tile, in scan order.
    //
    // `known_creatures` is a mirror of TConnection::KnownCreatureTable and, like
    // the server's own table, deliberately retains creatures that have scrolled
    // out of view: reference/game/src/connections.cc::NewKnownCreature only
    // drops an entry when it needs the slot. Use this when comparing against a
    // freshly connected session, whose table starts empty.
    std::vector<std::uint32_t> visible_creature_ids() const;
};

const char* CreatureDescriptorKindName(CreatureDescriptorKind kind) noexcept;
const char* WorldStateAnomalyKindName(WorldStateAnomalyKind kind) noexcept;

}  // namespace fusion32::protocol772

#endif
