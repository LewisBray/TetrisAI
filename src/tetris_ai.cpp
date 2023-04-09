#include "tetris_ai.h"
#include "rendering.h"
#include "resource.h"
#include "tetris.h"
#include "maths.h"
#include "types.h"
#include "util.h"

#include "rendering.cpp"
#include "tetris.cpp"
#include "maths.cpp"
#include "util.cpp"

#define DEBUG_ASSERT(condition) if (!(condition)) platform.show_error_box("Debug Assert", #condition)

struct GameState {
    PlayerInput previous_player_input;

    Tetris::Grid grid;
    Tetris::Tetrimino tetrimino;
    Tetris::Tetrimino next_tetrimino;

    i32 player_score;
    i32 total_rows_cleared;
    i32 updates_since_last_drop;

    u32 updates_down_held_count;
    u32 updates_left_held_count;
    u32 updates_right_held_count;
    u32 updates_clockwise_held_count;
    u32 updates_anti_clockwise_held_count;

    GLuint vertex_buffer_object;
    GLuint index_buffer_object;
    GLuint shader_program;
    GLint sampler_uniform_location;
    GLuint block_texture_id;
    GLuint font_texture_id;

    i64 tick_frequency;
    i64 previous_tick_count;
    f32 accumulated_time;

    u32 rng_seed;
};

static_assert(sizeof(GameState) < GameMemory::permanent_storage_size);

static constexpr Coordinates TETRIMINO_SPAWN_LOCATION = Coordinates{4, 0};

