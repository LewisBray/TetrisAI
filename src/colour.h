#ifndef COLOUR_H
#define COLOUR_H

#include "types.h"

struct Colour {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
};

static constexpr Colour RED = Colour{1.0f, 0.0f, 0.0f, 1.0f};
static constexpr Colour GREEN = Colour{0.0f, 1.0f, 0.0f, 1.0f};
static constexpr Colour BLUE = Colour{0.0f, 0.0f, 1.0f, 1.0f};
static constexpr Colour YELLOW = Colour{1.0f, 1.0f, 0.0f, 1.0f};
static constexpr Colour PURPLE = Colour{1.0f, 0.0f, 1.0f, 1.0f};
static constexpr Colour CYAN = Colour{0.0f, 1.0f, 1.0f, 1.0f};
static constexpr Colour WHITE = Colour{1.0f, 1.0f, 1.0f, 1.0f};
static constexpr Colour BLACK = Colour{0.0f, 0.0f, 0.0f, 1.0f};
static constexpr Colour GREY = Colour{0.2f, 0.2f, 0.2f, 1.0f};

#endif
