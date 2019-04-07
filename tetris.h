#pragma once

#include "position.hpp"
#include "colour.h"

#include <optional>
#include <array>

namespace Tetris
{
    constexpr int Rows = 18;
    constexpr int Columns = 10;

    using Blocks = std::array<Position<int>, 4>;
    using Grid = std::array<std::array<std::optional<Colour>, Columns>, Rows>;

	class Tetrimino
    {
    public:
        enum class Type { T, L, RL, S, Z, Square, Long };

        Tetrimino(Type type, const Position<int>& topLeft);

        constexpr const Colour& colour() const noexcept { return colour_; }
        constexpr const Blocks& blocks() const noexcept { return blocks_; }

        void rotateClockwise() noexcept;
        void rotateAntiClockwise() noexcept;
        void shift(const Position<int>& direction) noexcept;
		bool collides(const Grid& grid) const noexcept;

    private:
		Blocks blocks_ = {};			// Does this set every value to 0?
		Position<float> centre_ = {};	// Ditto...
        Colour colour_ = Black;
    };
}
