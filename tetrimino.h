#pragma once

#include <optional>
#include <array>

template <typename T>
struct Position
{
    // static check for numeric type???
    T x;
    T y;
};

using Colour = std::array<float, 4>;
using Blocks = std::array<Position<int>, 4>;

// should we be declaring constexpr variables in header?
constexpr int rows = 18;
constexpr int columns = 10;
using Grid = std::array<std::array<std::optional<Colour>, columns>, rows>;

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
    Tetrimino(const Colour& colour,
        const Blocks& blocks, const Position<float>& centre) noexcept;

    virtual ~Tetrimino() {}

    void rotateClockwise() noexcept;
    void rotateAntiClockwise() noexcept;

    constexpr const Colour& colour() const noexcept { return colour_; }
    constexpr const Blocks& blocks() const noexcept { return blocks_; }
    // pretty sure all of these could be constexpr...
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

// maybe make 1 spawnBlockLocations function which is polymorphic?
class T final : public Tetrimino
{
public:
    T(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};

class L final : public Tetrimino
{
public:
    L(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};

class reverseL final : public Tetrimino
{
public:
    reverseL(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};

class S final : public Tetrimino
{
public:
    S(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};

class Z final : public Tetrimino
{
public:
    Z(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};

class Square final : public Tetrimino
{
public:
    Square(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};

class Long final : public Tetrimino
{
public:
    Long(const Position<int>& topLeft) noexcept;

private:
    // could be constexpr
    static Position<float> spawnCentre(const Position<int>& topLeft) noexcept;
    static Blocks spawnBlockLocations(const Position<int>& topLeft) noexcept;
};
