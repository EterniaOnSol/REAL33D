// CLIENTCORE-MOVEMENT-ACTIONS-772-001.
//
// The four diagonal walks and the three object actions: rotate, close
// container and up container. All client-side; none of the seven has a
// server command of its own, so this suite is byte vectors, the dispatcher's
// own offsets, and the step-delay arithmetic Fusion32 uses for diagonals.

#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/player_state.h"

#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace fusion32::protocol772;

class Failure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define CHECK(condition) do { if (!(condition)) throw Failure(#condition); } while (false)

std::string Hex(const std::vector<std::uint8_t>& bytes) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const std::uint8_t byte : bytes) {
        out.push_back(digits[byte >> 4]);
        out.push_back(digits[byte & 0x0F]);
    }
    return out;
}

constexpr DiagonalDirection kDiagonals[] = {
    DiagonalDirection::Northeast, DiagonalDirection::Southeast,
    DiagonalDirection::Southwest, DiagonalDirection::Northwest,
};

// --------------------------------------------------------- golden: diagonals

void GoldenDiagonalCommands() {
    // Like the cardinals, a diagonal is one byte: CGoDirection takes its
    // offsets from the dispatcher and reads nothing from the buffer.
    CHECK(Hex(BuildDiagonalWalkCommand(DiagonalDirection::Northeast)) == "6a");
    CHECK(Hex(BuildDiagonalWalkCommand(DiagonalDirection::Southeast)) == "6b");
    CHECK(Hex(BuildDiagonalWalkCommand(DiagonalDirection::Southwest)) == "6c");
    CHECK(Hex(BuildDiagonalWalkCommand(DiagonalDirection::Northwest)) == "6d");

    for (const DiagonalDirection direction : kDiagonals) {
        CHECK(BuildDiagonalWalkCommand(direction).size() == 1);
        CHECK(BuildDiagonalWalkCommand(direction)[0]
              == DiagonalWalkCommandOpcode(direction));
    }

    static_assert(kClientCommandGoNortheast == 106, "CL_CMD_GO_NORTHEAST");
    static_assert(kClientCommandGoSoutheast == 107, "CL_CMD_GO_SOUTHEAST");
    static_assert(kClientCommandGoSouthwest == 108, "CL_CMD_GO_SOUTHWEST");
    static_assert(kClientCommandGoNorthwest == 109, "CL_CMD_GO_NORTHWEST");

    // No opcode may collide with a command this client already sends. 110 is
    // deliberately absent from the list: CL_CMD_ROTATE_* start at 111 and
    // 110 is unused by the client.
    const std::set<std::uint8_t> existing{
        kClientCommandGoNorth, kClientCommandGoEast, kClientCommandGoSouth,
        kClientCommandGoWest, kClientCommandGoStop, kClientCommandRotateNorth,
        kClientCommandRotateEast, kClientCommandRotateSouth,
        kClientCommandRotateWest, kClientCommandMoveObject,
        kClientCommandUseObject, kClientCommandUseTwoObjects,
        kClientCommandUseOnCreature, kClientCommandLookAtPoint,
        kClientCommandTalk, kClientCommandPing, kClientCommandLogout,
    };
    std::set<std::uint8_t> added;
    for (const DiagonalDirection direction : kDiagonals) {
        const std::uint8_t opcode = DiagonalWalkCommandOpcode(direction);
        CHECK(existing.count(opcode) == 0);
        CHECK(added.insert(opcode).second);  // all four distinct
    }
    for (const std::uint8_t opcode : {kClientCommandTurnObject,
                                      kClientCommandCloseContainer,
                                      kClientCommandUpContainer}) {
        CHECK(existing.count(opcode) == 0);
        CHECK(added.insert(opcode).second);
    }
    CHECK(added.size() == 7);

    CHECK(std::string(DiagonalDirectionName(DiagonalDirection::Northeast)) == "Northeast");
    CHECK(std::string(DiagonalDirectionName(DiagonalDirection::Northwest)) == "Northwest");

    std::cout << "  diagonal golden bytes and opcode isolation: PASS\n";
}

