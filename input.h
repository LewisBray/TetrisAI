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

struct InputHistory
{
    PlayerInput current;
    PlayerInput previous;
};

bool shiftDown(const InputHistory& inputHistory) noexcept;
bool shiftLeft(const InputHistory& inputHistory) noexcept;
bool shiftRight(const InputHistory& inputHistory) noexcept;
bool rotateClockwise(const InputHistory& inputHistory) noexcept;
bool rotateAntiClockwise(const InputHistory& inputHistory) noexcept;
