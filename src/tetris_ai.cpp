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

#include "neural_network.h"
#include "neural_network.cpp"

#define DEBUG_ASSERT(condition) if (!(condition)) platform.show_error_box("Debug Assert", #condition)

enum class GameMode : i32 {
    PLAYER_CONTROLLED = 0,
    AI_CONTROLLED = 1
};

struct GameState {
    GameMode game_mode;

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

    bool down_was_pressed;
    bool left_was_pressed;
    bool right_was_pressed;
    bool clockwise_was_pressed;
    bool anti_clockwise_was_pressed;
    
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

    NeuralNetwork neural_network;
    File training_data_file;
};

static_assert(sizeof(GameState) < GameMemory::PERMANENT_STORAGE_SIZE);

static i32 calculate_difficulty_level(const i32 total_rows_cleared) {
    return total_rows_cleared / 10 + 1;
}

static void game_state_to_neural_network_input(const GameState& game_state, NeuralNetwork::InputLayer& input) {
    const i32 difficulty_level = calculate_difficulty_level(game_state.total_rows_cleared);
    input[0] = static_cast<f32>(difficulty_level);
    input[1] = static_cast<f32>(game_state.total_rows_cleared);
    input[2] = static_cast<f32>(game_state.next_tetrimino.type);
    input[3] = static_cast<f32>(game_state.tetrimino.type);

    for (i32 i = 0; i < 4; ++i) {
        input[4 + i + 0] = static_cast<f32>(game_state.tetrimino.blocks.top_left_coordinates[i].x);
        input[4 + i + 1] = static_cast<f32>(game_state.tetrimino.blocks.top_left_coordinates[i].y);
    }

    i32 i = 12;
    for (i32 row = 0; row < Tetris::Grid::ROW_COUNT; ++row) {
        for (i32 column = 0; column < Tetris::Grid::COLUMN_COUNT; ++column) {
            const bool cell_has_block = !Tetris::is_empty_cell(game_state.grid.cells[row][column]);
            input[i++] = static_cast<f32>(cell_has_block);
        }
    }
}

static constexpr u8 BINARY_GAME_STATE_SIZE = 54;
using BinaryGameState = i8[BINARY_GAME_STATE_SIZE];
using BinaryPlayerInput = u16;

static u32 game_state_to_binary_game_state(const GameState& game_state, BinaryGameState& binary_game_state) {
    u32 bytes_written = 0;
    const i32 difficulty_level = calculate_difficulty_level(game_state.total_rows_cleared);
    bytes_written += copy_bytes(reinterpret_cast<const i8*>(&difficulty_level), sizeof(difficulty_level), binary_game_state + bytes_written);
    bytes_written += copy_bytes(reinterpret_cast<const i8*>(&game_state.total_rows_cleared), sizeof(game_state.total_rows_cleared), binary_game_state + bytes_written);

    const i8 next_tetrimino_type = static_cast<i8>(game_state.next_tetrimino.type);
    binary_game_state[bytes_written++] = next_tetrimino_type;

    const i8 tetrimino_type = static_cast<i8>(game_state.tetrimino.type);
    binary_game_state[bytes_written++] = tetrimino_type;

    const Tetris::Tetrimino::Blocks& tetrimino_blocks = game_state.tetrimino.blocks;
    for (i32 block_index = 0; block_index < 4; ++block_index) {
        const i8 block_top_left_x = static_cast<i8>(tetrimino_blocks.top_left_coordinates[block_index].x);
        binary_game_state[bytes_written++] = block_top_left_x;
        const i8 block_top_left_y = static_cast<i8>(tetrimino_blocks.top_left_coordinates[block_index].y);
        binary_game_state[bytes_written++] = block_top_left_y;
    }

    for (i32 row = 0; row < Tetris::Grid::ROW_COUNT; ++row) {
        u16 binary_row_state = 0;
        for (i32 column = 0; column < Tetris::Grid::COLUMN_COUNT; ++column) {
            const bool cell_has_block = !Tetris::is_empty_cell(game_state.grid.cells[row][column]);
            binary_row_state |= (static_cast<u16>(cell_has_block) << column);
        }

        bytes_written += copy_bytes(reinterpret_cast<const i8*>(&binary_row_state), sizeof(binary_row_state), binary_game_state + bytes_written);
    }

    return bytes_written;
}

