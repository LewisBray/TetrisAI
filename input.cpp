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

bool shiftDown(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.current.down == KeyState::Pressed &&
        inputHistory.previous.down != KeyState::Pressed) ||
        inputHistory.current.down == KeyState::Held);
}

bool shiftLeft(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.current.left == KeyState::Pressed &&
        inputHistory.previous.left != KeyState::Pressed) ||
        inputHistory.current.left == KeyState::Held);
}

bool shiftRight(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.current.right == KeyState::Pressed &&
        inputHistory.previous.right != KeyState::Pressed) ||
        inputHistory.current.right == KeyState::Held);
}

bool rotateClockwise(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.current.rotateClockwise == KeyState::Pressed &&
        inputHistory.previous.rotateClockwise != KeyState::Pressed) ||
        inputHistory.current.rotateClockwise == KeyState::Held);
}

bool rotateAntiClockwise(const InputHistory& inputHistory) noexcept
{
    return ((inputHistory.current.rotateAntiClockwise == KeyState::Pressed &&
        inputHistory.previous.rotateAntiClockwise != KeyState::Pressed) ||
        inputHistory.current.rotateAntiClockwise == KeyState::Held);
}
