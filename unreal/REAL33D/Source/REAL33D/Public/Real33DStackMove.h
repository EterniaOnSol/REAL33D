#pragma once

#include <cstdint>

// How many of a stack a drag asks Fusion32 to move.
//
// THE ONE PLACE THAT DECIDES. All four drag routes in this client -- slot to
// slot, slot to map, map to slot and map to map -- end in `DecideStackMove`,
// so the gesture cannot mean one thing in the inventory panel and another on
// the ground, and the rule can be proven without a renderer or a server.
//
// Nothing here moves anything. A decision becomes either one
// CL_CMD_MOVE_OBJECT with a count byte, or one question put to the player, and
// a question the player cancels sends nothing at all. The object stays drawn
// where Fusion32 last said it was until Fusion32 says otherwise, exactly as a
// walk request changes no position.
//
// Deliberately free of Unreal types: tests/real33d_stack_move_tests.cpp
// compiles it with a plain C++ toolchain, the same way the other
// tests/real33d_*.cpp suites do.
namespace Real33D
{

/**
 * The widest count the wire can carry.
 *
 * CL_CMD_MOVE_OBJECT ends in a single byte -- `reference/game/src/receiving.cc`
 * CMoveObject reads `Count` with the one-byte reader, and
 * `BuildMoveObjectCommand` writes one byte -- so 255 is the ceiling the
 * protocol imposes whatever a stack claims to hold. 7.72 piles are far smaller
 * than that; this is the bound of the field, not a guess at the game's limit.
 */
inline constexpr std::uint32_t kMaxWireStackCount = 255;

/** The floor. CMoveObject refuses a cumulative object with a count of zero. */
inline constexpr std::uint32_t kMinStackCount = 1;

/** What a completed drag turns into. */
enum class StackMoveKind
{
	/** Put one move on the wire, carrying `StackMoveDecision::Count`. */
	Send,
	/** Ask the player how many first. Nothing reaches the wire yet. */
	Ask,
};

struct StackMoveDecision
{
	StackMoveKind Kind = StackMoveKind::Send;

	/**
	 * For `Send`, the count byte to put on the wire.
	 * For `Ask`, the largest amount the question may answer, and the amount it
	 * opens on -- the whole stack, so a player who just presses OK gets the
	 * same result an unmodified drag would have given them.
	 *
	 * Never zero, and never above `kMaxWireStackCount`.
	 */
	std::uint32_t Count = kMinStackCount;
};

/** 1..StackCount, with zero, overflow and a byte-sized wire all folded in. */
inline std::uint32_t ClampStackAmount(std::int64_t Requested, std::uint32_t StackCount)
{
	// A stack the server described as holding nothing is still one object: the
	// amount byte is absent for a non-cumulative object and callers pass 0
	// rather than inventing a 1, and CMoveObject refuses a count of zero.
	std::uint32_t Ceiling = StackCount < kMinStackCount ? kMinStackCount : StackCount;
	if (Ceiling > kMaxWireStackCount)
	{
		Ceiling = kMaxWireStackCount;
	}
	if (Requested < static_cast<std::int64_t>(kMinStackCount))
	{
		return kMinStackCount;
	}
	if (Requested > static_cast<std::int64_t>(Ceiling))
	{
		return Ceiling;
	}
	return static_cast<std::uint32_t>(Requested);
}

/**
 * The gesture, as this milestone's task states it.
 *
 *   plain drag   the whole stack
 *   Shift+drag   one item
 *   Ctrl+drag    ask the player how many
 *
 * @param bStackable whether Fusion32 sent an amount for this object at all,
 *                   which is what makes it a stack. Read from the wire
 *                   (`bHasAmount`), never inferred from a type id: the amount
 *                   byte is present only for a type carrying the CUMULATIVE
 *                   flag. A non-stackable object ignores both modifiers and
 *                   always moves as one whole object, which is the behaviour
 *                   this client already had and must keep -- a sword has no
 *                   partial amount to ask about.
 * @param StackCount the authoritative amount the server last reported. Zero is
 *                   read as one; see ClampStackAmount.
 *
 * CTRL WINS WHEN BOTH MODIFIERS ARE HELD, and the asymmetry is deliberate. A
 * player who meant Shift and brushed Ctrl gets a question they can cancel; a
 * player who meant Ctrl and brushed Shift would otherwise have a single item
 * moved silently, with nothing to cancel. Only one of those two mistakes is
 * recoverable, so the fold goes that way.
 */
inline StackMoveDecision DecideStackMove(bool bStackable, std::uint32_t StackCount,
	bool bShift, bool bCtrl)
{
	StackMoveDecision Decision;
	if (!bStackable)
	{
		// Preserved exactly: one whole object, whatever is being held down.
		Decision.Kind = StackMoveKind::Send;
		Decision.Count = kMinStackCount;
		return Decision;
	}
	if (bCtrl)
	{
		Decision.Kind = StackMoveKind::Ask;
		Decision.Count = ClampStackAmount(static_cast<std::int64_t>(StackCount), StackCount);
		return Decision;
	}
	Decision.Kind = StackMoveKind::Send;
	Decision.Count = bShift
		? kMinStackCount
		: ClampStackAmount(static_cast<std::int64_t>(StackCount), StackCount);
	return Decision;
}

/**
 * How far a stepper in the amount question moves the answer.
 *
 * Presentation only, and listed here so the panel and its tests agree on one
 * set. The coarse step exists so a pile of a hundred is not forty clicks away.
 */
inline constexpr std::int32_t kStackAmountFineStep = 1;
inline constexpr std::int32_t kStackAmountCoarseStep = 10;

/** Applies a stepper to the amount currently showing, clamped as above. */
inline std::uint32_t StepStackAmount(std::uint32_t Current, std::int32_t Delta,
	std::uint32_t StackCount)
{
	return ClampStackAmount(static_cast<std::int64_t>(Current)
		+ static_cast<std::int64_t>(Delta), StackCount);
}

} // namespace Real33D
