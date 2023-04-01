#include "maths.h"

static Coordinates operator+(const Coordinates& lhs, const Coordinates& rhs) {
    return Coordinates{lhs.x + rhs.x, lhs.y + rhs.y};
}

static Coordinates& operator+=(Coordinates& lhs, const Coordinates& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;

    return lhs;
}