void DiagonalOffsetsMatchTheDispatcher() {
    // receiving.cc::ReceiveData lines 1735-1738, verbatim:
    //   NORTHEAST ( 1, -1)   SOUTHEAST ( 1,  1)
    //   SOUTHWEST (-1,  1)   NORTHWEST (-1, -1)
    // y grows southward, which is the whole reason Northeast subtracts.
    const MapPosition from{32097, 32219, 7};

    const MapPosition ne = DiagonalStepPosition(from, DiagonalDirection::Northeast);
    CHECK(ne.x == 32098 && ne.y == 32218 && ne.z == 7);

    const MapPosition se = DiagonalStepPosition(from, DiagonalDirection::Southeast);
    CHECK(se.x == 32098 && se.y == 32220 && se.z == 7);

    const MapPosition sw = DiagonalStepPosition(from, DiagonalDirection::Southwest);
    CHECK(sw.x == 32096 && sw.y == 32220 && sw.z == 7);

    const MapPosition nw = DiagonalStepPosition(from, DiagonalDirection::Northwest);
    CHECK(nw.x == 32096 && nw.y == 32218 && nw.z == 7);

    // A diagonal never changes floor, and the four destinations are distinct.
    std::set<std::pair<std::int32_t, std::int32_t>> seen;
    for (const DiagonalDirection direction : kDiagonals) {
        const MapPosition to = DiagonalStepPosition(from, direction);
        CHECK(to.z == from.z);
        CHECK(to != from);
        CHECK(seen.insert({to.x, to.y}).second);
        // Every diagonal step is, by construction, one the server charges
        // triple for.
        CHECK(IsTripleDelayDiagonal(from, to));
    }
    CHECK(seen.size() == 4);

    // A diagonal is exactly the composition of two cardinals, which is what
    // "both offsets non-zero through the same CGoDirection" means.
    CHECK(DiagonalStepPosition(from, DiagonalDirection::Northeast)
          == StepPosition(StepPosition(from, CardinalDirection::North),
                          CardinalDirection::East));
    CHECK(DiagonalStepPosition(from, DiagonalDirection::Southwest)
          == StepPosition(StepPosition(from, CardinalDirection::South),
                          CardinalDirection::West));

    std::cout << "  diagonal offsets match the dispatcher: PASS\n";
}

// ------------------------------------------------------ the triple delay rule

void TripleDelayClassification() {
    const MapPosition from{100, 100, 7};

    // Both axes change, floor does not: charged triple.
    CHECK(IsTripleDelayDiagonal(from, MapPosition{101, 101, 7}));
    CHECK(IsTripleDelayDiagonal(from, MapPosition{99, 101, 7}));

    // One axis only: an ordinary cardinal step.
    CHECK(!IsTripleDelayDiagonal(from, MapPosition{101, 100, 7}));
    CHECK(!IsTripleDelayDiagonal(from, MapPosition{100, 101, 7}));

    // The boundary that actually matters: cract.cc:1341 requires
    // DestZ == OrigZ, so a diagonal that also changes floor is NOT charged
    // triple. Getting this backwards would overstate every staircase step.
    CHECK(!IsTripleDelayDiagonal(from, MapPosition{101, 101, 6}));
    CHECK(!IsTripleDelayDiagonal(from, MapPosition{101, 101, 8}));

    // Standing still is not a diagonal.
    CHECK(!IsTripleDelayDiagonal(from, from));

    std::cout << "  triple-delay classification incl. the floor boundary: PASS\n";
}

