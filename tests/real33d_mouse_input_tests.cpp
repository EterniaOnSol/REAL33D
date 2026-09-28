#include "Real33DMouseGesture.h"
#include <cassert>
#include <iostream>

int main()
{
	using namespace Real33D;
	MouseGesture g;
	assert(g.Release(MouseButton::Right) == MouseAction::None);
	assert(g.Press(MouseButton::Left, false, false) == MouseAction::None);
	assert(g.Release(MouseButton::Left) == MouseAction::Walk);
	assert(g.Press(MouseButton::Right, false, false) == MouseAction::None);
	assert(g.Release(MouseButton::Right) == MouseAction::Use);
	assert(g.Press(MouseButton::Left, true, false) == MouseAction::Look);
	assert(g.Release(MouseButton::Left) == MouseAction::None);
	assert(g.Press(MouseButton::Left, false, false) == MouseAction::None);
	g.DiscardReleasedButtons(false, true); // Slot drag ended outside the widget.
	assert(g.Press(MouseButton::Right, false, false) == MouseAction::None);
	assert(g.Release(MouseButton::Right) == MouseAction::Use);
	for (const auto first : {MouseButton::Left, MouseButton::Right})
	{
		const auto second = first == MouseButton::Left ? MouseButton::Right : MouseButton::Left;
		for (bool reverseRelease : {false, true})
		{
			assert(g.Press(first, false, false) == MouseAction::None);
			assert(g.Press(second, false, false) == MouseAction::Look);
			assert(g.Release(reverseRelease ? second : first) == MouseAction::None);
			assert(g.Release(reverseRelease ? first : second) == MouseAction::None);
		}
	}
	assert(g.Press(MouseButton::Right, false, false, true) == MouseAction::Look);
	assert(g.Release(MouseButton::Right) == MouseAction::None);
	assert(g.Press(MouseButton::Right, true, false) == MouseAction::None);
	assert(g.Release(MouseButton::Right) == MouseAction::UseWith);
	assert(g.Press(MouseButton::Right, false, false) == MouseAction::None);
	g.RightDragged();
	assert(g.Release(MouseButton::Right) == MouseAction::None);
	assert(g.Press(MouseButton::Left, false, false) == MouseAction::None);
	g.Reset();
	assert(g.Release(MouseButton::Left) == MouseAction::None);
	std::cout << "PASS: left walk, shift-left look, both chord orders/release orders, UI release isolation, use-with, orbit, reset\n";
}
