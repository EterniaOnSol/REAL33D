// How this client numbers the eight walk directions, without Unreal.
//
// The wire values are not here and are not this header's business: the worker
// turns these back into a CardinalDirection or a DiagonalDirection and
// ClientCore builds the command. What these cases fix is the numbering, the
// offsets and the names -- the three things two separate files used to keep
// their own copies of, and the reason a direction could be printed as "?".
//
// The offsets are the ones receiving.cc::ReceiveData hands to CGoDirection:
// Northeast (1, -1), Southeast (1, 1), Southwest (-1, 1), Northwest (-1, -1),
// with y growing southward.

#include "Real33DWalkDirection.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

namespace
{
using namespace Real33D;

void TheCardinalsKeepTheServersOwnNumbers()
{
	// enums.hh: DIRECTION_NORTH 0, EAST 1, SOUTH 2, WEST 3. These values
	// travelled through the bridge long before diagonals existed and must not
	// have moved, or every existing journal would be misread.
	assert(kWalkNorth == 0);
	assert(kWalkEast == 1);
	assert(kWalkSouth == 2);
	assert(kWalkWest == 3);
}

void TheDiagonalsSitPastThemInDiagonalDirectionOrder()
{
	// DiagonalDirection: Northeast 0, Southeast 1, Southwest 2, Northwest 3.
	assert(kWalkNortheast == WalkDirectionFromDiagonal(0));
	assert(kWalkSoutheast == WalkDirectionFromDiagonal(1));
	assert(kWalkSouthwest == WalkDirectionFromDiagonal(2));
	assert(kWalkNorthwest == WalkDirectionFromDiagonal(3));
	assert(kWalkDirectionCount == 8);
}

void TheTwoHalvesNeverOverlap()
{
	for (std::uint8_t Direction = 0; Direction < kWalkDirectionCount; ++Direction)
	{
		assert(IsCardinalWalk(Direction) != IsDiagonalWalk(Direction));
	}
	// Out of range is neither, so an event carrying no direction cannot be
	// mistaken for a step in one.
	assert(!IsCardinalWalk(kNoWalkDirection));
	assert(!IsDiagonalWalk(kNoWalkDirection));
	assert(!IsDiagonalWalk(kWalkDirectionCount));
}

void TheDiagonalRoundTripHolds()
{
	for (std::uint8_t Diagonal = 0; Diagonal < 4; ++Diagonal)
	{
		const std::uint8_t Direction = WalkDirectionFromDiagonal(Diagonal);
		assert(IsDiagonalWalk(Direction));
		assert(DiagonalFromWalkDirection(Direction) == Diagonal);
	}
}

void EveryDirectionIsNamedAndNoNameRepeats()
{
	const char* Seen[kWalkDirectionCount] = {};
	for (std::uint8_t Direction = 0; Direction < kWalkDirectionCount; ++Direction)
	{
		const std::string Name = WalkDirectionName(Direction);
		assert(!Name.empty());
		// A direction that printed "none" would be a hole in the table.
		assert(Name != "none");
		for (std::uint8_t Earlier = 0; Earlier < Direction; ++Earlier)
		{
			assert(Name != std::string(Seen[Earlier]));
		}
		Seen[Direction] = WalkDirectionName(Direction);
	}
	assert(std::string(WalkDirectionName(kNoWalkDirection)) == "none");
	assert(std::string(WalkDirectionName(kWalkDirectionCount)) == "none");
}

void TheNamesAreTheCompassOnesTheTaskAsksFor()
{
	assert(std::string(WalkDirectionName(kWalkNorthwest)) == "northwest");
	assert(std::string(WalkDirectionName(kWalkNortheast)) == "northeast");
	assert(std::string(WalkDirectionName(kWalkSouthwest)) == "southwest");
	assert(std::string(WalkDirectionName(kWalkSoutheast)) == "southeast");
}

void EveryOffsetIsTheDispatchersOwn()
{
	int Dx = 0;
	int Dy = 0;

	assert(WalkDirectionOffset(kWalkNorth, Dx, Dy) && Dx == 0 && Dy == -1);
	assert(WalkDirectionOffset(kWalkEast, Dx, Dy) && Dx == 1 && Dy == 0);
	assert(WalkDirectionOffset(kWalkSouth, Dx, Dy) && Dx == 0 && Dy == 1);
	assert(WalkDirectionOffset(kWalkWest, Dx, Dy) && Dx == -1 && Dy == 0);

	assert(WalkDirectionOffset(kWalkNortheast, Dx, Dy) && Dx == 1 && Dy == -1);
	assert(WalkDirectionOffset(kWalkSoutheast, Dx, Dy) && Dx == 1 && Dy == 1);
	assert(WalkDirectionOffset(kWalkSouthwest, Dx, Dy) && Dx == -1 && Dy == 1);
	assert(WalkDirectionOffset(kWalkNorthwest, Dx, Dy) && Dx == -1 && Dy == -1);
}

void ADiagonalAlwaysMovesBothAxesAndACardinalExactlyOne()
{
	for (std::uint8_t Direction = 0; Direction < kWalkDirectionCount; ++Direction)
	{
		int Dx = 0;
		int Dy = 0;
		assert(WalkDirectionOffset(Direction, Dx, Dy));
		const int Moved = (Dx != 0 ? 1 : 0) + (Dy != 0 ? 1 : 0);
		assert(Moved == (IsDiagonalWalk(Direction) ? 2 : 1));
		// And no step is ever longer than one field on either axis.
		assert(Dx >= -1 && Dx <= 1);
		assert(Dy >= -1 && Dy <= 1);
	}
}

void AnOutOfRangeDirectionHasNoOffset()
{
	int Dx = 99;
	int Dy = 99;
	assert(!WalkDirectionOffset(kNoWalkDirection, Dx, Dy));
	// Left alone rather than zeroed: a caller must not read a step out of a
	// direction that does not name one.
	assert(Dx == 99 && Dy == 99);
	assert(!WalkDirectionOffset(kWalkDirectionCount, Dx, Dy));
}

// ----------------------------------------------------- camera-relative corners

void FacingNorthReproducesTheTasksNamedTable()
{
	// The default camera faces north, and there the four keys are exactly the
	// compass directions the milestone names. Camera-relative and the stated
	// table are the same thing at the default orientation, which is also why
	// W means north there.
	constexpr std::uint8_t North = 0;
	assert(CameraRelativeDiagonal(North, kRelativeDiagonalForwardLeft) == 3);   // Q -> NW
	assert(CameraRelativeDiagonal(North, kRelativeDiagonalForwardRight) == 0);  // E -> NE
	assert(CameraRelativeDiagonal(North, kRelativeDiagonalBackLeft) == 2);      // Z -> SW
	assert(CameraRelativeDiagonal(North, kRelativeDiagonalBackRight) == 1);     // C -> SE
}

void FacingSouthSendsTheForwardKeysSouthward()
{
	// The live defect this fixes: walking south while facing south, Q and E
	// went NORTH because the diagonals were absolute while WASD was relative.
	// Facing south, left is east and right is west, so forward-left is
	// southeast and forward-right is southwest -- both carry on southwards.
	constexpr std::uint8_t South = 2;
	const std::uint8_t Q = CameraRelativeDiagonal(South, kRelativeDiagonalForwardLeft);
	const std::uint8_t E = CameraRelativeDiagonal(South, kRelativeDiagonalForwardRight);
	assert(Q == 1);  // southeast
	assert(E == 2);  // southwest

	int Dx = 0;
	int Dy = 0;
	assert(WalkDirectionOffset(WalkDirectionFromDiagonal(Q), Dx, Dy));
	assert(Dy > 0);  // southward, y grows south
	assert(WalkDirectionOffset(WalkDirectionFromDiagonal(E), Dx, Dy));
	assert(Dy > 0);

	// And the back keys then point north, which is the other half of the same
	// statement.
	const std::uint8_t Z = CameraRelativeDiagonal(South, kRelativeDiagonalBackLeft);
	const std::uint8_t C = CameraRelativeDiagonal(South, kRelativeDiagonalBackRight);
	assert(WalkDirectionOffset(WalkDirectionFromDiagonal(Z), Dx, Dy));
	assert(Dy < 0);
	assert(WalkDirectionOffset(WalkDirectionFromDiagonal(C), Dx, Dy));
	assert(Dy < 0);
}

void AForwardKeyAlwaysCarriesTheFacingAxis()
{
	// The general rule, for every orientation: the two forward corners always
	// move the way the camera is looking, and the two back corners always move
	// against it. This is what "same gate and same feel as WASD" means.
	for (std::uint8_t Facing = 0; Facing < 4; ++Facing)
	{
		int Fx = 0;
		int Fy = 0;
		assert(WalkDirectionOffset(Facing, Fx, Fy));  // the facing cardinal itself

		for (const std::uint8_t Forward : {kRelativeDiagonalForwardLeft,
				kRelativeDiagonalForwardRight})
		{
			int Dx = 0;
			int Dy = 0;
			const std::uint8_t Absolute =
				WalkDirectionFromDiagonal(CameraRelativeDiagonal(Facing, Forward));
			assert(WalkDirectionOffset(Absolute, Dx, Dy));
			// Agrees with the facing cardinal on whichever axis that cardinal
			// moves along.
			assert(Fx != 0 ? Dx == Fx : Dy == Fy);
		}
		for (const std::uint8_t Back : {kRelativeDiagonalBackLeft,
				kRelativeDiagonalBackRight})
		{
			int Dx = 0;
			int Dy = 0;
			const std::uint8_t Absolute =
				WalkDirectionFromDiagonal(CameraRelativeDiagonal(Facing, Back));
			assert(WalkDirectionOffset(Absolute, Dx, Dy));
			assert(Fx != 0 ? Dx == -Fx : Dy == -Fy);
		}
	}
}

void EveryOrientationOffersAllFourDiagonalsExactlyOnce()
{
	for (std::uint8_t Facing = 0; Facing < 4; ++Facing)
	{
		bool Seen[4] = {};
		for (std::uint8_t Relative = 0; Relative < 4; ++Relative)
		{
			const std::uint8_t Absolute = CameraRelativeDiagonal(Facing, Relative);
			assert(Absolute < 4);
			assert(!Seen[Absolute]);
			Seen[Absolute] = true;
		}
		// No orientation can leave a diagonal unreachable.
		for (const bool Reached : Seen)
		{
			assert(Reached);
		}
	}
}

void TheFourBoundKeysMapToTheFourDiagonals()
{
	// The milestone's own binding table, written down where it can be checked:
	// Q northwest, E northeast, Z southwest, C southeast.
	assert(std::string(WalkDirectionName(kWalkNorthwest)) == "northwest");  // Q
	assert(std::string(WalkDirectionName(kWalkNortheast)) == "northeast");  // E
	assert(std::string(WalkDirectionName(kWalkSouthwest)) == "southwest");  // Z
	assert(std::string(WalkDirectionName(kWalkSoutheast)) == "southeast");  // C

	// Q and C are opposite corners, as are E and Z. If any two of the four
	// ever collapsed onto one direction this is what would catch it.
	int Qx = 0, Qy = 0, Cx = 0, Cy = 0, Ex = 0, Ey = 0, Zx = 0, Zy = 0;
	WalkDirectionOffset(kWalkNorthwest, Qx, Qy);
	WalkDirectionOffset(kWalkSoutheast, Cx, Cy);
	WalkDirectionOffset(kWalkNortheast, Ex, Ey);
	WalkDirectionOffset(kWalkSouthwest, Zx, Zy);
	assert(Qx == -Cx && Qy == -Cy);
	assert(Ex == -Zx && Ey == -Zy);
}

} // namespace

int main()
{
	TheCardinalsKeepTheServersOwnNumbers();
	TheDiagonalsSitPastThemInDiagonalDirectionOrder();
	TheTwoHalvesNeverOverlap();
	TheDiagonalRoundTripHolds();

	EveryDirectionIsNamedAndNoNameRepeats();
	TheNamesAreTheCompassOnesTheTaskAsksFor();

	EveryOffsetIsTheDispatchersOwn();
	ADiagonalAlwaysMovesBothAxesAndACardinalExactlyOne();
	AnOutOfRangeDirectionHasNoOffset();

	FacingNorthReproducesTheTasksNamedTable();
	FacingSouthSendsTheForwardKeysSouthward();
	AForwardKeyAlwaysCarriesTheFacingAxis();
	EveryOrientationOffersAllFourDiagonalsExactlyOnce();

	TheFourBoundKeysMapToTheFourDiagonals();

	std::cout << "real33d walk direction tests: 14 cases passed\n";
	return 0;
}