void StepDelayArithmetic() {
    // cract.cc:1453-1463, with Beat = 200 from config.cc:100.
    //   if(DiagonalMove) Waypoints *= 3;
    //   Delay = (Waypoints * 1000) / Speed;
    //   BeatCount = (Delay + Beat - 1) / Beat;   // ceiling
    //   interval = BeatCount * Beat;
    static_assert(kDefaultBeatMilliseconds == 200, "config.cc:100 Beat = 200");

    // Clean case: 100 waypoints at speed 500 is exactly one beat, and the
    // diagonal is exactly three.
    CHECK(PredictStepDelayMilliseconds(100, 500, false) == 200);
    CHECK(PredictStepDelayMilliseconds(100, 500, true) == 600);

    // Quantisation rounds up, so a sub-beat step still costs a whole beat.
    CHECK(PredictStepDelayMilliseconds(100, 1000, false) == 200);

    // The x3 lands on waypoints, *before* the division and the quantisation.
    // So the diagonal interval is not generally three times the cardinal
    // one, and a client that multiplied the final number instead would be
    // wrong. 100 waypoints at speed 220: 454 ms -> 600, and 300 waypoints:
    // 1363 ms -> 1400, not 1800.
    CHECK(PredictStepDelayMilliseconds(100, 220, false) == 600);
    CHECK(PredictStepDelayMilliseconds(100, 220, true) == 1400);
    CHECK(PredictStepDelayMilliseconds(100, 220, true)
          != 3 * PredictStepDelayMilliseconds(100, 220, false));

    // Equivalent to passing the tripled waypoints directly, which is what
    // the server literally does.
    CHECK(PredictStepDelayMilliseconds(100, 220, true)
          == PredictStepDelayMilliseconds(300, 220, false));

    // A non-default Beat is honoured: it is script-overridable at
    // config.cc:183, so it is a parameter and not a baked constant.
    CHECK(PredictStepDelayMilliseconds(100, 500, false, 50) == 200);
    CHECK(PredictStepDelayMilliseconds(100, 1000, false, 50) == 100);
    CHECK(PredictStepDelayMilliseconds(100, 1000, false, 1) == 100);

    // Guards: the server would never be in these states, and dividing
    // anyway would either trap or hand back a number a caller might trust.
    CHECK(PredictStepDelayMilliseconds(0, 500, false) == 0);
    CHECK(PredictStepDelayMilliseconds(100, 0, false) == 0);
    CHECK(PredictStepDelayMilliseconds(100, 500, false, 0) == 0);
    CHECK(PredictStepDelayMilliseconds(-100, 500, true) == 0);
    CHECK(PredictStepDelayMilliseconds(100, -500, true) == 0);
    CHECK(PredictStepDelayMilliseconds(100, 500, true, -200) == 0);

    // Monotonic in speed and in waypoints, which is the only behavioural
    // property a caller should rely on beyond the exact numbers above.
    CHECK(PredictStepDelayMilliseconds(100, 200, false)
          >= PredictStepDelayMilliseconds(100, 400, false));
    CHECK(PredictStepDelayMilliseconds(200, 500, false)
          >= PredictStepDelayMilliseconds(100, 500, false));
    CHECK(PredictStepDelayMilliseconds(100, 500, true)
          >= PredictStepDelayMilliseconds(100, 500, false));

    std::cout << "  step-delay arithmetic incl. the x3 ordering: PASS\n";
}

// ----------------------------------------------------------- golden: actions

void GoldenActionCommands() {
    // CTurnObject reads the same five-field object reference as CMoveObject
    // and CUseObject, and nothing else.
    //   85 | 617d | db7d | 07 | 260b | 02
    {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::OnMap(MapPosition{32097, 32219, 7}), 2854, 2);
        CHECK(Hex(command) == "85617ddb7d07260b02");
        CHECK(command.size() == 9);
        CHECK(command[0] == kClientCommandTurnObject);
    }

    // Inside open container 1, slot 3: x = 0xFFFF, y = CONTAINER_FIRST + 1.
    {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::InContainer(1, 3), 2120, 3);
        CHECK(Hex(command) == "85ffff410003480803");
    }

    // In body slot 5.
    {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::InInventory(5), 2854, 0);
        CHECK(Hex(command) == "85ffff050000260b00");
    }

    // Unlike CTradeObject and CMoveObject, CTurnObject does not overwrite
    // the stack index for a map object, so what is sent is what resolves.
    {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::OnMap(MapPosition{32097, 32219, 7}), 2854, 9);
        CHECK(command[8] == 9);
    }

    // CCloseContainer and CUpContainer each read one byte.
    CHECK(Hex(BuildCloseContainerCommand(0)) == "8700");
    CHECK(Hex(BuildCloseContainerCommand(15)) == "870f");
    CHECK(Hex(BuildUpContainerCommand(0)) == "8800");
    CHECK(Hex(BuildUpContainerCommand(15)) == "880f");
    CHECK(BuildCloseContainerCommand(0).size() == 2);
    CHECK(BuildUpContainerCommand(0).size() == 2);

    static_assert(kClientCommandTurnObject == 133, "CL_CMD_TURN_OBJECT");
    static_assert(kClientCommandCloseContainer == 135, "CL_CMD_CLOSE_CONTAINER");
    static_assert(kClientCommandUpContainer == 136, "CL_CMD_UP_CONTAINER");

    std::cout << "  action golden bytes: PASS\n";
}

