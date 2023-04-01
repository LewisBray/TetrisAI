#include "tetris.h"
#include "types.h"
#include "util.h"

// static std::array<char, 56> writeGameStateToBuffer(const int difficultyLevel,
//     const int rowsCleared, const Tetris::Tetrimino::Type nextTetriminoType,
//     const Tetris::Tetrimino& currentTetrimino, const Tetris::Grid& grid,
//     const InputHistory& inputHistory)
// {
//     std::array<char, 56> buffer{};
//     std::copy_n(reinterpret_cast<const char*>(&difficultyLevel), 4, buffer.begin());
//     std::copy_n(reinterpret_cast<const char*>(&rowsCleared), 4, buffer.begin() + 4);
//     buffer[8] = static_cast<char>(nextTetriminoType);
//     buffer[9] = static_cast<char>(currentTetrimino.type());

//     constexpr auto encodeAsInt16 = [](const Position<int> position) {
//         return static_cast<std::int16_t>((position.x << 8) + position.y);
//     };

//     const Tetris::Tetrimino::Blocks& tetriminoBlocks = currentTetrimino.blocks;
//     for (auto [blockPosition, offset] = std::make_pair(tetriminoBlocks.begin(), 10);
//         blockPosition != tetriminoBlocks.end(); ++blockPosition, offset += 2)
//     {
//         const std::int16_t position = encodeAsInt16(*blockPosition);
//         std::copy_n(reinterpret_cast<const char*>(&position), 2, buffer.begin() + offset);
//     }

//     for (int row = 0, offset = 18; row < Tetris::Grid::ROW_COUNT; ++row, offset += 2)
//     {
//         int rowState = 0;
//         for (int column = 0; column < Tetris::Grid::COLUMN_COUNT; ++column)
//             rowState |= (grid[row][column].has_value() << column);

//         std::copy_n(reinterpret_cast<const char*>(&rowState), 2, buffer.begin() + offset);
//     }

//     // This could be saved in a byte but saving it in 2 makes
//     // the whole game state a multiple of 8 bytes in the file
//     int playerInputState = 0;
//     playerInputState |= (inputHistory.down.currentState != KeyState::Released);
//     playerInputState |= ((inputHistory.left.currentState != KeyState::Released) << 1);
//     playerInputState |= ((inputHistory.right.currentState != KeyState::Released) << 2);
//     playerInputState |= ((inputHistory.clockwise.currentState != KeyState::Released) << 3);
//     playerInputState |= ((inputHistory.antiClockwise.currentState != KeyState::Released) << 4);
//     std::copy_n(reinterpret_cast<const char*>(&playerInputState), 2, buffer.begin() + 54);

//     return buffer;
// }

// static std::array<double, 188> toNeuralNetworkInputs(const int difficultyLevel,
//     const int rowsCleared, const Tetris::Tetrimino::Type nextTetriminoType,
//     const Tetris::Tetrimino & currentTetrimino, const Tetris::Grid & grid,
//     const InputHistory & inputHistory)
// {
//     std::array<double, 188> inputs{};
//     inputs[0] = static_cast<double>(difficultyLevel);
//     inputs[1] = static_cast<double>(rowsCleared);
//     inputs[2] = static_cast<double>(nextTetriminoType);
//     inputs[3] = static_cast<double>(currentTetrimino.type());

//     const Tetris::Tetrimino::Blocks& tetriminoBlocks = currentTetrimino.blocks;
//     for (auto [index, blockPosition] = std::make_pair(4, tetriminoBlocks.begin());
//         blockPosition != tetriminoBlocks.end(); index += 2, ++blockPosition)
//     {
//         inputs[index] = static_cast<double>(blockPosition->x);
//         inputs[index + 1] = static_cast<double>(blockPosition->y);
//     }

//     for (int row = 0, index = 12; row < Tetris::Grid::ROW_COUNT; ++row)
//         for (int column = 0; column < Tetris::Grid::COLUMN_COUNT; ++column, ++index)
//             inputs[index] = static_cast<double>(grid[row][column].has_value());

//     return inputs;
// }

// static constexpr PlayerInput toPlayerInput(const std::array<double, 5>& neuralNetworkOutput) noexcept
// {
//     constexpr auto toKeyState = [](const double d) noexcept {
//         return (d == 0.0) ? KeyState::Released : KeyState::Pressed;
//     };

//     PlayerInput translatedInput{};
//     translatedInput.down = toKeyState(neuralNetworkOutput[0]);
//     translatedInput.left = toKeyState(neuralNetworkOutput[1]);
//     translatedInput.right = toKeyState(neuralNetworkOutput[2]);
//     translatedInput.clockwise = toKeyState(neuralNetworkOutput[3]);
//     translatedInput.antiClockwise = toKeyState(neuralNetworkOutput[4]);

//     return translatedInput;
// }

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

            block_top_left.x = static_cast<i32>(rotated_block_centre.x);
            block_top_left.y = static_cast<i32>(rotated_block_centre.y);
        }

        return tetrimino;
    }

    static bool collision(const Tetrimino& tetrimino, const Grid& grid) {
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