extern "C" void initialise_game(const GameMemory& game_memory, const i32 client_width, const i32 client_height, const Platform& platform) {
    DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    GameState& game_state = *static_cast<GameState*>(game_memory.permanent_storage);

    platform.glViewport(0, 0, client_width, client_height);

    game_state.vertex_buffer_object = 0;
    platform.glGenBuffers(1, &game_state.vertex_buffer_object);
    platform.glBindBuffer(GL_ARRAY_BUFFER, game_state.vertex_buffer_object);

    platform.glEnableVertexAttribArray(0);
    platform.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));

    platform.glEnableVertexAttribArray(1);
    platform.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(Vec2)));

    platform.glEnableVertexAttribArray(2);
    platform.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(Vec2)));

    GLuint index_buffer_object = 0;
    platform.glGenBuffers(1, &index_buffer_object);
    platform.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_object);

    u16 indices[6 * SCENE_TILE_COUNT] = {};
    for (u16 tile_index = 0; tile_index < SCENE_TILE_COUNT; ++tile_index) {
        indices[6 * tile_index + 0] = 4 * tile_index + 0;
        indices[6 * tile_index + 1] = 4 * tile_index + 1;
        indices[6 * tile_index + 2] = 4 * tile_index + 2;
        indices[6 * tile_index + 3] = 4 * tile_index + 2;
        indices[6 * tile_index + 4] = 4 * tile_index + 3;
        indices[6 * tile_index + 5] = 4 * tile_index + 0;
    }

    platform.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), static_cast<const void*>(indices), GL_STATIC_DRAW);

    // TODO: destroy index buffer?

    const GLuint vertex_shader = platform.glCreateShader(GL_VERTEX_SHADER);
    const Resource vertex_shader_source = platform.load_resource(ID_VERTEX_SHADER);
    const char* vertex_shader_source_text = static_cast<const char*>(vertex_shader_source.data);
    const i32 vertex_shader_source_size = static_cast<i32>(vertex_shader_source.size);
    platform.glShaderSource(vertex_shader, 1, &vertex_shader_source_text, &vertex_shader_source_size);
    platform.glCompileShader(vertex_shader);

    const GLuint fragment_shader = platform.glCreateShader(GL_FRAGMENT_SHADER);
    const Resource fragment_shader_source = platform.load_resource(ID_FRAGMENT_SHADER);
    const char* fragment_shader_source_text = static_cast<const char*>(fragment_shader_source.data);
    const i32 fragment_shader_source_size = static_cast<i32>(fragment_shader_source.size);
    platform.glShaderSource(fragment_shader, 1, &fragment_shader_source_text, &fragment_shader_source_size);
    platform.glCompileShader(fragment_shader);

    game_state.shader_program = platform.glCreateProgram();
    platform.glAttachShader(game_state.shader_program, vertex_shader);
    platform.glAttachShader(game_state.shader_program, fragment_shader);
    platform.glLinkProgram(game_state.shader_program);
    platform.glUseProgram(game_state.shader_program);

    // TODO: delete shaders?

    game_state.sampler_uniform_location = platform.glGetUniformLocation(game_state.shader_program, "u_sampler");

    static constexpr u32 BLOCK_SIZE_PIXELS = 300;
    const Resource block_texture_resource = platform.load_resource(ID_BLOCK_TEXTURE);
    DEBUG_ASSERT(block_texture_resource.size == 4 * BLOCK_SIZE_PIXELS * BLOCK_SIZE_PIXELS);

    game_state.block_texture_id = 0;
    platform.glGenTextures(1, &game_state.block_texture_id);
    platform.glBindTexture(GL_TEXTURE_2D, game_state.block_texture_id);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    platform.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BLOCK_SIZE_PIXELS, BLOCK_SIZE_PIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, block_texture_resource.data);

    const Resource font_texture_resource = platform.load_resource(ID_FONT_TEXTURE);
    DEBUG_ASSERT(font_texture_resource.size == 4 * FONT_SET_WIDTH_PIXELS * FONT_SET_HEIGHT_PIXELS);

    game_state.font_texture_id = 0;
    platform.glGenTextures(1, &game_state.font_texture_id);
    platform.glBindTexture(GL_TEXTURE_2D, game_state.font_texture_id);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    platform.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    platform.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONT_SET_WIDTH_PIXELS, FONT_SET_HEIGHT_PIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_texture_resource.data);

    platform.glEnable(GL_BLEND);
    platform.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    game_state.tick_frequency = platform.query_performance_frequency();
    game_state.previous_tick_count = platform.query_performance_counter();

    game_state.rng_seed = static_cast<u32>(game_state.previous_tick_count);

    game_state.grid = {};

    game_state.rng_seed = random_number(game_state.rng_seed);
    const Tetris::Tetrimino::Type tetrimino_type = static_cast<Tetris::Tetrimino::Type>(game_state.rng_seed % Tetris::Tetrimino::Type::COUNT);
    game_state.tetrimino = construct_tetrimino(tetrimino_type, TETRIMINO_SPAWN_LOCATION);

    game_state.rng_seed = random_number(game_state.rng_seed);
    const Tetris::Tetrimino::Type initial_next_tetrimino_type = static_cast<Tetris::Tetrimino::Type>(game_state.rng_seed % Tetris::Tetrimino::Type::COUNT);
    game_state.next_tetrimino = construct_tetrimino(initial_next_tetrimino_type, TETRIMINO_SPAWN_LOCATION);

    game_state.player_score = 0;
    game_state.total_rows_cleared = 0;
    game_state.updates_since_last_drop = 0;

    game_state.accumulated_time = 0.0f;

    game_state.updates_down_held_count = 0;
    game_state.updates_left_held_count = 0;
    game_state.updates_right_held_count = 0;
    game_state.updates_clockwise_held_count = 0;
    game_state.updates_anti_clockwise_held_count = 0;
}

static u32 update_held_count(const bool pressed, const bool previously_pressed, const u32 updates_held_count) {
    return (pressed && previously_pressed) ? (updates_held_count + 1) : 0;
}

static bool is_actionable_input(const bool pressed, const bool previously_pressed, const u32 updates_in_held_state) {
    static constexpr i32 DELAY = 3;

    const bool just_pressed = pressed && !previously_pressed;
    const bool held = pressed && previously_pressed;
    const bool act_on_held = held && (updates_in_held_state > 30) && (updates_in_held_state % DELAY) == 0;

    return just_pressed || act_on_held;
}

static i32 calculate_difficulty_level(const i32 total_rows_cleared) {
    return total_rows_cleared / 10 + 1;
}