// TODO: assert bytes_read is as expected at various points throughout
static void binary_game_state_to_neural_network_input(const BinaryGameState& binary_game_state, NeuralNetwork::InputLayer& input) {
    u32 bytes_read = 0;

    i32 difficulty_level = 0;
    bytes_read += copy_bytes(binary_game_state + bytes_read, sizeof(difficulty_level), reinterpret_cast<i8*>(&difficulty_level));
    input[0] = static_cast<f32>(difficulty_level);

    i32 rows_cleared = 0;
    bytes_read += copy_bytes(binary_game_state + bytes_read, sizeof(rows_cleared), reinterpret_cast<i8*>(&rows_cleared));
    input[1] = static_cast<f32>(rows_cleared);
    
    const i8 next_tetrimino_type = binary_game_state[bytes_read++];
    input[2] = static_cast<f32>(next_tetrimino_type);

    const i8 current_tetrimino_type = binary_game_state[bytes_read++];
    input[3] = static_cast<f32>(current_tetrimino_type);

    // read current tetrimino block positions
    for (i32 input_index = 4; input_index < 12; input_index += 2) {
        const i8 block_top_left_x = binary_game_state[bytes_read++];
        input[input_index] = static_cast<f32>(block_top_left_x);
        const i8 block_top_left_y = binary_game_state[bytes_read++];
        input[input_index + 1] = static_cast<f32>(block_top_left_y);
    }

    // read grid state
    i32 input_index = 12;
    for (i32 row = 0; row < Tetris::Grid::ROW_COUNT; ++row) {
        u16 encoded_row = 0;
        bytes_read += copy_bytes(binary_game_state + bytes_read, sizeof(encoded_row), reinterpret_cast<i8*>(&encoded_row));
        for (i32 column = 0; column < Tetris::Grid::COLUMN_COUNT; ++column) {
            const bool cell_has_block = (encoded_row & (1 << column)) != 0;
            input[input_index++] = static_cast<f32>(cell_has_block);
        }
    }
}

static BinaryPlayerInput player_input_to_binary_player_input(const PlayerInput& player_input) {
    BinaryPlayerInput binary_player_input = 0;

    binary_player_input |= static_cast<BinaryPlayerInput>(player_input.down);
    binary_player_input |= (static_cast<BinaryPlayerInput>(player_input.left) << 1);
    binary_player_input |= (static_cast<BinaryPlayerInput>(player_input.right) << 2);
    binary_player_input |= (static_cast<BinaryPlayerInput>(player_input.clockwise) << 3);
    binary_player_input |= (static_cast<BinaryPlayerInput>(player_input.anti_clockwise) << 4);

    return binary_player_input;
}

static void binary_player_input_to_neural_network_output(const BinaryPlayerInput encoded_outputs, NeuralNetwork::OutputLayer& output) {
    for (i32 i = 0; i < NeuralNetwork::OUTPUT_LAYER_SIZE; ++i) {
        const bool val = (encoded_outputs & (1 << i)) != 0;
        output[i] = static_cast<f32>(val);
    }
}

// TODO: should be averaging over batches
static void train(NeuralNetwork& neural_network, const i8* training_data, const u32 training_data_size) {
    // TODO: assert training data size is multiple of binary game state + player input

    NeuralNetwork neural_network_delta = {};

    u32 bytes_read = 0;
    while (bytes_read < training_data_size) {
        BinaryGameState binary_game_state = {};
        bytes_read += copy_bytes(training_data + bytes_read, sizeof(binary_game_state), reinterpret_cast<i8*>(binary_game_state));

        BinaryPlayerInput encoded_player_input = 0;
        bytes_read += copy_bytes(training_data + bytes_read, sizeof(encoded_player_input), reinterpret_cast<i8*>(&encoded_player_input));

        NeuralNetwork::InputLayer game_state = {};
        binary_game_state_to_neural_network_input(binary_game_state, game_state);

        NeuralNetwork::OutputLayer player_input = {};
        binary_player_input_to_neural_network_output(encoded_player_input, player_input);

        back_propagate(neural_network, game_state, player_input, neural_network_delta);
    }

    static constexpr f32 LEARNING_RATE = 0.1f;
    const f32 batch_size = static_cast<f32>(training_data_size / (BINARY_GAME_STATE_SIZE + 2));

    for (i32 row = 0; row < NeuralNetwork::HIDDEN_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::INPUT_LAYER_SIZE; ++column) {
            neural_network.input_to_hidden_weights[row][column] -= LEARNING_RATE * (neural_network_delta.input_to_hidden_weights[row][column]) / batch_size;
        }
    }

    for (i32 i = 0; i < NeuralNetwork::HIDDEN_LAYER_SIZE; ++i) {
        neural_network.hidden_biases[i] -= LEARNING_RATE * neural_network_delta.hidden_biases[i] / batch_size;
    }

    for (i32 row = 0; row < NeuralNetwork::OUTPUT_LAYER_SIZE; ++row) {
        for (i32 column = 0; column < NeuralNetwork::HIDDEN_LAYER_SIZE; ++column) {
            neural_network.hidden_to_output_weights[row][column] -= LEARNING_RATE * (neural_network_delta.hidden_to_output_weights[row][column]) / batch_size;
        }
    }

    for (i32 i = 0; i < NeuralNetwork::OUTPUT_LAYER_SIZE; ++i) {
        neural_network.output_biases[i] -= LEARNING_RATE * neural_network_delta.output_biases[i] / batch_size;
    }
}

