#pragma once

#include "CoreMinimal.h"

/**
 * The single deterministic transform from a Fusion32 map position to an Unreal
 * world position. Nothing else in this module may invent one.
 *
 * Scale, from visual/docs/TECHNICAL_STANDARD.md:
 *     1 SQM = 100 Unreal Units
 *
 * Axes follow the server. reference/game/src/receiving.cc::ReceiveData
 * dispatches CGoDirection with (0,-1) for north and (+1,0) for east, so in
 * Tibia +x is east and +y is south. That maps to Unreal +X east, +Y south,
 * +Z up.
 *
 * Floors are inverted: Tibia z=0 is the highest floor and z=15 the deepest, so
 * a smaller z must sit higher in Unreal.
 *
 * FLOOR HEIGHT IS A PRESENTATION CHOICE. The protocol fixes the horizontal
 * offset per floor but never states a vertical distance, which
 * TECHNICAL_STANDARD.md records as UNRESOLVED. This slice uses 100 uu, one
 * SQM, purely so multi-floor scenes are legible. It is not derived from the
 * data and must not be treated as such.
 *
 * ORIGIN. Tibia coordinates sit around 32,000, which at 100 uu is 3,200,000 uu.
 * Single-precision floats carry about seven significant digits, so at that
 * magnitude the representable step is roughly a quarter of a unit and jitter
 * becomes visible. Everything is therefore placed relative to an origin fixed
 * once, at the first anchor the session receives, keeping coordinates within a
 * few thousand units of zero.
 *
 * The logical position is always the WorldState position. This converts for
 * drawing; it never decides anything.
 */
namespace Real33D
{
	/** Unreal units per map field. */
	inline constexpr double UnitsPerSqm = 100.0;

	/** Presentation-only, see the note above. */
	inline constexpr double UnitsPerFloor = 100.0;

	/** A Fusion32 map position, mirrored here so the header needs no ClientCore include. */
	struct FMapPosition
	{
		int32 X = 0;
		int32 Y = 0;
		int32 Z = 0;

		bool operator==(const FMapPosition& Other) const
		{
			return X == Other.X && Y == Other.Y && Z == Other.Z;
		}
		bool operator!=(const FMapPosition& Other) const { return !(*this == Other); }
	};

	FORCEINLINE uint32 GetTypeHash(const FMapPosition& Position)
	{
		return HashCombine(HashCombine(::GetTypeHash(Position.X), ::GetTypeHash(Position.Y)),
			::GetTypeHash(Position.Z));
	}

	/**
	 * Map position as an engine type, for use as a UPROPERTY container key.
	 *
	 * Unreal Header Tool only accepts types it knows about, and FMapPosition is
	 * a plain struct on purpose: it must stay usable on the worker thread and in
	 * headers that no reflected code includes. FIntVector carries the same three
	 * integers with no loss.
	 */
	FORCEINLINE FIntVector ToKey(const FMapPosition& Position)
	{
		return FIntVector(Position.X, Position.Y, Position.Z);
	}

	FORCEINLINE FMapPosition FromKey(const FIntVector& Key)
	{
		return FMapPosition{ Key.X, Key.Y, Key.Z };
	}

	/** Fixed once per session, at the first anchor received. */
	struct FWorldOrigin
	{
		FMapPosition Position;
		bool bSet = false;

		void EnsureSet(const FMapPosition& Anchor)
		{
			if (!bSet)
			{
				Position = Anchor;
				bSet = true;
			}
		}
	};

	/** Map position to Unreal world position, at the centre of the field's floor plane. */
	FORCEINLINE FVector ToWorld(const FWorldOrigin& Origin, const FMapPosition& Position)
	{
		const double X = static_cast<double>(Position.X - Origin.Position.X) * UnitsPerSqm;
		const double Y = static_cast<double>(Position.Y - Origin.Position.Y) * UnitsPerSqm;
		const double Z = static_cast<double>(Origin.Position.Z - Position.Z) * UnitsPerFloor;
		return FVector(X, Y, Z);
	}

	/**
	 * Creature facing. reference/game/src/enums.hh declares
	 * DIRECTION_NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3.
	 * Unreal yaw 0 looks along +X, which is east here.
	 */
	FORCEINLINE FRotator ToRotation(uint8 Direction)
	{
		switch (Direction)
		{
		case 0:  return FRotator(0.0, 270.0, 0.0);   // north, -Y
		case 1:  return FRotator(0.0, 0.0, 0.0);     // east, +X
		case 2:  return FRotator(0.0, 90.0, 0.0);    // south, +Y
		case 3:  return FRotator(0.0, 180.0, 0.0);   // west, -X
		default: return FRotator::ZeroRotator;
		}
	}
}
