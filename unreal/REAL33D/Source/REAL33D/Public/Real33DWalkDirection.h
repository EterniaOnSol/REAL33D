#pragma once

#include <cstdint>

// How this client numbers the eight walk directions it can ask Fusion32 for.
//
// 7.72 has two step shapes and they are separate commands on the wire:
// CL_CMD_GO_NORTH..WEST (101..104) and CL_CMD_GO_NORTHEAST..NORTHWEST
// (106..109). The server's own `enums.hh` numbers only the cardinals, 0..3,
// and names nothing for the diagonals, so the 4..7 half of this table is THIS
// CLIENT'S OWN numbering for its own events and logs. It is never put on the
// wire: the worker turns it back into a `CardinalDirection` or a
// `DiagonalDirection` and lets ClientCore build the command.
//
// It exists because the direction travelled through the bridge as a bare
// `uint8` that two different files then printed from two copies of a
// four-entry table. Adding four more directions to two copies is how they
// drift, and a direction printed as "?" in evidence is a direction nobody can
// audit.
//
// Deliberately free of Unreal types: tests/real33d_walk_direction_tests.cpp
// compiles it with a plain C++ toolchain, like the other tests/real33d_*.cpp
// suites.
namespace Real33D
{

/** `enums.hh` order, and the values the bridge has always used. */
inline constexpr std::uint8_t kWalkNorth = 0;
inline constexpr std::uint8_t kWalkEast = 1;
inline constexpr std::uint8_t kWalkSouth = 2;
inline constexpr std::uint8_t kWalkWest = 3;

/**
 * The four diagonals, in `DiagonalDirection` order, offset past the cardinals.
 *
 * The offset is what keeps an existing cardinal value meaning exactly what it
 * always meant: every reader that only understands 0..3 still sees 0..3 for a
 * cardinal step and sees something it knows is not one for a diagonal.
 */
inline constexpr std::uint8_t kWalkDiagonalBase = 4;
inline constexpr std::uint8_t kWalkNortheast = 4;
inline constexpr std::uint8_t kWalkSoutheast = 5;
inline constexpr std::uint8_t kWalkSouthwest = 6;
inline constexpr std::uint8_t kWalkNorthwest = 7;

inline constexpr std::uint8_t kWalkDirectionCount = 8;

/** The bridge's "this event carries no direction". Mirrored, not redefined. */
inline constexpr std::uint8_t kNoWalkDirection = 0xFF;

/** True for 4..7. False for a cardinal and for anything out of range. */
inline constexpr bool IsDiagonalWalk(std::uint8_t Direction)
{
	return Direction >= kWalkDiagonalBase && Direction < kWalkDirectionCount;
}

inline constexpr bool IsCardinalWalk(std::uint8_t Direction)
{
	return Direction < kWalkDiagonalBase;
}

/** A `DiagonalDirection` value (0..3) as this table numbers it. */
inline constexpr std::uint8_t WalkDirectionFromDiagonal(std::uint8_t Diagonal)
{
	return static_cast<std::uint8_t>(kWalkDiagonalBase + Diagonal);
}

/** The inverse. Only meaningful when IsDiagonalWalk is true. */
inline constexpr std::uint8_t DiagonalFromWalkDirection(std::uint8_t Direction)
{
	return static_cast<std::uint8_t>(Direction - kWalkDiagonalBase);
}

/**
 * The direction's name, for logs and evidence. Never null.
 *
 * Out of range answers "none" rather than "?": an event that carries no
 * direction is an ordinary thing (a say, a move, a look), and printing a
 * question mark for it reads like a decoding failure.
 */
inline const char* WalkDirectionName(std::uint8_t Direction)
{
	switch (Direction)
	{
		case kWalkNorth:     return "north";
		case kWalkEast:      return "east";
		case kWalkSouth:     return "south";
		case kWalkWest:      return "west";
		case kWalkNortheast: return "northeast";
		case kWalkSoutheast: return "southeast";
		case kWalkSouthwest: return "southwest";
		case kWalkNorthwest: return "northwest";
		default:             return "none";
	}
}

/**
 * Where a camera-relative diagonal actually points.
 *
 * WASD in this client is camera-relative: `AReal33DWorld::CameraRelativeDirection`
 * adds the relative direction to whichever cardinal the camera is facing. The
 * four diagonal keys sit around WASD and must obey the same rule, or W walks
 * forward while Q walks absolute northwest and the keys beside each other
 * disagree about what the player is looking at. Walking south while facing
 * south and pressing Q has to carry on southwards.
 *
 * `FacingCardinal` is 0 north, 1 east, 2 south, 3 west -- the server's own
 * `enums.hh` order, which is clockwise.
 *
 * `RelativeDiagonal` is clockwise from the forward-right corner:
 *   0 forward-right, 1 back-right, 2 back-left, 3 forward-left.
 *
 * `DiagonalDirection` is clockwise from northeast too -- NE 0, SE 1, SW 2,
 * NW 3 -- and the corner between cardinal C and cardinal C+1 is diagonal C.
 * That is the whole reason this is one addition rather than a 16-entry table:
 * both enumerations already turn the same way.
 *
 * Returns a `DiagonalDirection` value, 0..3.
 */
inline constexpr std::uint8_t CameraRelativeDiagonal(std::uint8_t FacingCardinal,
	std::uint8_t RelativeDiagonal)
{
	return static_cast<std::uint8_t>((FacingCardinal + RelativeDiagonal) % 4);
}

/** The four diagonal keys, as relative corners. Q E Z C around WASD. */
inline constexpr std::uint8_t kRelativeDiagonalForwardRight = 0;  // E
inline constexpr std::uint8_t kRelativeDiagonalBackRight = 1;     // C
inline constexpr std::uint8_t kRelativeDiagonalBackLeft = 2;      // Z
inline constexpr std::uint8_t kRelativeDiagonalForwardLeft = 3;   // Q

/**
 * The field a step in this direction asks for, as an offset.
 *
 * y grows southward, which is why north subtracts. These are the same offsets
 * `receiving.cc::ReceiveData` hands to CGoDirection, and the diagonal half
 * matches `DiagonalStepPosition` exactly; the ClientCore suite asserts that
 * agreement rather than this header asserting it about itself.
 *
 * Returns false and leaves the offsets alone for a direction out of range.
 */
inline bool WalkDirectionOffset(std::uint8_t Direction, int& OutDx, int& OutDy)
{
	switch (Direction)
	{
		case kWalkNorth:     OutDx =  0; OutDy = -1; return true;
		case kWalkEast:      OutDx =  1; OutDy =  0; return true;
		case kWalkSouth:     OutDx =  0; OutDy =  1; return true;
		case kWalkWest:      OutDx = -1; OutDy =  0; return true;
		case kWalkNortheast: OutDx =  1; OutDy = -1; return true;
		case kWalkSoutheast: OutDx =  1; OutDy =  1; return true;
		case kWalkSouthwest: OutDx = -1; OutDy =  1; return true;
		case kWalkNorthwest: OutDx = -1; OutDy = -1; return true;
		default: return false;
	}
}

} // namespace Real33D