static constexpr Coordinates TETRIMINO_SPAWN_LOCATION = Coordinates{4, 0};
static constexpr Coordinates NEXT_TETRIMINO_DISPLAY_LOCATION = Coordinates{15, 13};

extern "C" void initialise_game(const GameMemory& game_memory, const i32 client_width, const i32 client_height, const Platform& platform) {
    DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    GameState& game_state = *static_cast<GameState*>(game_memory.permanent_storage);

    game_state.game_mode = GameMode::AI_CONTROLLED;

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
    const GLchar* vertex_shader_source_text = static_cast<const GLchar*>(vertex_shader_source.data);
    const i32 vertex_shader_source_size = static_cast<i32>(vertex_shader_source.size);
    platform.glShaderSource(vertex_shader, 1, &vertex_shader_source_text, &vertex_shader_source_size);
    platform.glCompileShader(vertex_shader);

    const GLuint fragment_shader = platform.glCreateShader(GL_FRAGMENT_SHADER);
    const Resource fragment_shader_source = platform.load_resource(ID_FRAGMENT_SHADER);
    const GLchar* fragment_shader_source_text = static_cast<const GLchar*>(fragment_shader_source.data);
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
    game_state.next_tetrimino = construct_tetrimino(initial_next_tetrimino_type, NEXT_TETRIMINO_DISPLAY_LOCATION);

    game_state.player_score = 0;
    game_state.total_rows_cleared = 0;
    game_state.updates_since_last_drop = 0;

    game_state.accumulated_time = 0.0f;

    game_state.updates_down_held_count = 0;
    game_state.updates_left_held_count = 0;
    game_state.updates_right_held_count = 0;
    game_state.updates_clockwise_held_count = 0;
    game_state.updates_anti_clockwise_held_count = 0;

    game_state.down_was_pressed = false;
    game_state.left_was_pressed = false;
    game_state.right_was_pressed = false;
    game_state.clockwise_was_pressed = false;
    game_state.anti_clockwise_was_pressed = false;

    static constexpr const i8* NEURAL_NETWORK_FILE_NAME = "neural_network.bin";
    static constexpr const i8* TRAINING_DATA_FILE_NAME = "training_data.bin";

    File neural_network_file = {};
    if (platform.open_file(NEURAL_NETWORK_FILE_NAME, FileAccessFlags::READ, FileCreationFlags::USE_EXISTING, neural_network_file)) {
        const u32 neural_network_file_size = platform.get_file_size(neural_network_file);

        const u32 bytes_read_from_file = platform.read_file_into_buffer(neural_network_file, game_memory.transient_storage, neural_network_file_size);
        DEBUG_ASSERT(bytes_read_from_file == neural_network_file_size);

        const u32 bytes_read = load_from_buffer(game_state.neural_network, reinterpret_cast<const i8*>(game_memory.transient_storage), static_cast<u32>(game_memory.TRANSIENT_STORAGE_SIZE));
        DEBUG_ASSERT(bytes_read == neural_network_file_size);

        platform.close_file(neural_network_file);
    } else {
        game_state.neural_network = random_neural_network(game_state.rng_seed);
    }

    // TODO: can only read as much into memory as transient storage allows, make sure we read file in chunks if file size > transient storage
    game_state.training_data_file = {};
    if (platform.open_file(TRAINING_DATA_FILE_NAME, FileAccessFlags::READ, FileCreationFlags::USE_EXISTING, game_state.training_data_file)) {
        const u32 training_data_file_size = platform.get_file_size(game_state.training_data_file);

        const u32 bytes_read_from_file = platform.read_file_into_buffer(game_state.training_data_file, game_memory.transient_storage, training_data_file_size);
        DEBUG_ASSERT(bytes_read_from_file == training_data_file_size);

        for (i32 i = 0; i < 100; ++i) {
            train(game_state.neural_network, reinterpret_cast<const i8*>(game_memory.transient_storage), training_data_file_size);
        }

        platform.close_file(game_state.training_data_file);
    }

    if (platform.open_file(NEURAL_NETWORK_FILE_NAME, FileAccessFlags::WRITE, FileCreationFlags::ALWAYS_CREATE, neural_network_file)) {
        const u32 bytes_written = save_to_buffer(game_state.neural_network, reinterpret_cast<i8*>(game_memory.transient_storage));
        DEBUG_ASSERT(bytes_written < game_memory.TRANSIENT_STORAGE_SIZE);
        
        const u32 bytes_written_to_file = platform.write_buffer_into_file(neural_network_file, game_memory.transient_storage, bytes_written);
        DEBUG_ASSERT(bytes_written_to_file == bytes_written);

        platform.close_file(neural_network_file);
    }

    const bool file_opened = platform.open_file(TRAINING_DATA_FILE_NAME, FileAccessFlags::WRITE, FileCreationFlags::ALWAYS_OPEN, game_state.training_data_file);
    DEBUG_ASSERT(file_opened);

    game_state.previous_tick_count = platform.query_performance_counter();
}

