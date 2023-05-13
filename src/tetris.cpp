#include "tetris.h"
#include "types.h"
#include "util.h"

namespace Tetris {
    Colour piece_colour(const Tetrimino::Type type) {
        static constexpr Colour COLOURS[Tetrimino::Type::COUNT] = {
            PURPLE, CYAN, GREEN, RED, BLUE, YELLOW, WHITE
        };

        return COLOURS[type];
    }

    static Vec2 centre_location(const Tetrimino::Type type, const Coordinates& top_left) {
        static constexpr Vec2 TOP_LEFT_OFFSETS[Tetrimino::Type::COUNT] = {
            Vec2{1.5f, 1.5f}, Vec2{0.5f, 1.5f}, Vec2{1.5f, 1.5f}, Vec2{1.5f, 1.5f}, Vec2{1.5f, 1.5f}, Vec2{1.0f, 1.0f}, Vec2{1.0f, 2.0f}
        };

        const Vec2 top_left_offset = TOP_LEFT_OFFSETS[type];
        return Vec2{static_cast<f32>(top_left.x) + top_left_offset.x, static_cast<f32>(top_left.y) + top_left_offset.y};
    }

    Tetrimino::Blocks block_locations(const Tetrimino::Type type, const Coordinates& top_left) {
        static constexpr Coordinates TOP_LEFT_OFFSET_COORDINATES[Tetrimino::Type::COUNT][4] = {
            {Coordinates{1, 0}, Coordinates{0, 1}, Coordinates{1, 1}, Coordinates{2, 1}},
            {Coordinates{0, 0}, Coordinates{0, 1}, Coordinates{0, 2}, Coordinates{1, 2}},
            {Coordinates{1, 0}, Coordinates{1, 1}, Coordinates{1, 2}, Coordinates{0, 2}},
            {Coordinates{1, 0}, Coordinates{2, 0}, Coordinates{0, 1}, Coordinates{1, 1}},
            {Coordinates{0, 0}, Coordinates{1, 0}, Coordinates{1, 1}, Coordinates{2, 1}},
            {Coordinates{0, 0}, Coordinates{1, 0}, Coordinates{1, 1}, Coordinates{0, 1}},
            {Coordinates{0, 0}, Coordinates{0, 1}, Coordinates{0, 2}, Coordinates{0, 3}}
        };

        return Tetrimino::Blocks{
            top_left + TOP_LEFT_OFFSET_COORDINATES[type][0],
            top_left + TOP_LEFT_OFFSET_COORDINATES[type][1],
            top_left + TOP_LEFT_OFFSET_COORDINATES[type][2],
            top_left + TOP_LEFT_OFFSET_COORDINATES[type][3]
        };
    }

    Tetrimino construct_tetrimino(const Tetrimino::Type type, const Coordinates& top_left) {
        Tetrimino tetrimino = {};
        tetrimino.blocks = block_locations(type, top_left);
        tetrimino.centre = centre_location(type, top_left);
        tetrimino.type = type;

        return tetrimino;
    }

    Tetrimino shift(Tetrimino tetrimino, const Coordinates& shift) {
        for (Coordinates& block_top_left : tetrimino.blocks.top_left_coordinates) {
            block_top_left += shift;
        }

        tetrimino.centre.x += static_cast<f32>(shift.x);
        tetrimino.centre.y += static_cast<f32>(shift.y);

        return tetrimino;
    }

    static Vec2 rotate_point_clockwise_about_reference_point(const Vec2& point, const Vec2& reference_point) {
        return Vec2{-point.y + reference_point.x + reference_point.y, point.x - reference_point.x + reference_point.y};
    }

    static Vec2 rotate_point_anti_clockwise_about_reference_point(const Vec2& point, const Vec2& reference_point) {
        return Vec2{point.y + reference_point.x - reference_point.y, -point.x + reference_point.x + reference_point.y};
    }

    static i32 floor(const f32 x) {
        const bool is_signed = x < 0.0f;
        const i32 x_truncated = static_cast<i32>(x);
        return is_signed ? x_truncated - 1 : x_truncated;
    }

