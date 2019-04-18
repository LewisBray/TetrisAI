#pragma once

#include "position.hpp"
#include "colour.h"
#include "input.h"

#include <optional>
#include <utility>
#include <array>

namespace Tetris
{
    class Grid;

	class Tetrimino
    {
    public:
        using Blocks = std::array<Position<int>, 4>;

        static constexpr int TotalTypes = 7;
        
        enum class Type { T, L, RL, S, Z, Square, Long };
        enum class Direction { Down, Left, Right, Clockwise, AntiClockwise };

        Tetrimino(Type type, const Position<int>& topLeft);

        constexpr const Colour& colour() const noexcept { return colour_; }
        constexpr const Blocks& blocks() const noexcept { return blocks_; }

        void rotate(Direction direction) noexcept;
        void shift(const Position<int>& direction) noexcept;
        
        std::pair<bool, int> update(const InputHistory& inputHistory,
            const Grid& grid, int updatesSinceLastDrop) noexcept;

    private:
        bool resolveRotationCollision(const Grid& grid) noexcept;
		
        Blocks blocks_ = {};			// Does this set every value to 0?
		Position<float> centre_ = {};	// Ditto...
        Colour colour_ = Black;
    };

    Tetrimino randomTetrimino(const Position<int>& position);

    class Grid
    {
    public:
        using Cell = std::optional<Colour>;

        static constexpr int Rows = 18;
        static constexpr int Columns = 10;

        const std::array<Cell, Columns>& operator[](int row) const noexcept;

        void merge(const Tetrimino& tetrimino) noexcept;
        void update() noexcept;

    private:
        bool rowIsComplete(const std::array<Cell, Columns>& row) const;

        std::array<std::array<Cell, Columns>, Rows> grid_;
    };

    bool collision(const Tetrimino& tetrimino, const Grid& grid) noexcept;
}