static u32 update_held_count(const bool pressed, const bool previously_pressed, const u32 updates_held_count) {
    return (pressed && previously_pressed) ? (updates_held_count + 1) : 0;
}

static bool is_actionable_input(const bool pressed, const bool previously_pressed, const u32 updates_in_held_state) {
    static constexpr i32 DELAY = 3;

    const bool held = pressed && previously_pressed;
    return held && (updates_in_held_state > 30) && (updates_in_held_state % DELAY) == 0;
}

static void update_tetris_game(GameState& game_state, const PlayerInput& player_input, const Platform& platform) {
    const i64 tick_count = platform.query_performance_counter();
    const f32 frame_duration = static_cast<f32>(tick_count - game_state.previous_tick_count) / static_cast<f32>(game_state.tick_frequency) * 1000.0f;
    game_state.accumulated_time += frame_duration;
    game_state.previous_tick_count = tick_count;

    if (!game_state.down_was_pressed && player_input.down && !game_state.previous_player_input.down) {
        game_state.down_was_pressed = true;
    }

    if (!game_state.left_was_pressed && player_input.left && !game_state.previous_player_input.left) {
        game_state.left_was_pressed = true;
    }
    
    if (!game_state.right_was_pressed && player_input.right && !game_state.previous_player_input.right) {
        game_state.right_was_pressed = true;
    }
    
    if (!game_state.clockwise_was_pressed && player_input.clockwise && !game_state.previous_player_input.clockwise) {
        game_state.clockwise_was_pressed = true;
    }
    
    if (!game_state.anti_clockwise_was_pressed && player_input.anti_clockwise && !game_state.previous_player_input.anti_clockwise) {
        game_state.anti_clockwise_was_pressed = true;
    }

    static constexpr f32 DELTA_TIME = 1000.0f / 60.0f;
    while (game_state.accumulated_time >= DELTA_TIME) {
        // dump game state for training data
        BinaryGameState binary_game_state = {};
        const u32 bytes_written = game_state_to_binary_game_state(game_state, binary_game_state);
        DEBUG_ASSERT(bytes_written == sizeof(binary_game_state));

        const BinaryPlayerInput binary_player_input = player_input_to_binary_player_input(player_input);

        u32 bytes_written_to_file = 0;
        bytes_written_to_file += platform.write_buffer_into_file(game_state.training_data_file, binary_game_state, sizeof(binary_game_state));
        DEBUG_ASSERT(bytes_written_to_file == sizeof(binary_game_state));

        bytes_written_to_file += platform.write_buffer_into_file(game_state.training_data_file, &binary_player_input, sizeof(binary_player_input));
        DEBUG_ASSERT(bytes_written_to_file == sizeof(binary_game_state) + sizeof(binary_player_input));

        const PlayerInput& previous_player_input = game_state.previous_player_input;

        game_state.updates_down_held_count = update_held_count(player_input.down, previous_player_input.down, game_state.updates_down_held_count);
        game_state.updates_left_held_count = update_held_count(player_input.left, previous_player_input.left, game_state.updates_left_held_count);
        game_state.updates_right_held_count = update_held_count(player_input.right, previous_player_input.right, game_state.updates_right_held_count);
        game_state.updates_clockwise_held_count = update_held_count(player_input.clockwise, previous_player_input.clockwise, game_state.updates_clockwise_held_count);
        game_state.updates_anti_clockwise_held_count = update_held_count(player_input.anti_clockwise, previous_player_input.anti_clockwise, game_state.updates_anti_clockwise_held_count);

        const bool should_move_tetrimino_left = game_state.left_was_pressed || is_actionable_input(player_input.left, previous_player_input.left, game_state.updates_left_held_count);
        if (should_move_tetrimino_left) {
            game_state.tetrimino = shift(game_state.tetrimino, Coordinates{-1, 0});
            if (collision(game_state.tetrimino, game_state.grid)) {
                game_state.tetrimino = shift(game_state.tetrimino, Coordinates{1, 0});
            }
        }

        const bool should_move_tetrimino_right = game_state.right_was_pressed || is_actionable_input(player_input.right, previous_player_input.right, game_state.updates_right_held_count);
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
        const bool should_move_tetrimino_down = game_state.down_was_pressed || is_actionable_input(player_input.down, previous_player_input.down, game_state.updates_down_held_count) || game_state.updates_since_last_drop >= updates_allowed_before_drop;
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
            const bool should_rotate_tetrimino_clockwise = game_state.clockwise_was_pressed || is_actionable_input(player_input.clockwise, previous_player_input.clockwise, game_state.updates_clockwise_held_count);
            if (should_rotate_tetrimino_clockwise) {
                game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::CLOCKWISE);
                if (collision(game_state.tetrimino, game_state.grid) && !resolve_rotation_collision(game_state.tetrimino, game_state.grid)) {
                    game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::ANTI_CLOCKWISE);
                }
            }

            const bool should_rotate_tetrimino_anti_clockwise = game_state.anti_clockwise_was_pressed || is_actionable_input(player_input.anti_clockwise, previous_player_input.anti_clockwise, game_state.updates_anti_clockwise_held_count);
            if (should_rotate_tetrimino_anti_clockwise) {
                game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::ANTI_CLOCKWISE);
                if (collision(game_state.tetrimino, game_state.grid) && !resolve_rotation_collision(game_state.tetrimino, game_state.grid)) {
                    game_state.tetrimino = rotate(game_state.tetrimino, Tetris::Rotation::CLOCKWISE);
                }
            }
        } else {
            merge(game_state.tetrimino, game_state.grid);
            game_state.tetrimino = construct_tetrimino(game_state.next_tetrimino.type, TETRIMINO_SPAWN_LOCATION);
            if (collision(game_state.tetrimino, game_state.grid)) {
                // game over, reset
                game_state.player_score = 0;
                game_state.total_rows_cleared = 0;
                game_state.updates_since_last_drop = 0;
                game_state.grid = {};
            }

            game_state.rng_seed = random_number(game_state.rng_seed);
            const Tetris::Tetrimino::Type next_tetrimino_type = static_cast<Tetris::Tetrimino::Type>(game_state.rng_seed % Tetris::Tetrimino::Type::COUNT);
            game_state.next_tetrimino = construct_tetrimino(next_tetrimino_type, NEXT_TETRIMINO_DISPLAY_LOCATION);
        }

        const i32 rows_cleared = remove_completed_rows(game_state.grid);
        game_state.total_rows_cleared += rows_cleared;
        game_state.player_score += rows_cleared * 100 * difficulty_level;

        game_state.down_was_pressed = false;
        game_state.left_was_pressed = false;
        game_state.right_was_pressed = false;
        game_state.clockwise_was_pressed = false;
        game_state.anti_clockwise_was_pressed = false;

        ++game_state.updates_since_last_drop;
        game_state.accumulated_time -= DELTA_TIME;
    }

    game_state.previous_player_input = player_input;
}