    Tetrimino rotate(Tetrimino tetrimino, const Rotation rotation) {
        using RotationFunction = Vec2(*)(const Vec2&, const Vec2&);
        static constexpr RotationFunction rotation_functions[2] = {
            rotate_point_clockwise_about_reference_point,
            rotate_point_anti_clockwise_about_reference_point
        };

        const RotationFunction rotate = rotation_functions[rotation];
        for (Coordinates& block_top_left : tetrimino.blocks.top_left_coordinates) {
            const Vec2 block_centre = Vec2{static_cast<f32>(block_top_left.x) + 0.5f, static_cast<f32>(block_top_left.y) + 0.5f};    
            const Vec2 rotated_block_centre = rotate(block_centre, tetrimino.centre);

            block_top_left.x = floor(rotated_block_centre.x);
            block_top_left.y = floor(rotated_block_centre.y);
        }

        return tetrimino;
    }

    bool collision(const Tetrimino& tetrimino, const Grid& grid) {
        for (i32 block_index = 0; block_index < Tetrimino::Blocks::COUNT; ++block_index) {
            const Coordinates& block_top_left = tetrimino.blocks.top_left_coordinates[block_index];
            const bool overlaps_grid = block_top_left.x < 0 ||
                block_top_left.x >= Grid::COLUMN_COUNT ||
                block_top_left.y >= Grid::ROW_COUNT ||
                !is_empty_cell(grid.cells[static_cast<u64>(block_top_left.y)][static_cast<u64>(block_top_left.x)]);
            if (overlaps_grid) {
                return true;
            }
        }

        return false;
    }

    // TODO: don't like this mutable ref
    bool resolve_rotation_collision(Tetrimino& tetrimino, const Grid& grid) {
        const Tetrimino initial_tetrimino = tetrimino;

        static constexpr i32 MAX_NUM_ATTEMPTS = 4;
        for (i32 attempt = 1; attempt <= MAX_NUM_ATTEMPTS; ++attempt) {
            const i32 x_shift = ((attempt % 2 == 0) ? 1 : -1) * attempt;

            tetrimino = shift(tetrimino, Coordinates{x_shift, 0});
            if (!collision(tetrimino, grid)) {
                return true;
            }
        }

        tetrimino = initial_tetrimino;
        return false;
    }

    bool is_empty_cell(const Grid::Cell& grid_cell) {
        return grid_cell.r == 0.0f && grid_cell.g == 0.0f && grid_cell.b == 0.0f && grid_cell.a == 0.0f;
    }

    // TODO: unit tests
    i32 remove_completed_rows(Grid& grid) {
        bool discard_row[Grid::ROW_COUNT] = {};
        i32 discarded_row_count = 0;

        for (i32 row = 0; row < Grid::ROW_COUNT; ++row) {
            discard_row[row] = true;
            for (i32 column = 0; column < Grid::COLUMN_COUNT; ++column) {
                if (is_empty_cell(grid.cells[row][column])) {
                    discard_row[row] = false;
                    break;
                }
            }

            discarded_row_count += static_cast<i32>(discard_row[row]);
        }

        i32 insertion_row = Grid::ROW_COUNT - 1;
        for (i32 row = Grid::ROW_COUNT - 1; row >= 0; --row) {
            if (discard_row[row]) {
                insertion_row = row;
            } else {
                for (i32 column = 0; column < Grid::COLUMN_COUNT; ++column) {
                    grid.cells[insertion_row][column] = grid.cells[row][column];
                }

                --insertion_row;
            }
        }

        for (i32 row = discarded_row_count - 1; row >= 0; --row) {
            for (i32 column = 0; column < Grid::COLUMN_COUNT; ++column) {
                grid.cells[row][column] = {};
            }
        }

        return discarded_row_count;
    }

    void merge(const Tetrimino& tetrimino, Grid& grid) {
        const Colour& tetrimino_colour = piece_colour(tetrimino.type);
        for (const Coordinates& block_top_left : tetrimino.blocks.top_left_coordinates) {
            grid.cells[static_cast<u64>(block_top_left.y)][static_cast<u64>(block_top_left.x)] = tetrimino_colour;
        }
    }
}
