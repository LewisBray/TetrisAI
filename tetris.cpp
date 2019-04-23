#include "GLEW\\glew.h"

#include "currenttime.h"
#include "tetris.h"

#include <exception>
#include <utility>
#include <sstream>
#include <random>

namespace Tetris
{
    // This could be a map but then it can't be constexpr and we would have to
    // write a function to retrieve the colour value for the Tetrimino constructor
    // if we want the map to be const so this seemed the simplest solution
    static constexpr Colour pieceColour(Tetrimino::Type type)
    {
        switch (type)
        {
        case Tetrimino::Type::T:        return Purple;
        case Tetrimino::Type::L:        return Cyan;
        case Tetrimino::Type::RL:       return Green;
        case Tetrimino::Type::S:        return Red;
        case Tetrimino::Type::Z:        return Blue;
        case Tetrimino::Type::Square:   return Yellow;
        case Tetrimino::Type::Long:     return White;

        default:
            throw std::exception {
				"Invalid Tetrimino type in function 'pieceColour'"
			};
        }
    }

    static constexpr Position<float> centreLocation(
        const Tetrimino::Type type, const Position<int>& topLeft)
    {
        switch (type)
        {
        case Tetrimino::Type::T:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::L:
            return topLeft + Position<float>{ 0.5f, 1.5f };

        case Tetrimino::Type::RL:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::S:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::Z:
            return topLeft + Position<float>{ 1.5f, 1.5f };

        case Tetrimino::Type::Square:
            return topLeft + Position<float>{ 1.0f, 1.0f };

        case Tetrimino::Type::Long:
            return topLeft + Position<float>{ 0.0f, 2.0f };

        default:
            throw std::exception {
				"Invalid Tetrimino type used in function 'centreLocation'"
			};
        }
    }

    static constexpr Tetrimino::Blocks blockLocations(
        const Tetrimino::Type type, const Position<int>& topLeft)
    {
        switch (type)
        {
        case Tetrimino::Type::T:
            return {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 2, 1 }
            };

        case Tetrimino::Type::L:
            return {
                topLeft,
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 0, 2 },
                topLeft + Position<int>{ 1, 2 }
            };

