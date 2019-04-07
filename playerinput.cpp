#include "playerinput.h"

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
