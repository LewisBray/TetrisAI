#pragma once

#include "position.hpp"

#include <optional>
#include <array>

namespace Tetris
{
    using Colour = std::array<float, 4>;
    constexpr Colour Red = { 1.0f, 0.0f, 0.0f, 1.0f };
    constexpr Colour Blue = { 0.0f, 1.0f, 0.0f, 1.0f };
    constexpr Colour Green = { 0.0f, 0.0f, 1.0f, 1.0f };
    constexpr Colour Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
    constexpr Colour Purple = { 1.0f, 0.0f, 1.0f, 1.0f };
    constexpr Colour Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
    constexpr Colour White = { 1.0f, 1.0f, 1.0f, 1.0f };

    constexpr int Rows = 18;
    constexpr int Columns = 10;
    using Grid = std::array<std::array<std::optional<Colour>, Columns>, Rows>;

    using Blocks = std::array<Position<int>, 4>;

    enum class Direction
    {
        Up,
        Down,
        Left,
        Right
    };

    class Tetrimino
    {
    public:
        enum class Type { T, L, RL, S, Z, Square, Long };

        Tetrimino(Type type, const Position<int>& topLeft);

        void rotateClockwise() noexcept;
        void rotateAntiClockwise() noexcept;

        constexpr const Colour& colour() const noexcept { return colour_; }
        constexpr const Blocks& blocks() const noexcept { return blocks_; }

        void shift(Direction direction) noexcept;
        bool collidedWithBottom() const noexcept;
        bool collidedWithLeftSide() const noexcept;
        bool collidedWithRightSide() const noexcept;
        bool collidedWithBlock(const Grid& grid) const noexcept;

    private:
        Colour colour_;
        Blocks blocks_;
        Position<float> centre_;
    };
}
