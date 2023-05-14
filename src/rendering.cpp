#include "rendering.h"

static void render_tetrimino_block(Scene& scene, const f32 x, const f32 y, const Colour& colour) {
    scene.vertices[scene.vertex_index + 0].position.x = x + 0.0f;
    scene.vertices[scene.vertex_index + 0].position.y = y + 1.0f;
    scene.vertices[scene.vertex_index + 0].texture_coords.x = 0.0f;
    scene.vertices[scene.vertex_index + 0].texture_coords.y = 0.0f;
    scene.vertices[scene.vertex_index + 0].colour = colour;

    scene.vertices[scene.vertex_index + 1].position.x = x + 1.0f;
    scene.vertices[scene.vertex_index + 1].position.y = y + 1.0f;
    scene.vertices[scene.vertex_index + 1].texture_coords.x = 1.0f;
    scene.vertices[scene.vertex_index + 1].texture_coords.y = 0.0f;
    scene.vertices[scene.vertex_index + 1].colour = colour;

    scene.vertices[scene.vertex_index + 2].position.x = x + 1.0f;
    scene.vertices[scene.vertex_index + 2].position.y = y + 0.0f;
    scene.vertices[scene.vertex_index + 2].texture_coords.x = 1.0f;
    scene.vertices[scene.vertex_index + 2].texture_coords.y = 1.0f;
    scene.vertices[scene.vertex_index + 2].colour = colour;

    scene.vertices[scene.vertex_index + 3].position.x = x + 0.0f;
    scene.vertices[scene.vertex_index + 3].position.y = y + 0.0f;
    scene.vertices[scene.vertex_index + 3].texture_coords.x = 0.0f;
    scene.vertices[scene.vertex_index + 3].texture_coords.y = 1.0f;
    scene.vertices[scene.vertex_index + 3].colour = colour;

    scene.vertex_index += 4;
}

static constexpr u32 FONT_SET_WIDTH = 16;
static constexpr u32 FONT_SET_HEIGHT = 16;
static constexpr u32 FONT_WIDTH_PIXELS = 16;
static constexpr u32 FONT_HEIGHT_PIXELS = 16;
static constexpr u32 FONT_SET_WIDTH_PIXELS = FONT_SET_WIDTH * FONT_WIDTH_PIXELS;
static constexpr u32 FONT_SET_HEIGHT_PIXELS = FONT_SET_HEIGHT * FONT_HEIGHT_PIXELS;
static constexpr f32 FONT_WIDTH_NORMALISED = static_cast<f32>(FONT_WIDTH_PIXELS) / static_cast<f32>(FONT_SET_WIDTH_PIXELS);
static constexpr f32 FONT_HEIGHT_NORMALISED = static_cast<f32>(FONT_HEIGHT_PIXELS) / static_cast<f32>(FONT_SET_HEIGHT_PIXELS);

static Vec2 texture_coordinates(const i8 c) {
    const u32 x = static_cast<u32>(c) % FONT_SET_WIDTH;
    const u32 y = FONT_SET_HEIGHT - static_cast<u32>(c) / FONT_SET_WIDTH;
    const f32 x_normalised = FONT_WIDTH_NORMALISED * static_cast<f32>(x);
    const f32 y_normalised = FONT_HEIGHT_NORMALISED * static_cast<f32>(y);

    return Vec2{x_normalised, y_normalised};
}

static void render_character(Scene& scene, const i8 c, const f32 x, const f32 y, const Colour& colour) {
    const Vec2 texture_top_left = texture_coordinates(c);

    scene.vertices[scene.vertex_index + 0].position.x = x + 0.0f;
    scene.vertices[scene.vertex_index + 0].position.y = y + 1.0f;
    scene.vertices[scene.vertex_index + 0].texture_coords.x = texture_top_left.x;
    scene.vertices[scene.vertex_index + 0].texture_coords.y = texture_top_left.y - FONT_HEIGHT_NORMALISED;
    scene.vertices[scene.vertex_index + 0].colour = colour;

    scene.vertices[scene.vertex_index + 1].position.x = x + 1.0f;
    scene.vertices[scene.vertex_index + 1].position.y = y + 1.0f;
    scene.vertices[scene.vertex_index + 1].texture_coords.x = texture_top_left.x + FONT_WIDTH_NORMALISED;
    scene.vertices[scene.vertex_index + 1].texture_coords.y = texture_top_left.y - FONT_HEIGHT_NORMALISED;
    scene.vertices[scene.vertex_index + 1].colour = colour;

    scene.vertices[scene.vertex_index + 2].position.x = x + 1.0f;
    scene.vertices[scene.vertex_index + 2].position.y = y + 0.0f;
    scene.vertices[scene.vertex_index + 2].texture_coords.x = texture_top_left.x + FONT_WIDTH_NORMALISED;
    scene.vertices[scene.vertex_index + 2].texture_coords.y = texture_top_left.y;
    scene.vertices[scene.vertex_index + 2].colour = colour;

    scene.vertices[scene.vertex_index + 3].position.x = x + 0.0f;
    scene.vertices[scene.vertex_index + 3].position.y = y + 0.0f;
    scene.vertices[scene.vertex_index + 3].texture_coords.x = texture_top_left.x;
    scene.vertices[scene.vertex_index + 3].texture_coords.y = texture_top_left.y;
    scene.vertices[scene.vertex_index + 3].colour = colour;

    scene.vertex_index += 4;
}

static void render_text(Scene& scene, const i8* text, f32 x, const f32 y, const Colour& colour) {
    while (*text != '\0') {
        render_character(scene, *text, x, y, colour);
        x += 1.0f;
        ++text;
    }
}

// This renders right-to-left as it comes out the easiest/most convenient, be careful!
static void render_integer(Scene& scene, i32 integer, f32 x, const f32 y, const Colour& colour) {
    static constexpr i32 MAX_INT_DIGITS = 10;   // max value held in 32-bits is O(10^9), i.e. 10 digits
    i8 digits[MAX_INT_DIGITS] = {};
    for (i32 i = 0; i < 10; ++i) {
        digits[i] = integer % 10;
        integer /= 10;
    }

    i32 digit_count = 1;
    for (i32 i = MAX_INT_DIGITS - 1; i >= 0; --i) {
        if (digits[i] != 0) {
            digit_count = i + 1;
            break;
        }
    }

    for (i32 i = 0; i < digit_count; ++i) {
        const i8 c = digits[i] + '0';
        render_character(scene, c, x, y, colour);
        x -= 1.0f;
    }
}