extern "C" void update_game(const GameMemory& game_memory, const PlayerInput& player_input, const Platform& platform) {
    DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    GameState& game_state = *static_cast<GameState*>(game_memory.permanent_storage);

    const i64 tick_count = platform.query_performance_counter();
    const f32 frame_duration = static_cast<f32>(tick_count - game_state.previous_tick_count) / static_cast<float>(game_state.tick_frequency) * 1000.0f;
    game_state.accumulated_time += frame_duration;
    game_state.previous_tick_count = tick_count;

    static constexpr f32 DELTA_TIME = 1000.0f / 60.0f;
    while (game_state.accumulated_time >= DELTA_TIME) {
        const PlayerInput& previous_player_input = game_state.previous_player_input;

        game_state.updates_down_held_count = update_held_count(player_input.down, previous_player_input.down, game_state.updates_down_held_count);
        game_state.updates_left_held_count = update_held_count(player_input.left, previous_player_input.left, game_state.updates_left_held_count);
        game_state.updates_right_held_count = update_held_count(player_input.right, previous_player_input.right, game_state.updates_right_held_count);
        game_state.updates_clockwise_held_count = update_held_count(player_input.clockwise, previous_player_input.clockwise, game_state.updates_clockwise_held_count);
        game_state.updates_anti_clockwise_held_count = update_held_count(player_input.anti_clockwise, previous_player_input.anti_clockwise, game_state.updates_anti_clockwise_held_count);

        const bool should_move_tetrimino_left = is_actionable_input(player_input.left, previous_player_input.left, game_state.updates_left_held_count);
        if (should_move_tetrimino_left) {
            game_state.tetrimino = shift(game_state.tetrimino, Coordinates{-1, 0});
            if (collision(game_state.tetrimino, game_state.grid)) {
                game_state.tetrimino = shift(game_state.tetrimino, Coordinates{1, 0});
            }
        }

        const bool should_move_tetrimino_right = is_actionable_input(player_input.right, previous_player_input.right, game_state.updates_right_held_count);
        if (should_move_tetrimino_right) {
            game_state.tetrimino = shift(game_state.tetrimino, Coordinates{1, 0});
            if (collision(game_state.tetrimino, game_state.grid)) {
                game_state.tetrimino = shift(game_state.tetrimino, Coordinates{-1, 0});
            }
        }

        // Fastest we can get is 6 drops per second
        const i32 difficulty_level = calculate_difficulty_level(game_state.total_rows_cleared);
        const i32 updates_allowed = 120 - 5 * (difficulty_level - 1);
        const i32 updates_allowed_before_drop = (updates_allowed < 10) ? 10 : updates_allowed;

        bool should_merge_tetrimino_with_grid = false;
        const bool should_move_tetrimino_down = is_actionable_input(player_input.down, previous_player_input.down, game_state.updates_down_held_count) || game_state.updates_since_last_drop >= updates_allowed_before_drop;
        if (should_move_tetrimino_down) {
            game_state.tetrimino = shift(game_state.tetrimino, Coordinates{0, 1});
            if (collision(game_state.tetrimino, game_state.grid)) {   // Then we need to merge tetrimino to grid and spawn another
                game_state.tetrimino = shift(game_state.tetrimino, Coordinates{0, -1});
                should_merge_tetrimino_with_grid = true;
            }
            
            game_state.updates_since_last_drop = 0;
        }

        // TODO: should this go before attempting to drop tetrimino down/left/right?
        if (!should_merge_tetrimino_with_grid) {
            const bool should_rotate_tetrimino_clockwise = is_actionable_input(player_input.clockwise, previous_player_input.clockwise, game_state.updates_clockwise_held_count);
            if (should_rotate_tetrimino_clockwise) {
                game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::CLOCKWISE);
                if (collision(game_state.tetrimino, game_state.grid) && !resolve_rotation_collision(game_state.tetrimino, game_state.grid)) {
                    game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::ANTI_CLOCKWISE);
                }
            }

            const bool should_rotate_tetrimino_anti_clockwise = is_actionable_input(player_input.anti_clockwise, previous_player_input.anti_clockwise, game_state.updates_anti_clockwise_held_count);
            if (should_rotate_tetrimino_anti_clockwise) {
                game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::ANTI_CLOCKWISE);
                if (collision(game_state.tetrimino, game_state.grid) && !resolve_rotation_collision(game_state.tetrimino, game_state.grid)) {
                    game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::CLOCKWISE);
                }
            }
        } else {
            merge(game_state.tetrimino, game_state.grid);
            game_state.tetrimino = game_state.next_tetrimino;
            if (collision(game_state.tetrimino, game_state.grid)) {
                // game over, reset
                game_state.player_score = 0;
                game_state.total_rows_cleared = 0;
                game_state.updates_since_last_drop = 0;
                game_state.grid = {};
            }

            game_state.rng_seed = random_number(game_state.rng_seed);
            const Tetris::Tetrimino::Type next_tetrimino_type = static_cast<Tetris::Tetrimino::Type>(game_state.rng_seed % Tetris::Tetrimino::Type::COUNT);
            game_state.next_tetrimino = construct_tetrimino(next_tetrimino_type, TETRIMINO_SPAWN_LOCATION);
        }

        const i32 rows_cleared = remove_completed_rows(game_state.grid);
        game_state.total_rows_cleared += rows_cleared;
        game_state.player_score += rows_cleared * 100 * difficulty_level;

        ++game_state.updates_since_last_drop;
        game_state.accumulated_time -= DELTA_TIME;
    }

    game_state.previous_player_input = player_input;
}

