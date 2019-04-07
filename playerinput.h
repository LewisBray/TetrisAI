#pragma once

#include "keystate.h"
#include "glfw.h"

struct PlayerInput
{
	KeyState down = KeyState::Released;
	KeyState left = KeyState::Released;
	KeyState right = KeyState::Released;
	KeyState rotateClockwise = KeyState::Released;
	KeyState rotateAntiClockwise = KeyState::Released;
};

PlayerInput getPlayerInput(const GLFW::Window& window) noexcept;
