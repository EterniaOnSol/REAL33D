// The stack-move gesture's rules, without Slate, without a bridge and without
// a server.
//
// What a live run then has to prove is that Fusion32 accepts the counts these
// rules produce and that the authoritative inventory and container updates
// which follow carry them. Nothing here can show that, and nothing here
// pretends to: these cases fix the arithmetic and the gesture, which is the
// part a compiler can check.
//
// The three gestures are the ones UNREAL-STACK-MOVE-CONTROLS-001 states:
// plain drag moves the whole stack, Shift+drag moves one, Ctrl+drag asks. The
// bounds are the protocol's: `reference/game/src/receiving.cc` CMoveObject
// reads one count byte and refuses a cumulative object whose count is zero.

#include "Real33DStackMove.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace
{
using Real33D::ClampStackAmount;
using Real33D::DecideStackMove;
using Real33D::kMaxWireStackCount;
using Real33D::kMinStackCount;
using Real33D::kStackAmountCoarseStep;
using Real33D::kStackAmountFineStep;
using Real33D::StackMoveDecision;
using Real33D::StackMoveKind;
using Real33D::StepStackAmount;

constexpr bool kStackable = true;
constexpr bool kPlain = false;

// --------------------------------------------------------------- the gesture

void APlainDragMovesTheWholeStack()
{
	for (const std::uint32_t Amount : {1u, 2u, 10u, 37u, 100u})
	{
		const StackMoveDecision Decision = DecideStackMove(kStackable, Amount, false, false);
		assert(Decision.Kind == StackMoveKind::Send);
		assert(Decision.Count == Amount);
	}
}

void ShiftDragMovesExactlyOne()
{
	for (const std::uint32_t Amount : {1u, 2u, 10u, 37u, 100u})
	{
		const StackMoveDecision Decision = DecideStackMove(kStackable, Amount, true, false);
		assert(Decision.Kind == StackMoveKind::Send);
		assert(Decision.Count == kMinStackCount);
	}
}

void CtrlDragAsksAndOpensOnTheWholeStack()
{
	for (const std::uint32_t Amount : {1u, 2u, 10u, 37u, 100u})
	{
		const StackMoveDecision Decision = DecideStackMove(kStackable, Amount, false, true);
		assert(Decision.Kind == StackMoveKind::Ask);
		// The question's ceiling is the stack, and it opens there, so pressing
		// OK without touching anything equals the unmodified drag.
		assert(Decision.Count == Amount);
	}
}

void CtrlWinsOverShift()
{
	// Documented asymmetry: the question can still answer one, while a silent
	// single-item move could not have been taken back.
	const StackMoveDecision Decision = DecideStackMove(kStackable, 50, true, true);
	assert(Decision.Kind == StackMoveKind::Ask);
	assert(Decision.Count == 50);
}

// ------------------------------------------------- non-stackable is untouched

void ANonStackableObjectAlwaysMovesWhole()
{
	// The behaviour this client already had, and the one this milestone must
	// not change. A sword has no amount byte on the wire, so there is nothing
	// to ask about and no modifier may produce a partial move.
	for (const bool Shift : {false, true})
	{
		for (const bool Ctrl : {false, true})
		{
			const StackMoveDecision Decision = DecideStackMove(kPlain, 0, Shift, Ctrl);
			assert(Decision.Kind == StackMoveKind::Send);
			assert(Decision.Count == kMinStackCount);
		}
	}
}

void ANonStackableObjectIgnoresAnyAmountItWasGiven()
{
	// Defensive: a caller that passes a count for a non-cumulative object is
	// describing something the wire cannot carry, and it is dropped rather
	// than honoured.
	const StackMoveDecision Decision = DecideStackMove(kPlain, 64, false, false);
	assert(Decision.Kind == StackMoveKind::Send);
	assert(Decision.Count == kMinStackCount);
}

// ------------------------------------------------------------------- clamping

void TheQuestionClampsToOneAtTheBottom()
{
	assert(ClampStackAmount(0, 10) == 1);
	assert(ClampStackAmount(-1, 10) == 1);
	assert(ClampStackAmount(-1000000, 10) == 1);
	assert(ClampStackAmount(1, 10) == 1);
}

void TheQuestionClampsToTheStackAtTheTop()
{
	assert(ClampStackAmount(10, 10) == 10);
	assert(ClampStackAmount(11, 10) == 10);
	assert(ClampStackAmount(1000000, 10) == 10);
}

void EveryAmountInRangeSurvivesUnchanged()
{
	for (std::uint32_t Amount = 1; Amount <= 100; ++Amount)
	{
		assert(ClampStackAmount(static_cast<std::int64_t>(Amount), 100) == Amount);
	}
}

void AStackOfOneAdmitsExactlyOneAnswer()
{
	// The live case the task lists first. The range is [1,1], so the question
	// has one answer and the clamp proves it cannot produce another.
	for (const std::int64_t Requested : {-5, 0, 1, 2, 99})
	{
		assert(ClampStackAmount(Requested, 1) == 1);
	}
}

void AStackTheServerDescribedAsEmptyIsStillOneObject()
{
	assert(ClampStackAmount(0, 0) == 1);
	assert(ClampStackAmount(5, 0) == 1);
	const StackMoveDecision Decision = DecideStackMove(kStackable, 0, false, false);
	assert(Decision.Kind == StackMoveKind::Send);
	assert(Decision.Count == 1);
}

void NothingCanExceedTheOneCountByteOnTheWire()
{
	assert(ClampStackAmount(400, 400) == kMaxWireStackCount);
	const StackMoveDecision Decision = DecideStackMove(kStackable, 1000, false, false);
	assert(Decision.Count == kMaxWireStackCount);
	const StackMoveDecision Asked = DecideStackMove(kStackable, 1000, false, true);
	assert(Asked.Kind == StackMoveKind::Ask);
	assert(Asked.Count == kMaxWireStackCount);
}

// -------------------------------------------------------------- the steppers

void SteppersMoveTheAnswerAndStayInRange()
{
	assert(StepStackAmount(5, kStackAmountFineStep, 10) == 6);
	assert(StepStackAmount(5, -kStackAmountFineStep, 10) == 4);
	assert(StepStackAmount(5, kStackAmountCoarseStep, 10) == 10);
	assert(StepStackAmount(5, -kStackAmountCoarseStep, 10) == 1);
}

void SteppersCannotWalkOffEitherEnd()
{
	assert(StepStackAmount(10, kStackAmountFineStep, 10) == 10);
	assert(StepStackAmount(1, -kStackAmountFineStep, 10) == 1);
	assert(StepStackAmount(1, -kStackAmountCoarseStep, 10) == 1);
	assert(StepStackAmount(10, kStackAmountCoarseStep, 10) == 10);
}

void EveryReachableAnswerIsReachableWithTheFineStep()
{
	// A partial amount has to be selectable, which is the whole point of the
	// question. Walking the fine step from one end visits all of them.
	constexpr std::uint32_t Stack = 23;
	std::uint32_t Amount = 1;
	for (std::uint32_t Expected = 1; Expected <= Stack; ++Expected)
	{
		assert(Amount == Expected);
		Amount = StepStackAmount(Amount, kStackAmountFineStep, Stack);
	}
	// And it stops at the top rather than wrapping: wrapping a count would let
	// a slip turn "move all" into "move one".
	assert(Amount == Stack);
}

void ASteppedAnswerIsAlwaysSendable()
{
	// Whatever the steppers produce, CMoveObject's two rules hold: not zero,
	// and inside one byte.
	for (const std::uint32_t Stack : {1u, 2u, 10u, 100u})
	{
		std::uint32_t Amount = Stack;
		for (const std::int32_t Delta : {-1, -10, +1, +10, -100, +100})
		{
			Amount = StepStackAmount(Amount, Delta, Stack);
			assert(Amount >= kMinStackCount);
			assert(Amount <= Stack);
			assert(Amount <= kMaxWireStackCount);
		}
	}
}

} // namespace

int main()
{
	APlainDragMovesTheWholeStack();
	ShiftDragMovesExactlyOne();
	CtrlDragAsksAndOpensOnTheWholeStack();
	CtrlWinsOverShift();

	ANonStackableObjectAlwaysMovesWhole();
	ANonStackableObjectIgnoresAnyAmountItWasGiven();

	TheQuestionClampsToOneAtTheBottom();
	TheQuestionClampsToTheStackAtTheTop();
	EveryAmountInRangeSurvivesUnchanged();
	AStackOfOneAdmitsExactlyOneAnswer();
	AStackTheServerDescribedAsEmptyIsStillOneObject();
	NothingCanExceedTheOneCountByteOnTheWire();

	SteppersMoveTheAnswerAndStayInRange();
	SteppersCannotWalkOffEitherEnd();
	EveryReachableAnswerIsReachableWithTheFineStep();
	ASteppedAnswerIsAlwaysSendable();

	std::cout << "real33d stack move tests: 16 cases passed\n";
	return 0;
}
