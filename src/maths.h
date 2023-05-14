#ifndef MATHS_H
#define MATHS_H

#include "types.h"

static f32 min(f32 x, f32 y);
static f32 max(f32 x, f32 y);
static f32 clamp(f32 val, f32 lower_bound, f32 upper_bound);

struct Vec2 {
    f32 x;
    f32 y;
};

struct Coordinates {
    i32 x;
    i32 y;
};

static Coordinates operator+(const Coordinates& lhs, const Coordinates& rhs);
static Coordinates& operator+=(Coordinates& lhs, const Coordinates& rhs);

#endif
