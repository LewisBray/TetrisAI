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

static constexpr int delay = 3;

bool shiftDown(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.down.currentState == KeyState::Pressed &&
        inputHistory.down.previousState != KeyState::Pressed) ||
        (inputHistory.down.currentState == KeyState::Held &&
        inputHistory.down.numUpdatesInHeldState % delay == 0));
}

bool shiftLeft(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.left.currentState == KeyState::Pressed &&
        inputHistory.left.previousState != KeyState::Pressed) ||
        (inputHistory.left.currentState == KeyState::Held &&
        inputHistory.left.numUpdatesInHeldState % delay == 0));
}

bool shiftRight(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.right.currentState == KeyState::Pressed &&
        inputHistory.right.previousState != KeyState::Pressed) ||
        (inputHistory.right.currentState == KeyState::Held &&
        inputHistory.right.numUpdatesInHeldState % delay == 0));
}

bool rotateClockwise(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.clockwise.currentState == KeyState::Pressed &&
        inputHistory.clockwise.previousState != KeyState::Pressed) ||
        (inputHistory.clockwise.currentState == KeyState::Held &&
        inputHistory.clockwise.numUpdatesInHeldState % delay == 0));
}

bool rotateAntiClockwise(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.antiClockwise.currentState == KeyState::Pressed &&
        inputHistory.antiClockwise.previousState != KeyState::Pressed) ||
        (inputHistory.antiClockwise.currentState == KeyState::Held &&
        inputHistory.antiClockwise.numUpdatesInHeldState % delay == 0));
}
