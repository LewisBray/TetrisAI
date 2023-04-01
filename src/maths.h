#ifndef MATHS_H
#define MATHS_H

#include "types.h"

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