extern "C" void update_game(const GameMemory& game_memory, const PlayerInput& player_input, const Platform& platform) {
    DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    GameState& game_state = *static_cast<GameState*>(game_memory.permanent_storage);

    switch (game_state.game_mode) {
        case GameMode::PLAYER_CONTROLLED: {
            update_tetris_game(game_state, player_input, platform);
        } break;

        case GameMode::AI_CONTROLLED: {
            NeuralNetwork::InputLayer nn_input = {};
            game_state_to_neural_network_input(game_state, nn_input);

            NeuralNetwork::OutputLayer nn_output = {};
            feed_forward(game_state.neural_network, nn_input, nn_output);

            static constexpr f32 AI_INPUT_THRESHOLD = 0.75f;

            PlayerInput ai_input = {};
            ai_input.down = nn_output[0] > AI_INPUT_THRESHOLD;
            ai_input.left = nn_output[1] > AI_INPUT_THRESHOLD;
            ai_input.right = nn_output[2] > AI_INPUT_THRESHOLD;
            ai_input.clockwise = nn_output[3] > AI_INPUT_THRESHOLD;
            ai_input.anti_clockwise = nn_output[4] > AI_INPUT_THRESHOLD;

            update_tetris_game(game_state, ai_input, platform);
        } break;
    }
}

