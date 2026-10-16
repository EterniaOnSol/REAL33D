#include "../unreal/REAL33D/Source/REAL33D/Public/Real33DStackSprite.h"

#include <iostream>
#include <stdexcept>

#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)

using Real33D::StackSpriteFrame;
using Real33D::kStackSpriteTiers;
using Real33D::kStackSpriteTierCount;

namespace {

// Gold coin 3031 is the shape this was written against: pattern_x=4,
// pattern_y=2 in Tibia.dat, so eight pictures.
constexpr int kEight = 8;

void TestEveryTierBoundary()
{
	// Each tier must begin exactly at its own amount and not one earlier.
	for (int Index = 0; Index < kStackSpriteTierCount; ++Index)
	{
		const std::uint32_t Tier = kStackSpriteTiers[Index];
		CHECK(StackSpriteFrame(Tier, kEight) == Index);
		if (Tier > 1)
		{
			// One below a boundary still belongs to the previous picture.
			CHECK(StackSpriteFrame(Tier - 1, kEight) == Index - 1);
		}
	}

	// Spelled out, so a change to the table has to be deliberate rather than
	// quietly absorbed by the loop above.
	CHECK(StackSpriteFrame(1, kEight) == 0);
	CHECK(StackSpriteFrame(2, kEight) == 1);
	CHECK(StackSpriteFrame(3, kEight) == 2);
	CHECK(StackSpriteFrame(4, kEight) == 3);
	CHECK(StackSpriteFrame(5, kEight) == 4);
	CHECK(StackSpriteFrame(9, kEight) == 4);
	CHECK(StackSpriteFrame(10, kEight) == 5);
	CHECK(StackSpriteFrame(24, kEight) == 5);
	CHECK(StackSpriteFrame(25, kEight) == 6);
	CHECK(StackSpriteFrame(49, kEight) == 6);
	CHECK(StackSpriteFrame(50, kEight) == 7);
}

void TestCountOne()
{
	// The single-object picture, and the reason the defect went unnoticed:
	// every stack used to render as this one.
	CHECK(StackSpriteFrame(1, kEight) == 0);

	// Zero is not a different picture. The wire carries no amount for a
	// non-cumulative object and callers pass 0 rather than inventing a 1.
	CHECK(StackSpriteFrame(0, kEight) == 0);
}

void TestHighStackCount()
{
	// Above the last tier the biggest picture keeps applying; it must never
	// run off the end of the table.
	CHECK(StackSpriteFrame(51, kEight) == 7);
	CHECK(StackSpriteFrame(100, kEight) == 7);
	CHECK(StackSpriteFrame(255, kEight) == 7);          // the wire maximum, a u8
	CHECK(StackSpriteFrame(4294967295u, kEight) == 7);  // and far beyond it
}

void TestNonStackable()
{
	// One picture means there is nothing to choose, whatever the amount says.
	// A dagger (3267) and a short sword (3294) both declare a single sprite.
	for (std::uint32_t Amount : {0u, 1u, 2u, 5u, 50u, 255u})
	{
		CHECK(StackSpriteFrame(Amount, 1) == 0);
	}
}

void TestMalformedOrMissingPatternData()
{
	// A frame count that is absent, zero or nonsense must degrade to the
	// first picture rather than index out of bounds. Reading a corrupt dat
	// should cost a wrong picture at worst, never a crash.
	for (int Frames : {0, -1, -1000})
	{
		for (std::uint32_t Amount : {0u, 1u, 50u, 255u})
		{
			CHECK(StackSpriteFrame(Amount, Frames) == 0);
		}
	}

	// Fewer pictures than the table has tiers: the object stops early and its
	// last picture absorbs every larger amount.
	CHECK(StackSpriteFrame(1, 3) == 0);
	CHECK(StackSpriteFrame(2, 3) == 1);
	CHECK(StackSpriteFrame(3, 3) == 2);
	CHECK(StackSpriteFrame(4, 3) == 2);
	CHECK(StackSpriteFrame(255, 3) == 2);

	// Two pictures is the smallest real choice.
	CHECK(StackSpriteFrame(1, 2) == 0);
	CHECK(StackSpriteFrame(2, 2) == 1);
	CHECK(StackSpriteFrame(255, 2) == 1);

	// More pictures than tiers: the extra ones stay unaddressed, because no
	// evidence says what amount they would stand for. The result must still
	// be inside the object's own range.
	for (std::uint32_t Amount : {0u, 1u, 50u, 255u})
	{
		const int Frame = StackSpriteFrame(Amount, 16);
		CHECK(Frame >= 0 && Frame < 16);
		CHECK(Frame <= kStackSpriteTierCount - 1);
	}
}

void TestResultAlwaysInRange()
{
	// The property the callers depend on: whatever comes off the wire, the
	// index can be used to pick a picture without a bounds check.
	for (int Frames = 1; Frames <= 12; ++Frames)
	{
		for (std::uint32_t Amount = 0; Amount <= 300; ++Amount)
		{
			const int Frame = StackSpriteFrame(Amount, Frames);
			CHECK(Frame >= 0);
			CHECK(Frame < Frames);
		}
	}
}

void TestMonotonic()
{
	// More objects can never show a smaller pile.
	int Previous = 0;
	for (std::uint32_t Amount = 0; Amount <= 300; ++Amount)
	{
		const int Frame = StackSpriteFrame(Amount, kEight);
		CHECK(Frame >= Previous);
		Previous = Frame;
	}
}

} // namespace

int main()
{
	try
	{
		TestEveryTierBoundary();
		TestCountOne();
		TestHighStackCount();
		TestNonStackable();
		TestMalformedOrMissingPatternData();
		TestResultAlwaysInRange();
		TestMonotonic();
	}
	catch (const std::exception& error)
	{
		std::cerr << "real33d_stackable_sprite_tests FAILED: " << error.what() << "\n";
		return 1;
	}
	std::cout << "real33d_stackable_sprite_tests: PASS\n";
	return 0;
}
