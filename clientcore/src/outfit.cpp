#include "fusion32/protocol772/outfit.h"

namespace fusion32::protocol772 {

// Mirrors reference/game/src/sending.cc::SendOutfit lines 1671-1675 and
// reference/game/src/receiving.cc::CSetOutfit lines 1425-1429, which are the
// same four lines twice. The test is `Sex == 1`, not `Sex == 2` on the other
// branch, so every value that is not male takes the 136 series.
OutfitSelectionRange SelectableOutfits(std::uint8_t sex, bool premium) noexcept {
    const bool male = sex == kPlayerSexMale;
    OutfitSelectionRange range;
    range.first = male ? std::uint16_t{128} : std::uint16_t{136};
    range.last = male ? std::uint16_t{131} : std::uint16_t{139};
    if (premium) {
        range.last = static_cast<std::uint16_t>(range.last + kPremiumExtraOutfits);
    }
    return range;
}

OutfitSelectionRange DialogSelectionRange(const OutfitDialogUpdate& dialog) noexcept {
    return OutfitSelectionRange{dialog.first_outfit, dialog.last_outfit};
}

const char* OutfitSelectionErrorName(OutfitSelectionError error) noexcept {
    switch (error) {
        case OutfitSelectionError::None: return "None";
        case OutfitSelectionError::DisguiseNotSelectable: return "DisguiseNotSelectable";
        case OutfitSelectionError::OutfitIdOutOfRange: return "OutfitIdOutOfRange";
        case OutfitSelectionError::ColorOutOfRange: return "ColorOutOfRange";
    }
    return "Unknown";
}

// CSetOutfit's two tests, in CSetOutfit's order: the band first (lines
// 1431-1438), then the colours (1440-1446). Keeping the order means the reason
// reported here is the one the server would have printed.
OutfitSelectionError ValidateOutfitSelection(const OutfitSelectionRange& range,
                                             std::uint16_t outfit_id,
                                             const OutfitColors& colors) noexcept {
    if (!range.contains(outfit_id)) {
        return OutfitSelectionError::OutfitIdOutOfRange;
    }
    for (const std::uint8_t color : colors) {
        if (color > kMaxOutfitColor) {
            return OutfitSelectionError::ColorOutOfRange;
        }
    }
    return OutfitSelectionError::None;
}

OutfitSelectionError ValidateOutfitSelection(const OutfitSelectionRange& range,
                                             const OutfitDescriptor& outfit) noexcept {
    // Ahead of the server's own tests, because this one is about the command
    // rather than about the selection: CL_CMD_SET_OUTFIT has no field for an
    // object type, so a disguise cannot be expressed however valid its id.
    if (outfit.disguised_as_object) {
        return OutfitSelectionError::DisguiseNotSelectable;
    }
    return ValidateOutfitSelection(range, outfit.outfit_id, outfit.colors);
}

std::vector<std::uint8_t> BuildGetOutfitCommand() {
    return {kClientCommandGetOutfit};
}

std::vector<std::uint8_t> BuildSetOutfitCommand(std::uint16_t outfit_id,
                                                const OutfitColors& colors) {
    std::vector<std::uint8_t> command;
    command.reserve(kSetOutfitCommandSize);
    command.push_back(kClientCommandSetOutfit);
    // readWord: little-endian, as everywhere else on this protocol.
    command.push_back(static_cast<std::uint8_t>(outfit_id & 0xFFU));
    command.push_back(static_cast<std::uint8_t>((outfit_id >> 8U) & 0xFFU));
    // readBytes(Colors, 4): always four, never the object-type branch.
    for (const std::uint8_t color : colors) {
        command.push_back(color);
    }
    return command;
}

std::vector<std::uint8_t> BuildSetOutfitCommand(const OutfitDescriptor& outfit) {
    // `object_type` is deliberately not consulted. The command has no field
    // for it; see kSetOutfitCommandSize and ValidateOutfitSelection.
    return BuildSetOutfitCommand(outfit.outfit_id, outfit.colors);
}

}  // namespace fusion32::protocol772