        case Tetrimino::Type::RL:
            return {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 1, 2 },
                topLeft + Position<int>{ 0, 2 }
            };

        case Tetrimino::Type::S:
            return {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 2, 0 },
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 1, 1 }
            };

        case Tetrimino::Type::Z:
            return {
                topLeft,
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 2, 1 }
            };

        case Tetrimino::Type::Square:
            return {
                topLeft,
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 0, 1 }
            };

        case Tetrimino::Type::Long:
            return {
                topLeft,
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 0, 2 },
                topLeft + Position<int>{ 0, 3 }
            };

        default:
            throw std::exception {
				"Invalid Tetrimino type in used in function 'blockLocations'"
			};
        }
    }

    Tetrimino::Tetrimino(const Type type, const Position<int>& topLeft)
        : blocks_{ blockLocations(type, topLeft) }
        , centre_{ centreLocation(type, topLeft) }
        , colour_{ pieceColour(type) }
        , type_{ type }
    {}

    void Tetrimino::rotate(const Direction direction) noexcept
    {
        if (direction != Direction::Clockwise &&
            direction != Direction::AntiClockwise)
            return;

        for (Position<int>& blockTopLeft : blocks_)
        {
            const Position<float> blockCentre =
                blockTopLeft + Position<float>{ 0.5f, 0.5f };

            const Position<float> rotatedBlockCentre =
                (direction == Direction::Clockwise) ?
            Position<float>{
                -blockCentre.y + centre_.x + centre_.y,
                 blockCentre.x - centre_.x + centre_.y
            } :
            Position<float>{
                 blockCentre.y + centre_.x - centre_.y,
                -blockCentre.x + centre_.x + centre_.y
            };

            blockTopLeft.x = static_cast<int>(std::floor(rotatedBlockCentre.x));
            blockTopLeft.y = static_cast<int>(std::floor(rotatedBlockCentre.y));
        }
    }

	void Tetrimino::shift(const Position<int>& shift) noexcept
	{
		for (Position<int>& block : blocks_)
			block += shift;

		centre_ += shift;
	}

    static bool shouldMove(
        const Tetrimino::Direction direction, const InputHistory& inputHistory)
    {
        static constexpr int delay = 3;

        switch (direction)
        {
        case Tetrimino::Direction::Down:
            return ((inputHistory.down.currentState == KeyState::Pressed &&
                inputHistory.down.previousState != KeyState::Pressed) ||
                (inputHistory.down.currentState == KeyState::Held &&
                inputHistory.down.numUpdatesInHeldState % delay == 0));

        case Tetrimino::Direction::Left:
            return ((inputHistory.left.currentState == KeyState::Pressed &&
                inputHistory.left.previousState != KeyState::Pressed) ||
                (inputHistory.left.currentState == KeyState::Held &&
                inputHistory.left.numUpdatesInHeldState % delay == 0));

        case Tetrimino::Direction::Right:
            return ((inputHistory.right.currentState == KeyState::Pressed &&
                inputHistory.right.previousState != KeyState::Pressed) ||
                (inputHistory.right.currentState == KeyState::Held &&
                inputHistory.right.numUpdatesInHeldState % delay == 0));

        case Tetrimino::Direction::Clockwise:
            return ((inputHistory.clockwise.currentState == KeyState::Pressed &&
                inputHistory.clockwise.previousState != KeyState::Pressed) ||
                (inputHistory.clockwise.currentState == KeyState::Held &&
                inputHistory.clockwise.numUpdatesInHeldState % delay == 0));
        
        case Tetrimino::Direction::AntiClockwise:
            return ((inputHistory.antiClockwise.currentState == KeyState::Pressed &&
                inputHistory.antiClockwise.previousState != KeyState::Pressed) ||
                (inputHistory.antiClockwise.currentState == KeyState::Held &&
                inputHistory.antiClockwise.numUpdatesInHeldState % delay == 0));

        default:
            throw std::exception{
                "Unhandled player input in function 'shouldMove'"
            };
        }
    }

    std::pair<bool, int> Tetrimino::update(const InputHistory& inputHistory,
        const Grid& grid, const int updatesSinceLastDrop)
    {
        if (shouldMove(Direction::Left, inputHistory))
        {
            shift({ -1, 0 });
            if (collision(*this, grid))
                shift({ 1, 0 });
        }

        if (shouldMove(Direction::Right, inputHistory))
        {
            shift({ 1, 0 });
            if (collision(*this, grid))
                shift({ -1, 0 });
        }

        const bool dropTetrimino =
            (shouldMove(Direction::Down, inputHistory) ||
            updatesSinceLastDrop >= 120);
        if (dropTetrimino)
        {
            shift({ 0, 1 });
            if (collision(*this, grid)) // Then we need to merge tetrimino...
            {						    // ...to grid and spawn another
                shift({ 0, -1 });
                return std::make_pair(true, 0);
            }
        }

        if (shouldMove(Direction::Clockwise, inputHistory))
        {
            rotate(Direction::Clockwise);

            if (collision(*this, grid) && !resolveRotationCollision(grid))
                rotate(Direction::AntiClockwise);
        }

        if (shouldMove(Direction::AntiClockwise, inputHistory))
        {
            rotate(Direction::AntiClockwise);

            if (collision(*this, grid) && !resolveRotationCollision(grid))
                rotate(Direction::Clockwise);
        }

        if (dropTetrimino)
            return std::make_pair(false, 0);
        else
            return std::make_pair(false, updatesSinceLastDrop);
    }

    bool Tetrimino::resolveRotationCollision(const Grid& grid) noexcept
    {
        const std::pair<Blocks, Position<float>> initialState{ blocks_, centre_ };

        static constexpr int MaxNumAttempts = 4;
        for (int attempt = 1; attempt <= MaxNumAttempts; ++attempt)
        {
            const int xShift = ((attempt % 2 == 0) ? 1 : -1) * attempt;

            shift({ xShift, 0 });
            if (!collision(*this, grid))
                return true;
        }

        std::tie(blocks_, centre_) = initialState;
        return false;
    }

    Tetrimino randomTetrimino(const Position<int>& position)
    {
        const std::chrono::milliseconds time = currentTime();
        const std::uint32_t rngSeed = static_cast<std::uint32_t>(time.count());
        
        std::minstd_rand rng(rngSeed);
        const std::uniform_int_distribution<int>
            uniformDist(0, Tetrimino::TotalTypes - 1);

        const Tetrimino::Type randomType =
            static_cast<Tetrimino::Type>(uniformDist(rng));

        return Tetrimino(randomType, position);
    }

    Tetrimino::Blocks nextTetriminoBlockDisplayLocations(const Tetrimino::Type type)
    {
        static constexpr Position<int> DisplayLocationTopLeft = { 5, 12 };
        
        return blockLocations(type, DisplayLocationTopLeft);
    }

    const std::array<Grid::Cell, Grid::Columns>&
        Grid::operator[](const int row) const noexcept
    {
        return grid_[row];
    }

    int Grid::update() noexcept
    {
        constexpr auto rowIsComplete =
            [](const std::array<Cell, Columns> & row) noexcept {
                return std::all_of(row.begin(), row.end(),
                    [](const Cell & cell) { return cell.has_value(); });
        };

        const auto startOfRemovedRows =
            std::remove_if(grid_.rbegin(), grid_.rend(), rowIsComplete);

        constexpr auto clearRow = [](std::array <Cell, Columns>& row) noexcept {
            for (Cell& cell : row)
                cell.reset();
        };

        std::for_each(startOfRemovedRows, grid_.rend(), clearRow);

        const int rowsCleared =
            static_cast<int>(std::distance(startOfRemovedRows, grid_.rend()));

        return rowsCleared;
    }

    void Grid::merge(const Tetrimino& tetrimino) noexcept
    {
        const Colour& tetriminoColour = tetrimino.colour();
        const Tetrimino::Blocks& tetriminoBlocks = tetrimino.blocks();
        for (const Position<int>& block : tetriminoBlocks)
            grid_[block.y][block.x] = tetriminoColour;
    }

    bool collision(const Tetrimino& tetrimino, const Grid& grid)
    {
        const auto overlapsGrid = [&grid](const Position<int>& block) noexcept {
            return (block.x < 0 || block.x >= Grid::Columns ||
                block.y >= Grid::Rows || grid[block.y][block.x].has_value());
        };

        const Tetrimino::Blocks& blocks = tetrimino.blocks();
        return std::any_of(blocks.begin(), blocks.end(), overlapsGrid);
    }
}
