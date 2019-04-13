#include "GLEW\\glew.h"

#include "tetris.h"

#include <exception>
#include <sstream>
#include <map>

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
            return topLeft + Position<float>{ 0.0f, 2.0f };

        case Tetrimino::Type::RL:
            return topLeft + Position<float>{ 1.0f, 1.0f };

        case Tetrimino::Type::S:
            return topLeft + Position<float>{ 1.0f, 0.0f };

        case Tetrimino::Type::Z:
            return topLeft + Position<float>{ 1.0f, 0.0f };

        case Tetrimino::Type::Square:
            return topLeft + Position<float>{ 1.0f, 1.0f };

        case Tetrimino::Type::Long:
            return topLeft + Position<float>{ 0.0f, 1.0f };

        default:
            throw std::exception {
				"Invalid Tetrimino type used in function 'centreLocation'"
			};
        }
    }

    static constexpr Blocks blockLocations(
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
    {}

    void Tetrimino::rotate(const Rotation direction) noexcept
    {
        if (direction != Rotation::Clockwise &&
            direction != Rotation::AntiClockwise)
            return;

        for (Position<int>& blockTopLeft : blocks_)
        {
            const Position<float> blockCentre =
                blockTopLeft + Position<float>{ 0.5f, 0.5f };

            const Position<float> rotatedBlockCentre =
                (direction == Rotation::Clockwise) ?
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

	bool Tetrimino::collides(const Grid& grid) const noexcept
	{
		for (const Position<int>& block : blocks_)
		{
			if (block.x < 0)
				return true;
			else if (block.x >= Columns)
				return true;
			else if (block.y >= Rows)
				return true;
			else if (grid[block.y][block.x])
				return true;
		}

		return false;
	}

    std::pair<bool, int> Tetrimino::update(const InputHistory& inputHistory,
        const Grid& grid, const int updatesSinceLastDrop) noexcept
    {
        if (shiftLeft(inputHistory))
        {
            shift({ -1, 0 });
            if (collides(grid))
                shift({ 1, 0 });
        }

        if (shiftRight(inputHistory))
        {
            shift({ 1, 0 });
            if (collides(grid))
                shift({ -1, 0 });
        }

        const bool dropTetrimino =
            (shiftDown(inputHistory) || updatesSinceLastDrop >= 120);
        if (dropTetrimino)
        {
            shift({ 0, 1 });
            if (collides(grid))     // Then we need to merge tetrimino...
            {						// ...to grid and spawn another
                shift({ 0, -1 });
                return std::make_pair(true, 0);
            }
        }

        if (rotateClockwise(inputHistory))
        {
            rotate(Rotation::Clockwise);

            if (collides(grid) && !resolveRotationCollision(grid))
                rotate(Rotation::AntiClockwise);
        }

        if (rotateAntiClockwise(inputHistory))
        {
            rotate(Rotation::AntiClockwise);

            if (collides(grid) && !resolveRotationCollision(grid))
                rotate(Rotation::Clockwise);
        }

        if (dropTetrimino)
            return std::make_pair(false, 0);
        else
            return std::make_pair(false, updatesSinceLastDrop);
    }

    bool Tetrimino::resolveRotationCollision(const Grid& grid) noexcept
    {
        const std::pair<Blocks, Position<float>> initialState{ blocks_, centre_ };

        constexpr int MaxNumAttempts = 4;
        for (int attempt = 1; attempt <= MaxNumAttempts; ++attempt)
        {
            const int xShift = ((attempt % 2 == 0) ? 1 : -1) * attempt;

            shift({ xShift, 0 });
            if (!collides(grid))
                return true;
        }

        std::tie(blocks_, centre_) = initialState;
        return false;
    }
}
