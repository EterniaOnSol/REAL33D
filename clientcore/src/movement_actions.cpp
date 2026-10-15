// The four diagonal walks and the three object actions.
//
// Source for every layout here:
//   reference/game/src/receiving.cc::ReceiveData lines 1735-1738 (the
//     diagonal offsets), CGoDirection, CTurnObject, CCloseContainer,
//     CUpContainer
//   reference/game/src/cract.cc::TCreature::NotifyGo lines 1341, 1453-1463
//     (the triple diagonal delay)
//   reference/game/src/config.cc:100 (Beat = 200)
//   reference/game/src/cr.hh:917 (Object OpenContainer[16])
//
// Separate from movement.cpp only to keep that file from growing without
// bound; the API is declared in movement.h beside the cardinals and the
// other object-reference builders, which is where a reader looks for it.

#include "fusion32/protocol772/movement.h"

namespace fusion32::protocol772 {

namespace {

// x/y/z, type id, stack index: the five fields CMoveObject, CUseObject,
// CTradeObject and CTurnObject all read the same way.
void AppendObjectRef(std::vector<std::uint8_t>* command, const MoveEndpoint& where,
                     std::uint16_t type_id, std::uint8_t stack_index) {
    const auto word = [command](std::uint16_t value) {
        command->push_back(static_cast<std::uint8_t>(value & 0xFF));
        command->push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    };
    word(where.x);
    word(where.y);
    command->push_back(where.z);
    word(type_id);
    command->push_back(stack_index);
}

}  // namespace

std::uint8_t DiagonalWalkCommandOpcode(DiagonalDirection direction) noexcept {
    switch (direction) {
        case DiagonalDirection::Northeast: return kClientCommandGoNortheast;
        case DiagonalDirection::Southeast: return kClientCommandGoSoutheast;
        case DiagonalDirection::Southwest: return kClientCommandGoSouthwest;
        case DiagonalDirection::Northwest: return kClientCommandGoNorthwest;
    }
    return 0;
}

const char* DiagonalDirectionName(DiagonalDirection direction) noexcept {
    switch (direction) {
        case DiagonalDirection::Northeast: return "Northeast";
        case DiagonalDirection::Southeast: return "Southeast";
        case DiagonalDirection::Southwest: return "Southwest";
        case DiagonalDirection::Northwest: return "Northwest";
    }
    return "Unknown";
}

MapPosition DiagonalStepPosition(const MapPosition& from,
                                 DiagonalDirection direction) noexcept {
    // The offsets the dispatcher passes, verbatim. y grows southward.
    MapPosition to = from;
    switch (direction) {
        case DiagonalDirection::Northeast: to.x += 1; to.y -= 1; break;
        case DiagonalDirection::Southeast: to.x += 1; to.y += 1; break;
        case DiagonalDirection::Southwest: to.x -= 1; to.y += 1; break;
        case DiagonalDirection::Northwest: to.x -= 1; to.y -= 1; break;
    }
    return to;
}

std::vector<std::uint8_t> BuildDiagonalWalkCommand(DiagonalDirection direction) {
    return {DiagonalWalkCommandOpcode(direction)};
}

int PredictStepDelayMilliseconds(int waypoints, int speed, bool diagonal,
                                 int beat) noexcept {
    // The server would never be in this state, and dividing anyway would
    // either trap or produce a number a caller might believe.
    if (waypoints <= 0 || speed <= 0 || beat <= 0) {
        return 0;
    }
    if (diagonal) {
        waypoints *= 3;
    }
    const int delay = (waypoints * 1000) / speed;
    const int beat_count = (delay + beat - 1) / beat;
    return beat_count * beat;
}

bool IsTripleDelayDiagonal(const MapPosition& from, const MapPosition& to) noexcept {
    // Both axes must change and the floor must not: cract.cc:1341.
    return to.x != from.x && to.y != from.y && to.z == from.z;
}

std::vector<std::uint8_t> BuildTurnObjectCommand(const MoveEndpoint& object,
                                                 std::uint16_t type_id,
                                                 std::uint8_t stack_index) {
    std::vector<std::uint8_t> command;
    command.reserve(9);
    command.push_back(kClientCommandTurnObject);
    AppendObjectRef(&command, object, type_id, stack_index);
    return command;
}

std::vector<std::uint8_t> BuildCloseContainerCommand(std::uint8_t container) {
    return {kClientCommandCloseContainer, container};
}

std::vector<std::uint8_t> BuildUpContainerCommand(std::uint8_t container) {
    return {kClientCommandUpContainer, container};
}

}  // namespace fusion32::protocol772
