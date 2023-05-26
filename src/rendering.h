#ifndef RENDERING_H
#define RENDERING_H

#include "colour.h"
#include "types.h"
#include "maths.h"

struct Vertex {
    Vec2 position;
    Vec2 texture_coords;
    Colour colour;
};

struct Vertices {
    Vertex* data;
    u32 index;
};

static void render_tetrimino_block(Vertices& vertices, f32 x, f32 y, const Colour& colour);
static void render_character(Vertices& vertices, i8 c, f32 x, f32 y, const Colour& colour);
static void render_text(Vertices& vertices, const i8* text, f32 x, f32 y, const Colour& colour);
static void render_integer(Vertices& vertices, i32 integer, f32 x, f32 y, const Colour& colour);

#endif
