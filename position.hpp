#pragma once

#include <type_traits>

template <typename T>
struct Position
{
    static_assert(std::is_arithmetic_v<T>, "Position must be of numeric type");

    T x;
    T y;
};

template <typename S, typename T>
static constexpr Position<std::common_type_t<S, T>>
operator+(const Position<S>& lhs, const Position<T>& rhs) noexcept
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}

template <typename S, typename T>
static constexpr Position<S>&
operator+=(Position<S>& lhs, const Position<T>& rhs) noexcept
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;

    return lhs;
}