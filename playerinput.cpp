#include "playerinput.h"

PlayerInput getPlayerInput(const GLFW::Window& window) noexcept
{
	const KeyState up = window.getKeyState('W');
	const KeyState down = window.getKeyState('S');
	const KeyState left = window.getKeyState('A');
	const KeyState right = window.getKeyState('D');
	const KeyState rotateClockwise = window.getKeyState('K');
	const KeyState rotateAntiClockwise = window.getKeyState('J');

	return { down, left, right, rotateClockwise, rotateAntiClockwise };
}
