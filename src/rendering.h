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

static constexpr u32 SCENE_HORIZONTAL_TILE_COUNT = 20;
static constexpr u32 SCENE_VERTICAL_TILE_COUNT = 18;
static constexpr u32 SCENE_TILE_COUNT = SCENE_HORIZONTAL_TILE_COUNT * SCENE_VERTICAL_TILE_COUNT;

struct Scene {
    Vertex vertices[4 * SCENE_TILE_COUNT];
    u32 vertex_index;
};

static void render_tetrimino_block(Scene& scene, f32 x, f32 y, const Colour& colour);
static void render_character(Scene& scene, i8 c, f32 x, f32 y, const Colour& colour);
static void render_text(Scene& scene, const i8* text, f32 x, f32 y, const Colour& colour);
static void render_integer(Scene& scene, i32 integer, f32 x, f32 y, const Colour& colour);

#endif