extern "C" void render_game(const GameMemory& game_memory, const Platform& platform) {
    // DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    const GameState& game_state = *static_cast<const GameState*>(game_memory.permanent_storage);

    platform.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    platform.glClear(GL_COLOR_BUFFER_BIT);

    Scene scene = {};
    for (i32 y = 0; y < Tetris::Grid::ROW_COUNT; ++y) {
        for (i32 x = 0; x < Tetris::Grid::COLUMN_COUNT; ++x) {
            const Tetris::Grid::Cell& cell = game_state.grid.cells[static_cast<u64>(y)][static_cast<u64>(x)];
            render_tetrimino_block(scene, static_cast<f32>(x), static_cast<f32>(y), cell);
        }
    }

    const Colour tetrimino_colour = piece_colour(game_state.tetrimino.type);
    for (i32 top_left_index = 0; top_left_index < 4; ++top_left_index) {
        const Coordinates& block_top_left = game_state.tetrimino.blocks.top_left_coordinates[top_left_index];
        render_tetrimino_block(scene, static_cast<f32>(block_top_left.x), static_cast<f32>(block_top_left.y), tetrimino_colour);
    }

    static constexpr Coordinates NEXT_TETRIMINO_DISPLAY_LOCATION = {15, 13};
    const Colour next_tetrimino_colour = piece_colour(game_state.next_tetrimino.type);
    const Tetris::Tetrimino::Blocks next_tetrimino_display_blocks = Tetris::block_locations(game_state.next_tetrimino.type, NEXT_TETRIMINO_DISPLAY_LOCATION);
    for (i32 top_left_index = 0; top_left_index < 4; ++top_left_index) {
        const Coordinates& block_top_left = next_tetrimino_display_blocks.top_left_coordinates[top_left_index];
        render_tetrimino_block(scene, static_cast<f32>(block_top_left.x), static_cast<f32>(block_top_left.y), next_tetrimino_colour);
    }

    render_text(scene, "SCORE", 13.0f, 1.0f);
    render_integer(scene, game_state.player_score, 18.0f, 2.0f);

    render_text(scene, "LEVEL", 13.0f, 4.0f);
    const i32 difficulty_level = calculate_difficulty_level(game_state.total_rows_cleared);
    render_integer(scene, difficulty_level, 16.0f, 5.0f);

    render_text(scene, "NEXT", 13.0f, 10.0f);

    platform.glBindBuffer(GL_ARRAY_BUFFER, game_state.vertex_buffer_object);
    platform.glBufferData(GL_ARRAY_BUFFER, sizeof(scene.vertices), reinterpret_cast<const void*>(scene.vertices), GL_STATIC_DRAW);

    // render blocks
    platform.glUniform1i(game_state.sampler_uniform_location, 0);
    platform.glActiveTexture(GL_TEXTURE0);
    platform.glBindTexture(GL_TEXTURE_2D, game_state.block_texture_id);

    static constexpr u32 BLOCK_COUNT = Tetris::Grid::ROW_COUNT * Tetris::Grid::COLUMN_COUNT + 8;
    platform.glDrawElements(GL_TRIANGLES, 6 * BLOCK_COUNT, GL_UNSIGNED_SHORT, nullptr);

    // render ui
    platform.glUniform1i(game_state.sampler_uniform_location, 1);
    platform.glActiveTexture(GL_TEXTURE1);
    platform.glBindTexture(GL_TEXTURE_2D, game_state.font_texture_id);

    static constexpr u32 UI_TILE_COUNT = SCENE_TILE_COUNT - BLOCK_COUNT;
    platform.glDrawElements(GL_TRIANGLES, 6 * UI_TILE_COUNT, GL_UNSIGNED_SHORT, reinterpret_cast<const void*>(6 * BLOCK_COUNT * sizeof(u16)));
}