static void render_grid(Scene& scene, const Tetris::Grid& grid) {
    for (i32 y = 0; y < Tetris::Grid::ROW_COUNT; ++y) {
        for (i32 x = 0; x < Tetris::Grid::COLUMN_COUNT; ++x) {
            const Tetris::Grid::Cell& cell = grid.cells[static_cast<u64>(y)][static_cast<u64>(x)];
            render_tetrimino_block(scene, static_cast<f32>(x), static_cast<f32>(y), cell);
        }
    }
}

static void render_tetrimino(Scene& scene, const Tetris::Tetrimino& tetrimino) {
    const Colour tetrimino_colour = piece_colour(tetrimino.type);
    for (i32 top_left_index = 0; top_left_index < 4; ++top_left_index) {
        const Coordinates& block_top_left = tetrimino.blocks.top_left_coordinates[top_left_index];
        render_tetrimino_block(scene, static_cast<f32>(block_top_left.x), static_cast<f32>(block_top_left.y), tetrimino_colour);
    }
}

static void render_tetris_game(Scene& scene, const GameState& game_state) {
    render_grid(scene, game_state.grid);
    render_tetrimino(scene, game_state.tetrimino);
    render_tetrimino(scene, game_state.next_tetrimino);

    render_text(scene, "SCORE", 13.0f, 1.0f, WHITE);
    render_integer(scene, game_state.player_score, 18.0f, 2.0f, WHITE);

    render_text(scene, "LEVEL", 13.0f, 4.0f, WHITE);
    const i32 difficulty_level = calculate_difficulty_level(game_state.total_rows_cleared);
    render_integer(scene, difficulty_level, 16.0f, 5.0f, WHITE);

    render_text(scene, "NEXT", 13.0f, 10.0f, WHITE);
}

static void render_neural_network_output(Scene& scene, const NeuralNetwork::OutputLayer nn_output) {
    const f32 left_confidence = clamp(nn_output[1], 0.0f, 1.0f);
    render_character(scene, '\x11', 0.0f, 0.0f, Colour{1.0f, 1.0f, 1.0f, left_confidence});

    const f32 down_confidence = clamp(nn_output[0], 0.0f, 1.0f);
    render_character(scene, '\x1F', 1.0f, 0.0f, Colour{1.0f, 1.0f, 1.0f, down_confidence});

    const f32 right_confidence = clamp(nn_output[2], 0.0f, 1.0f);
    render_character(scene, '\x10', 2.0f, 0.0f, Colour{1.0f, 1.0f, 1.0f, right_confidence});

    const f32 clockwise_confidence = clamp(nn_output[3], 0.0f, 1.0f);
    render_character(scene, '\x1A', 3.0f, 0.0f, Colour{1.0f, 1.0f, 1.0f, clockwise_confidence});

    const f32 anti_clockwise_confidence = clamp(nn_output[4], 0.0f, 1.0f);
    render_character(scene, '\x1B', 4.0f, 0.0f, Colour{1.0f, 1.0f, 1.0f, anti_clockwise_confidence});
}

extern "C" void render_game(const GameMemory& game_memory, const Platform& platform) {
    DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    const GameState& game_state = *static_cast<const GameState*>(game_memory.permanent_storage);

    platform.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    platform.glClear(GL_COLOR_BUFFER_BIT);

    Scene scene = {};
    switch (game_state.game_mode) {
        case GameMode::PLAYER_CONTROLLED: {
            render_tetris_game(scene, game_state);
        } break;

        case GameMode::AI_CONTROLLED: {
            render_tetris_game(scene, game_state);

            NeuralNetwork::InputLayer nn_input = {};
            game_state_to_neural_network_input(game_state, nn_input);

            NeuralNetwork::OutputLayer nn_output = {};
            feed_forward(game_state.neural_network, nn_input, nn_output);

            render_neural_network_output(scene, nn_output);
        } break;
    }

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
