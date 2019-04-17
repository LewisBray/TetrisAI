#pragma once

#include <array>

using Colour = std::array<float, 4>;

constexpr Colour Red = { 1.0f, 0.0f, 0.0f, 1.0f };
constexpr Colour Green = { 0.0f, 1.0f, 0.0f, 1.0f };
constexpr Colour Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
constexpr Colour Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
constexpr Colour Purple = { 1.0f, 0.0f, 1.0f, 1.0f };
constexpr Colour Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
constexpr Colour White = { 1.0f, 1.0f, 1.0f, 1.0f };
constexpr Colour Black = { 0.0f, 0.0f, 0.0f, 1.0f };
constexpr Colour Grey = { 0.2f, 0.2f, 0.2f, 1.0f };
