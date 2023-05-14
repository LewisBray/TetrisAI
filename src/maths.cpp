#include "maths.h"

static Coordinates operator+(const Coordinates& lhs, const Coordinates& rhs) {
    return Coordinates{lhs.x + rhs.x, lhs.y + rhs.y};
}

static Coordinates& operator+=(Coordinates& lhs, const Coordinates& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;

    return lhs;
}

static f32 min(const f32 x, const f32 y) {
    return (x < y) ? x : y;
}

static f32 max(const f32 x, const f32 y) {
    return (x > y) ? x : y;
}

static f32 clamp(const f32 val, const f32 lower_bound, const f32 upper_bound) {
    return min(upper_bound, max(lower_bound, val));
}
