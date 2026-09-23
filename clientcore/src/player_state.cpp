#include "fusion32/protocol772/player_state.h"

namespace fusion32::protocol772 {
namespace {

bool ReadPosition(MapScanner* scanner, MapPosition* position) {
    return ReadFieldPosition(scanner, position);
}

bool ReadSkill(MapScanner* scanner, PlayerSkill* skill, const char* level_what,
               const char* percent_what) {
    return ReadScannerByte(scanner, &skill->level, level_what)
        && ReadScannerByte(scanner, &skill->percent, percent_what);
}

}  // namespace

bool DecodeGraphicalEffect(MapScanner* scanner, GraphicalEffectUpdate* output) {
    return ReadPosition(scanner, &output->position)
        && ReadScannerByte(scanner, &output->effect, "graphical effect type");
}

bool DecodeTextualEffect(MapScanner* scanner, TextualEffectUpdate* output) {
    return ReadPosition(scanner, &output->position)
        && ReadScannerByte(scanner, &output->color, "textual effect color")
        && ReadScannerString(scanner, &output->text, "textual effect text");
}

bool DecodeMissileEffect(MapScanner* scanner, MissileEffectUpdate* output) {
    return ReadPosition(scanner, &output->origin)
        && ReadPosition(scanner, &output->destination)
        && ReadScannerByte(scanner, &output->effect, "missile effect type");
}

bool DecodeMarkCreature(MapScanner* scanner, MarkCreatureUpdate* output) {
    return ReadScannerQuad(scanner, &output->creature_id, "mark creature id")
        && ReadScannerByte(scanner, &output->color, "mark creature color");
}

bool DecodeCreatureAttribute(MapScanner* scanner, std::uint8_t opcode,
                             CreatureAttributeUpdate* output) {
    if (!ReadScannerQuad(scanner, &output->creature_id, "creature id")) return false;
    switch (opcode) {
        case kServerCommandCreatureHealth:
            output->attribute = CreatureAttribute::Health;
            return ReadScannerByte(scanner, &output->health_percent, "creature health");
        case kServerCommandCreatureLight:
            output->attribute = CreatureAttribute::Light;
            return ReadScannerByte(scanner, &output->light_brightness, "creature brightness")
                && ReadScannerByte(scanner, &output->light_color, "creature light color");
        case kServerCommandCreatureOutfit:
            output->attribute = CreatureAttribute::Outfit;
            return ReadScannerOutfit(scanner, &output->outfit);
        case kServerCommandCreatureSpeed:
            output->attribute = CreatureAttribute::Speed;
            return ReadScannerWord(scanner, &output->speed, "creature speed");
        case kServerCommandCreatureSkull:
            output->attribute = CreatureAttribute::Skull;
            return ReadScannerByte(scanner, &output->playerkilling_mark, "creature skull");
        case kServerCommandCreatureParty:
            output->attribute = CreatureAttribute::Party;
            return ReadScannerByte(scanner, &output->party_mark, "creature party mark");
        default:
            break;
    }
    return false;
}

bool DecodeAmbient(MapScanner* scanner, AmbientUpdate* output) {
    return ReadScannerByte(scanner, &output->brightness, "ambient brightness")
        && ReadScannerByte(scanner, &output->color, "ambient color");
}

bool DecodePlayerData(MapScanner* scanner, PlayerDataUpdate* output) {
    PlayerStats& stats = output->stats;
    if (!ReadScannerWord(scanner, &stats.hitpoints, "hitpoints")) return false;
    if (!ReadScannerWord(scanner, &stats.max_hitpoints, "max hitpoints")) return false;
    if (!ReadScannerWord(scanner, &stats.capacity, "capacity")) return false;
    if (!ReadScannerQuad(scanner, &stats.experience, "experience")) return false;
    if (!ReadScannerWord(scanner, &stats.level, "level")) return false;
    if (!ReadScannerByte(scanner, &stats.level_percent, "level percent")) return false;
    if (!ReadScannerWord(scanner, &stats.mana, "mana")) return false;
    if (!ReadScannerWord(scanner, &stats.max_mana, "max mana")) return false;
    if (!ReadScannerByte(scanner, &stats.magic_level, "magic level")) return false;
    if (!ReadScannerByte(scanner, &stats.magic_level_percent, "magic level percent")) {
        return false;
    }
    if (!ReadScannerByte(scanner, &stats.soul_points, "soul points")) return false;
    stats.known = true;
    return true;
}

bool DecodePlayerSkills(MapScanner* scanner, PlayerSkillsUpdate* output) {
    PlayerSkills& skills = output->skills;
    if (!ReadSkill(scanner, &skills.fist, "fist level", "fist percent")) return false;
    if (!ReadSkill(scanner, &skills.club, "club level", "club percent")) return false;
    if (!ReadSkill(scanner, &skills.sword, "sword level", "sword percent")) return false;
    if (!ReadSkill(scanner, &skills.axe, "axe level", "axe percent")) return false;
    if (!ReadSkill(scanner, &skills.distance, "distance level", "distance percent")) {
        return false;
    }
    if (!ReadSkill(scanner, &skills.shielding, "shielding level", "shielding percent")) {
        return false;
    }
    if (!ReadSkill(scanner, &skills.fishing, "fishing level", "fishing percent")) return false;
    skills.known = true;
    return true;
}

bool DecodePlayerState(MapScanner* scanner, PlayerStateUpdate* output) {
    return ReadScannerByte(scanner, &output->flags, "player state flags");
}

bool DecodeInventory(MapScanner* scanner, std::uint8_t opcode,
                     const ObjectTypeTable& types, InventoryUpdate* output) {
    static_cast<void>(types);  // the scanner already owns the table
    if (!ReadScannerByte(scanner, &output->slot, "inventory slot")) return false;
    if (output->slot < kInventoryFirstSlot || output->slot > kInventoryLastSlot) {
        return FailScanner(scanner, MapDecodeError::InvalidInventorySlot,
                           "inventory slot outside INVENTORY_FIRST..INVENTORY_LAST");
    }
    if (opcode == kServerCommandDeleteInventory) {
        output->cleared = true;
        return true;
    }
    return ReadScannerItem(scanner, &output->item);
}

bool DecodeContainer(MapScanner* scanner, std::uint8_t opcode,
                     ContainerUpdate* output) {
    // Every one of the five begins with the container number, which is why
    // they are decoded together.
    if (!ReadScannerByte(scanner, &output->container, "container number")) return false;
    if (output->container >= kMaxOpenContainers) {
        return FailScanner(scanner, MapDecodeError::InvalidContainerNumber,
                           "container number outside CONTAINER_FIRST..CONTAINER_LAST");
    }

    switch (opcode) {
        case kServerCommandCloseContainer:
            output->kind = ContainerUpdateKind::Closed;
            return true;

        case kServerCommandCreateInContainer:
            // No index: SendCreateInContainer puts the object at the head of
            // the container's list, so the slot is implicitly zero.
            output->kind = ContainerUpdateKind::Created;
            output->slot = 0;
            return ReadScannerItem(scanner, &output->item);

        case kServerCommandChangeInContainer:
            output->kind = ContainerUpdateKind::Changed;
            if (!ReadScannerByte(scanner, &output->slot, "container slot")) return false;
            if (output->slot >= kMaxObjectsPerContainer) {
                return FailScanner(scanner, MapDecodeError::InvalidContainerSlot,
                                   "container slot beyond MAX_OBJECTS_PER_CONTAINER");
            }
            return ReadScannerItem(scanner, &output->item);

        case kServerCommandDeleteInContainer:
            output->kind = ContainerUpdateKind::Deleted;
            if (!ReadScannerByte(scanner, &output->slot, "container slot")) return false;
            if (output->slot >= kMaxObjectsPerContainer) {
                return FailScanner(scanner, MapDecodeError::InvalidContainerSlot,
                                   "container slot beyond MAX_OBJECTS_PER_CONTAINER");
            }
            return true;

        case kServerCommandContainer:
            break;

        default:
            return FailScanner(scanner, MapDecodeError::UnexpectedCommand,
                               "not a container command");
    }

    // SV_CMD_CONTAINER, in SendContainer's own order: the disguised type, the
    // name, the capacity attribute, whether it has a parent container, and
    // then the count followed by that many items.
    output->kind = ContainerUpdateKind::Opened;
    if (!ReadScannerWord(scanner, &output->type_id, "container type")) return false;
    if (!ReadScannerString(scanner, &output->name, "container name")) return false;
    if (!ReadScannerByte(scanner, &output->capacity, "container capacity")) return false;

    std::uint8_t has_parent = 0;
    if (!ReadScannerByte(scanner, &has_parent, "container parent flag")) return false;
    output->has_parent = has_parent != 0;

    std::uint8_t count = 0;
    if (!ReadScannerByte(scanner, &count, "container object count")) return false;
    if (count > kMaxObjectsPerContainer) {
        // The server clamps to MAX_OBJECTS_PER_CONTAINER before it sends, so a
        // larger count is not a big container: it is a desynchronised stream,
        // and reading that many items would walk off the end of the command.
        return FailScanner(scanner, MapDecodeError::InvalidContainerSlot,
                           "container object count beyond MAX_OBJECTS_PER_CONTAINER");
    }

    output->items.clear();
    output->items.reserve(count);
    for (std::uint8_t index = 0; index < count; index += 1) {
        ItemThing item;
        if (!ReadScannerItem(scanner, &item)) return false;
        output->items.push_back(item);
    }
    return true;
}

bool DecodeBuddy(MapScanner* scanner, std::uint8_t opcode, BuddyUpdate* output) {
    if (!ReadScannerQuad(scanner, &output->character_id, "buddy character id")) return false;
    if (opcode == kServerCommandBuddyData) {
        if (!ReadScannerString(scanner, &output->name, "buddy name")) return false;
        output->has_name = true;
        std::uint8_t online = 0;
        if (!ReadScannerByte(scanner, &online, "buddy online flag")) return false;
        output->online = online != 0;
        return true;
    }
    output->online = opcode == kServerCommandBuddyOnline;
    return true;
}

bool DecodeOutfitDialog(MapScanner* scanner, OutfitDialogUpdate* output) {
    return ReadScannerOutfit(scanner, &output->current)
        && ReadScannerWord(scanner, &output->first_outfit, "first selectable outfit")
        && ReadScannerWord(scanner, &output->last_outfit, "last selectable outfit");
}

bool IsPlayerStateCommand(std::uint8_t opcode) noexcept {
    switch (opcode) {
        case kServerCommandPing:
        case kServerCommandSetInventory:
        case kServerCommandDeleteInventory:
        case kServerCommandContainer:
        case kServerCommandCloseContainer:
        case kServerCommandCreateInContainer:
        case kServerCommandChangeInContainer:
        case kServerCommandDeleteInContainer:
        case kServerCommandAmbient:
        case kServerCommandGraphicalEffect:
        case kServerCommandTextualEffect:
        case kServerCommandMissileEffect:
        case kServerCommandMarkCreature:
        case kServerCommandCreatureHealth:
        case kServerCommandCreatureLight:
        case kServerCommandCreatureOutfit:
        case kServerCommandCreatureSpeed:
        case kServerCommandCreatureSkull:
        case kServerCommandCreatureParty:
        case kServerCommandPlayerData:
        case kServerCommandPlayerSkills:
        case kServerCommandPlayerState:
        case kServerCommandClearTarget:
        case kServerCommandOutfitDialog:
        case kServerCommandBuddyData:
        case kServerCommandBuddyOnline:
        case kServerCommandBuddyOffline:
            return true;
        default:
            return false;
    }
}

const char* CreatureAttributeName(CreatureAttribute attribute) noexcept {
    switch (attribute) {
        case CreatureAttribute::Health: return "Health";
        case CreatureAttribute::Light: return "Light";
        case CreatureAttribute::Outfit: return "Outfit";
        case CreatureAttribute::Speed: return "Speed";
        case CreatureAttribute::Skull: return "Skull";
        case CreatureAttribute::Party: return "Party";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772
