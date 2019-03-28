#include "GLEW\\glew.h"

#include "tetrimino.h"

template <typename T>
static constexpr Position<T>
operator+(const Position<T>& lhs, const Position<T>& rhs) noexcept
{
    return Position<T>{ lhs.x + rhs.x, lhs.y + rhs.y };
}

template <typename T>
static constexpr Position<T>&
operator+=(Position<T>& lhs, const Position<T>& rhs) noexcept
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;

    return lhs;
}


Tetrimino::Tetrimino(const Colour& colour,
    const Blocks& blocks, const Position<float>& centre) noexcept
    : colour_{ colour }
    , blocks_{ blocks }
    , centre_{ centre }
{}

void Tetrimino::rotateClockwise() noexcept
{
    for (Position<int>& block : blocks_)
    {
        const Position<float> blockCentre{ block.x + 0.5f, block.y + 0.5f };
        const Position<float> rotatedBlockCentre = {
            -blockCentre.y + centre_.x + centre_.y,
            blockCentre.x - centre_.x + centre_.y
        };

        block.x = static_cast<int>(rotatedBlockCentre.x);
        block.y = static_cast<int>(rotatedBlockCentre.y);
    }
}

void Tetrimino::rotateAntiClockwise() noexcept
{
    for (Position<int>& block : blocks_)
    {
        const Position<float> blockCentre{ block.x + 0.5f, block.y + 0.5f };
        const Position<float> rotatedBlockCentre = {
            blockCentre.y + centre_.x - centre_.y,
            -blockCentre.x + centre_.x + centre_.y
        };

        block.x = static_cast<int>(rotatedBlockCentre.x);
        block.y = static_cast<int>(rotatedBlockCentre.y);
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
}

// Make all of these into one function and return side collided with?
bool Tetrimino::collidedWithBottom() const noexcept
{
    for (const Position<int>& block : blocks_)
        if (block.y >= rows)
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
        if (block.x >= columns)
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


// maybe make part of the relevant classes or static if not needed elsewhere?
constexpr Colour red = { 1.0f, 0.0f, 0.0f, 1.0f };
constexpr Colour blue = { 0.0f, 1.0f, 0.0f, 1.0f };
constexpr Colour green = { 0.0f, 0.0f, 1.0f, 1.0f };
constexpr Colour yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
constexpr Colour purple = { 1.0f, 0.0f, 1.0f, 1.0f };
constexpr Colour cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
constexpr Colour white = { 1.0f, 1.0f, 1.0f, 1.0f };

T::T(const Position<int>& topLeft) noexcept
    : Tetrimino(purple , spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> T::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 1.5f, topLeft.y + 1.5f };
}

Blocks T::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft + Position<int>{ 1, 0 },
        topLeft + Position<int>{ 0, 1 },
        topLeft + Position<int>{ 1, 1 },
        topLeft + Position<int>{ 2, 1 }
    };
}


L::L(const Position<int>& topLeft) noexcept
    : Tetrimino(cyan, spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> L::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 0.0f, topLeft.y + 2.0f };
}

Blocks L::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft,
        topLeft + Position<int>{ 0, 1 },
        topLeft + Position<int>{ 0, 2 },
        topLeft + Position<int>{ 1, 2 }
    };
}


reverseL::reverseL(const Position<int>& topLeft) noexcept
    : Tetrimino(blue, spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> reverseL::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 1.0f, topLeft.y + 1.0f };
}

Blocks reverseL::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft + Position<int>{ 1, 0 },
        topLeft + Position<int>{ 1, 1 },
        topLeft + Position<int>{ 1, 2 },
        topLeft + Position<int>{ 0, 2 }
    };
}


S::S(const Position<int>& topLeft) noexcept
    : Tetrimino(red, spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> S::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 1.0f, topLeft.y + 0.0f };
}

Blocks S::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft + Position<int>{ 1, 0 },
        topLeft + Position<int>{ 2, 0 },
        topLeft + Position<int>{ 0, 1 },
        topLeft + Position<int>{ 1, 1 }
    };
}


Z::Z(const Position<int>& topLeft) noexcept
    : Tetrimino(green, spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> Z::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 1.0f, topLeft.y + 0.0f };
}

Blocks Z::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft + Position<int>{ 0, 0 },
        topLeft + Position<int>{ 1, 0 },
        topLeft + Position<int>{ 1, 1 },
        topLeft + Position<int>{ 2, 1 }
    };
}


Square::Square(const Position<int>& topLeft) noexcept
    : Tetrimino(yellow, spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> Square::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 1.0f, topLeft.y + 1.0f };
}

Blocks Square::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft + Position<int>{ 0, 0 },
        topLeft + Position<int>{ 1, 0 },
        topLeft + Position<int>{ 1, 1 },
        topLeft + Position<int>{ 0, 1 }
    };
}


Long::Long(const Position<int>& topLeft) noexcept
    : Tetrimino(white, spawnBlockLocations(topLeft), spawnCentre(topLeft))
{}

Position<float> Long::spawnCentre(const Position<int>& topLeft) noexcept
{
    return Position<float>{ topLeft.x + 0.0f, topLeft.y + 2.0f };
}

Blocks Long::spawnBlockLocations(const Position<int>& topLeft) noexcept
{
    return Blocks {
        topLeft + Position<int>{ 0, 0 },
        topLeft + Position<int>{ 0, 1 },
        topLeft + Position<int>{ 0, 2 },
        topLeft + Position<int>{ 0, 3 }
    };
}
