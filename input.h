#pragma once

enum class KeyState
{
    Released,
    Pressed,
    Held
};

struct PlayerInput
{
    KeyState down = KeyState::Released;
    KeyState left = KeyState::Released;
    KeyState right = KeyState::Released;
    KeyState clockwise = KeyState::Released;
    KeyState antiClockwise = KeyState::Released;
};

struct KeyStateHistory
{
    KeyState currentState = KeyState::Released;
    KeyState previousState = KeyState::Released;
    int numUpdatesInHeldState = 0;
};

struct InputHistory
{
    KeyStateHistory down;
    KeyStateHistory left;
    KeyStateHistory right;
    KeyStateHistory clockwise;
    KeyStateHistory antiClockwise;
};
