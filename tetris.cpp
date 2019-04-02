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
        case Tetrimino::Type::RL:       return Blue;
        case Tetrimino::Type::S:        return Red;
        case Tetrimino::Type::Z:        return Green;
        case Tetrimino::Type::Square:   return Yellow;
        case Tetrimino::Type::Long:     return White;

        default:
            throw std::exception{ "Invalid Tetrimino type in pieceColour" };
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
            throw std::exception{ "Invalid Tetrimino type used in centreLocation" };
        }
    }

    static constexpr Blocks blockLocations(
        const Tetrimino::Type type, const Position<int>& topLeft)
    {
        switch (type)
        {
        case Tetrimino::Type::T:
            return Blocks {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 2, 1 }
            };

        case Tetrimino::Type::L:
            return Blocks {
                topLeft,
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 0, 2 },
                topLeft + Position<int>{ 1, 2 }
            };

        case Tetrimino::Type::RL:
            return Blocks {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 1, 2 },
                topLeft + Position<int>{ 0, 2 }
            };

        case Tetrimino::Type::S:
            return Blocks {
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 2, 0 },
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 1, 1 }
            };

        case Tetrimino::Type::Z:
            return Blocks {
                topLeft,
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 2, 1 }
            };

        case Tetrimino::Type::Square:
            return Blocks {
                topLeft,
                topLeft + Position<int>{ 1, 0 },
                topLeft + Position<int>{ 1, 1 },
                topLeft + Position<int>{ 0, 1 }
            };

        case Tetrimino::Type::Long:
            return Blocks {
                topLeft,
                topLeft + Position<int>{ 0, 1 },
                topLeft + Position<int>{ 0, 2 },
                topLeft + Position<int>{ 0, 3 }
            };

        default:
            throw std::exception{ "Invalid Tetrimino type in blockLocations" };
        }
    }

    Tetrimino::Tetrimino(const Type type, const Position<int>& topLeft)
        : colour_{ pieceColour(type) }
        , blocks_{ blockLocations(type, topLeft) }
        , centre_{ centreLocation(type, topLeft) }
    {}

    void Tetrimino::rotateClockwise() noexcept
    {
        for (Position<int>& blockTopLeft : blocks_)
        {
            const Position<float> blockCentre =
                blockTopLeft + Position<float>{ 0.5f, 0.5f };
            const Position<float> rotatedBlockCentre = {
                -blockCentre.y + centre_.x + centre_.y,
                blockCentre.x - centre_.x + centre_.y
            };

            blockTopLeft.x = static_cast<int>(rotatedBlockCentre.x);
            blockTopLeft.y = static_cast<int>(rotatedBlockCentre.y);
        }
    }

    void Tetrimino::rotateAntiClockwise() noexcept
    {
        for (Position<int>& blockTopLeft : blocks_)
        {
            const Position<float> blockCentre =
                blockTopLeft + Position<float>{ 0.5f, 0.5f };
            const Position<float> rotatedBlockCentre = {
                blockCentre.y + centre_.x - centre_.y,
                -blockCentre.x + centre_.x + centre_.y
            };

            blockTopLeft.x = static_cast<int>(rotatedBlockCentre.x);
            blockTopLeft.y = static_cast<int>(rotatedBlockCentre.y);
        }
    }

    void Tetrimino::shift(const Direction direction) noexcept
    {
        const Position<int> shift = std::invoke([direction]() noexcept {
            switch (direction)
            {
            case Direction::Up:     return Position<int>{  0, -1 };
            case Direction::Down:   return Position<int>{  0,  1 };
            case Direction::Left:   return Position<int>{ -1,  0 };
            case Direction::Right:  return Position<int>{  1,  0 };
            default:                return Position<int>{  0,  0 };
            }
        });

        for (Position<int>& block : blocks_)
            block += shift;

        centre_ += shift;
    }

    bool Tetrimino::collidedWithBottom() const noexcept
    {
        for (const Position<int>& block : blocks_)
            if (block.y >= Rows)
                return true;

        return false;
    }

    bool Tetrimino::collidedWithLeftSide() const noexcept
    {
        for (const Position<int>& block : blocks_)
            if (block.x < 0)
                return true;

        return false;
    }

    bool Tetrimino::collidedWithRightSide() const noexcept
    {
        for (const Position<int>& block : blocks_)
            if (block.x >= Columns)
                return true;

        return false;
    }

    bool Tetrimino::collidedWithBlock(const Grid& grid) const noexcept
    {
        for (const Position<int>& block : blocks_)
            if (grid[block.x][block.y].has_value())
                return true;

        return false;
    }
}