void ContainerNumberBoundaries() {
    // cr.hh:917 is `Object OpenContainer[16]`, and both handlers test
    // `ContainerNr < NARRAY(...)` and otherwise do nothing at all. The
    // sixteen legal numbers are exactly the ones this client already
    // indexes.
    // A compile-time fact belongs in a static_assert: MSVC at /W4 /WX
    // rejects a constant condition inside the runtime CHECK macro (C4127).
    static_assert(kMaxOpenContainers == 16, "cr.hh:917 Object OpenContainer[16]");
    for (std::uint8_t container = 0; container < kMaxOpenContainers; ++container) {
        CHECK(BuildCloseContainerCommand(container)[1] == container);
        CHECK(BuildUpContainerCommand(container)[1] == container);
    }

    // A number past the table is still expressible: the server ignores it
    // silently, and refusing locally would invent a rule Fusion32 applies
    // itself. The byte is passed through untouched, which is what a caller
    // asserting against a capture needs.
    for (const std::uint8_t container : {std::uint8_t{16}, std::uint8_t{64},
                                         std::uint8_t{255}}) {
        CHECK(BuildCloseContainerCommand(container).size() == 2);
        CHECK(BuildCloseContainerCommand(container)[1] == container);
        CHECK(BuildUpContainerCommand(container)[1] == container);
    }
    CHECK(Hex(BuildCloseContainerCommand(255)) == "87ff");
    CHECK(Hex(BuildUpContainerCommand(255)) == "88ff");

    std::cout << "  container number boundaries, all 16 plus past the table: PASS\n";
}

void EndpointBoundaries() {
    // The full word range survives in both coordinates, and the special
    // x = 0xFFFF form is not accidentally produced by a map position.
    {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::OnMap(MapPosition{0, 0, 0}), 0, 0);
        CHECK(Hex(command) == "850000000000000000");
    }
    {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::OnMap(MapPosition{0xFFFE, 0xFFFE, 15}), 0xFFFF, 255);
        CHECK(Hex(command) == "85fefffeff0fffffff");
    }
    // Every body slot and every container slot round-trips into the bytes.
    for (std::uint8_t slot = 0; slot < 11; ++slot) {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::InInventory(slot), 100, 0);
        CHECK(command[1] == 0xFF && command[2] == 0xFF);
        CHECK(command[3] == slot && command[4] == 0);
    }
    for (std::uint8_t container = 0; container < kMaxOpenContainers; ++container) {
        const auto command = BuildTurnObjectCommand(
            MoveEndpoint::InContainer(container, 7), 100, 7);
        CHECK(command[1] == 0xFF && command[2] == 0xFF);
        CHECK(command[3] == static_cast<std::uint8_t>(kContainerCoordinateFirst + container));
        CHECK(command[5] == 7);
    }

    std::cout << "  object reference boundaries: PASS\n";
}

}  // namespace

int main() {
    try {
        GoldenDiagonalCommands();
        DiagonalOffsetsMatchTheDispatcher();
        TripleDelayClassification();
        StepDelayArithmetic();
        GoldenActionCommands();
        ContainerNumberBoundaries();
        EndpointBoundaries();
    } catch (const std::exception& error) {
        std::cerr << "movement_actions_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "movement_actions_tests: PASS\n";
    return 0;
}
