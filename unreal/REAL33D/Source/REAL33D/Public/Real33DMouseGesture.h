#pragma once

// Input ownership only. No protocol or gameplay state belongs here.
namespace Real33D
{
enum class MouseButton { Left, Right };
enum class MouseAction { None, Walk, Use, UseWith, Look };

class MouseGesture
{
public:
	MouseAction Press(MouseButton Button, bool Shift, bool Alt, bool OtherDown = false)
	{
		(Button == MouseButton::Left ? LeftDown : RightDown) = true;
		if ((LeftDown && RightDown) || OtherDown)
		{
			LeftAction = RightAction = MouseAction::None;
			if (ChordLookSent) return MouseAction::None;
			ChordLookSent = true;
			return MouseAction::Look;
		}
		if (Button == MouseButton::Left)
		{
			LeftAction = Shift ? MouseAction::None : MouseAction::Walk;
			return Shift ? MouseAction::Look : MouseAction::None;
		}
		RightAction = Alt ? MouseAction::None : (Shift ? MouseAction::UseWith : MouseAction::Use);
		return Alt ? MouseAction::Look : MouseAction::None;
	}

	MouseAction Release(MouseButton Button)
	{
		bool& Down = Button == MouseButton::Left ? LeftDown : RightDown;
		if (!Down) return MouseAction::None; // UI-owned press cannot become world use.
		Down = false;
		MouseAction& Pending = Button == MouseButton::Left ? LeftAction : RightAction;
		const MouseAction Result = Pending;
		Pending = MouseAction::None;
		if (!LeftDown && !RightDown) ChordLookSent = false;
		return Result;
	}

	void RightDragged() { RightAction = MouseAction::None; }
	void LeftConsumed() { LeftAction = MouseAction::None; }
	void DiscardReleasedButtons(bool LeftHeld, bool RightHeld)
	{
		// A Slate drag/release outside this widget may not deliver button-up here.
		if (!LeftHeld) { LeftDown = false; LeftAction = MouseAction::None; }
		if (!RightHeld) { RightDown = false; RightAction = MouseAction::None; }
		if (!LeftDown && !RightDown) ChordLookSent = false;
	}
	bool IsChord() const { return ChordLookSent; }
	void Reset() { *this = MouseGesture{}; }

private:
	bool LeftDown = false;
	bool RightDown = false;
	bool ChordLookSent = false;
	MouseAction LeftAction = MouseAction::None;
	MouseAction RightAction = MouseAction::None;
};
}
