#pragma once

#include <cstdint>

// Which picture a stackable object shows for a given amount.
//
// A cumulative object in Tibia 7.72 carries several pictures, not one. Gold
// coin 3031 declares pattern_x=4, pattern_y=2 in Tibia.dat, so it owns eight
// sprites (410..417), and the client picks one from the amount on the wire.
// Reading only the first pattern draws a single coin for a pile of fifty,
// which is the defect this header exists to fix.
//
// Deliberately free of Unreal types: tests/real33d_stackable_sprite_tests.cpp
// compiles it with a plain C++ toolchain, the same way the other
// tests/real33d_*.cpp suites do.
namespace Real33D
{

// The amount each successive picture stands for.
//
// Provenance, because the three parts of this table have different strengths
// and a later reader deserves to know which is which:
//
//  * The *count* of pictures is read from the data, never assumed: it is
//    pattern_x * pattern_y for the object in Tibia.dat.
//  * Tiers 1..5 are confirmed by the art itself. Decoding sprites 410..417 of
//    object 3031 and looking at them shows literally one, two, three and four
//    coins, then a short stack beside a loose coin for the fifth.
//  * Tiers 10, 25 and 50 are the documented 7.72 step values. The art only
//    shows three progressively larger mounds, so it fixes their *order* but
//    not their thresholds. Those three numbers are therefore UNVERIFIED
//    against the client binary: they are not contradicted by anything here,
//    and nothing in the server data defines them, because this is a client
//    presentation rule and Fusion32 only ever sends the amount.
//
// Never keyed to a type id. Which objects are cumulative comes from the data
// (Tibia.dat attribute 0x05, whose server counterpart is the `Cumulative`
// flag in objects.srv), so a new stackable needs no change here.
inline constexpr std::uint32_t kStackSpriteTiers[] = {1, 2, 3, 4, 5, 10, 25, 50};
inline constexpr int kStackSpriteTierCount =
    static_cast<int>(sizeof(kStackSpriteTiers) / sizeof(kStackSpriteTiers[0]));

/**
 * Picture index for an amount, given how many pictures the object really has.
 *
 * @param Amount     the authoritative count from the server. Zero and one are
 *                   both the single-item picture: the wire carries no amount
 *                   at all for a non-cumulative object, and callers pass 0
 *                   rather than inventing a 1.
 * @param FrameCount how many pictures the object declares. One or fewer means
 *                   there is nothing to choose between.
 *
 * @return an index in [0, FrameCount), always safe to use.
 *
 * Generic in FrameCount on purpose. An object with fewer pictures than the
 * table has tiers simply stops early and its last picture covers everything
 * above it; an object with more pictures than the table has tiers cannot have
 * them addressed, because no evidence here says what amount they would stand
 * for, and inventing thresholds would be worse than leaving them unused.
 */
inline int StackSpriteFrame(std::uint32_t Amount, int FrameCount)
{
	if (FrameCount <= 1)
	{
		return 0;
	}

	const int Usable = FrameCount < kStackSpriteTierCount ? FrameCount : kStackSpriteTierCount;

	// An amount below the first tier is still one object, not none: a tile
	// holding a single coin is reported either with amount 1 or, for a
	// non-cumulative object, with no amount at all.
	int Frame = 0;
	for (int Index = 0; Index < Usable; ++Index)
	{
		if (Amount >= kStackSpriteTiers[Index])
		{
			Frame = Index;
		}
	}
	return Frame;
}

} // namespace Real33D
