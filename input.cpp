#include "input.h"

PlayerInput getPlayerInput(const GLFW::Window& window) noexcept
{
	const KeyState up = window.keyState('W');
	const KeyState down = window.keyState('S');
	const KeyState left = window.keyState('A');
	const KeyState right = window.keyState('D');
	const KeyState rotateClockwise = window.keyState('K');
	const KeyState rotateAntiClockwise = window.keyState('J');

	return { down, left, right, rotateClockwise, rotateAntiClockwise };
}

void InputHistory::update(const PlayerInput& playerInput) noexcept
{
    down.previousState = down.currentState;
    down.currentState = playerInput.down;
    if (down.currentState == KeyState::Held)
        ++down.numUpdatesInHeldState;
    else
        down.numUpdatesInHeldState = 0;

    left.previousState = left.currentState;
    left.currentState = playerInput.left;
    if (left.currentState == KeyState::Held)
        ++left.numUpdatesInHeldState;
    else
        left.numUpdatesInHeldState = 0;

    right.previousState = right.currentState;
    right.currentState = playerInput.right;
    if (right.currentState == KeyState::Held)
        ++right.numUpdatesInHeldState;
    else
        right.numUpdatesInHeldState = 0;

    clockwise.previousState = clockwise.currentState;
    clockwise.currentState = playerInput.rotateClockwise;
    if (clockwise.currentState == KeyState::Held)
        ++clockwise.numUpdatesInHeldState;
    else
        clockwise.numUpdatesInHeldState = 0;

    antiClockwise.previousState = antiClockwise.currentState;
    antiClockwise.currentState = playerInput.rotateAntiClockwise;
    if (antiClockwise.currentState == KeyState::Held)
        ++antiClockwise.numUpdatesInHeldState;
    else
        antiClockwise.numUpdatesInHeldState = 0;
}
