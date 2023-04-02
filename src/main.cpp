// TODO:
//  - remove std lib + c runtime
//  - fix bug when holding out of grid and rotating
//  - fix user input being eaten by world update loop
//  - move non-platform code out of this file
//  - handle resizing window
//  - add padding around viewport to prevent skewing of square shapes
//  - refactor neural network stuff to not use awful lal
//  - bring neural network stuff back into game
//  - update types to use types defined by the API (DWORD, GLint, etc...)
//  - investigate using SIMD/multithreading for neural network heavy lifting

#include "rendering.h"
#include "colour.h"
#include "tetris.h"
#include "maths.h"
#include "util.h"

#include "rendering.cpp"
#include "tetris.cpp"
#include "maths.cpp"
#include "util.cpp"

#include "resource.h"

#include <Windows.h>
#include <gl\GL.h>

// game
static i32 calculate_difficulty_level(const i32 total_rows_cleared) {
    return total_rows_cleared / 10 + 1;
}

// OpenGL
using GLchar = char;
using GLsizeiptr = i64;

static constexpr GLenum GL_TEXTURE0 = 0x84C0;
static constexpr GLenum GL_TEXTURE1 = GL_TEXTURE0 + 1;
static constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
static constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
static constexpr GLenum GL_STATIC_DRAW = 0x88E4;
static constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
static constexpr GLenum GL_VERTEX_SHADER = 0x8B31;

#define GET_OPENGL_PROC(name, type) \
auto name = reinterpret_cast<type>(wglGetProcAddress(#name));

// user input
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

#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"

#define DEBUG_ASSERT(condition) \
if (!(condition)) { \
    MessageBoxA(NULL, #condition, "Debug Assert", MB_ICONERROR); \
}

struct Resource {
    const void* data;
    u32 size;
};

static Resource load_resource(const HINSTANCE instance, const i32 id) {
    const HRSRC resource = FindResourceA(instance, MAKEINTRESOURCEA(id), RT_RCDATA);
    DEBUG_ASSERT(resource != NULL)
    const HGLOBAL resource_data = LoadResource(instance, resource);
    DEBUG_ASSERT(resource_data != NULL)
    const void* data = LockResource(resource_data);
    DEBUG_ASSERT(data != nullptr)
    const u32 size = SizeofResource(instance, resource);
    DEBUG_ASSERT(size > 0)

    return Resource{data, size};
}

struct KeyboardInput {
    bool a;
    bool d;
    bool j;
    bool k;
    bool s;
};

struct ApplicationState {
    KeyboardInput keyboard_input;
};

static LRESULT main_window_procedure(const HWND window, const UINT message, const WPARAM w_param, const LPARAM l_param) {
    DEBUG_ASSERT(window != NULL)

    switch (message) {
        case WM_CREATE: {
            DEBUG_ASSERT(l_param != 0)
            CREATESTRUCTA* const create_struct = reinterpret_cast<CREATESTRUCTA*>(l_param);
            DEBUG_ASSERT(create_struct->lpCreateParams != nullptr)  // window should be created with CreateWindowEx
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_struct->lpCreateParams));

            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        case WM_KEYDOWN: {
            const LONG_PTR user_data = GetWindowLongPtrA(window, GWLP_USERDATA);
            ApplicationState* const application_state = reinterpret_cast<ApplicationState*>(user_data);
            KeyboardInput& keyboard_input = application_state->keyboard_input;
            const char vk_key = static_cast<char>(w_param);
            switch (vk_key) {
                case 'A': {
                    keyboard_input.a = true;
                    return 0;
                }

                case 'D': {
                    keyboard_input.d = true;
                    return 0;
                }

                case 'J': {
                    keyboard_input.j = true;
                    return 0;
                }

                case 'K': {
                    keyboard_input.k = true;
                    return 0;
                }

                case 'S': {
                    keyboard_input.s = true;
                    return 0;
                }

                default: {
                    return DefWindowProcA(window, message, w_param, l_param);
                }
            }
        }

        case WM_KEYUP: {
            const LONG_PTR user_data = GetWindowLongPtrA(window, GWLP_USERDATA);
            ApplicationState* const application_state = reinterpret_cast<ApplicationState*>(user_data);
            KeyboardInput& keyboard_input = application_state->keyboard_input;
            const char vk_key = static_cast<char>(w_param);
            switch (vk_key) {
                case 'A': {
                    keyboard_input.a = false;
                    return 0;
                }

                case 'D': {
                    keyboard_input.d = false;
                    return 0;
                }

                case 'J': {
                    keyboard_input.j = false;
                    return 0;
                }

                case 'K': {
                    keyboard_input.k = false;
                    return 0;
                }

                case 'S': {
                    keyboard_input.s = false;
                    return 0;
                }

                default: {
                    return DefWindowProcA(window, message, w_param, l_param);
                }
            }
        }

        default: {
            return DefWindowProcA(window, message, w_param, l_param);
        }
    }
}

int WINAPI wWinMain(const HINSTANCE instance, HINSTANCE, PWSTR, int) {
    const HCURSOR window_cursor = LoadCursorA(NULL, IDC_ARROW);

    const char* const main_window_class_name = "OpenGL Window";
    WNDCLASSA main_window_class = {};
    main_window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    main_window_class.lpfnWndProc = main_window_procedure;
    main_window_class.cbClsExtra = 0;
    main_window_class.cbWndExtra = 0;
    main_window_class.hInstance = instance;
    main_window_class.hIcon = NULL;
    main_window_class.hCursor = window_cursor;
    main_window_class.hbrBackground = NULL;
    main_window_class.lpszMenuName = nullptr;
    main_window_class.lpszClassName = main_window_class_name;

    const ATOM atom = RegisterClassA(&main_window_class);
    DEBUG_ASSERT(atom != 0)

    ApplicationState application_state = {};
    const HWND window = CreateWindowExA(
        0,
        main_window_class_name,
        "Tetris AI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        instance,
        reinterpret_cast<void*>(&application_state)
    );

    DEBUG_ASSERT(window != NULL)

    const HDC window_device_context = GetDC(window);
    DEBUG_ASSERT(window_device_context != NULL)

    PIXELFORMATDESCRIPTOR pixel_format_descriptor = {};
    pixel_format_descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixel_format_descriptor.nVersion = 1;
    pixel_format_descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format_descriptor.iPixelType = PFD_TYPE_RGBA;
    pixel_format_descriptor.cColorBits = 32;
    pixel_format_descriptor.cRedBits = 0;
    pixel_format_descriptor.cRedShift =  0;
    pixel_format_descriptor.cGreenBits = 0;
    pixel_format_descriptor.cGreenShift = 0;
    pixel_format_descriptor.cBlueBits = 0;
    pixel_format_descriptor.cBlueShift = 0;
    pixel_format_descriptor.cAlphaBits = 0;
    pixel_format_descriptor.cAlphaShift = 0;
    pixel_format_descriptor.cAccumBits = 0;
    pixel_format_descriptor.cAccumRedBits = 0;
    pixel_format_descriptor.cAccumGreenBits = 0;
    pixel_format_descriptor.cAccumBlueBits =  0;
    pixel_format_descriptor.cAccumAlphaBits = 0;
    pixel_format_descriptor.cDepthBits = 16;
    pixel_format_descriptor.cStencilBits = 0;
    pixel_format_descriptor.cAuxBuffers = 0;
    pixel_format_descriptor.iLayerType = 0;
    pixel_format_descriptor.bReserved = 0;
    pixel_format_descriptor.dwLayerMask = 0;
    pixel_format_descriptor.dwVisibleMask = 0;
    pixel_format_descriptor.dwDamageMask = 0;

    const int pixel_format = ChoosePixelFormat(window_device_context, &pixel_format_descriptor);
    DEBUG_ASSERT(pixel_format != 0)

    const BOOL pixel_format_set = SetPixelFormat(window_device_context, pixel_format, &pixel_format_descriptor);
    DEBUG_ASSERT(pixel_format_set != FALSE)

    const BOOL window_was_visible = ShowWindow(window, SW_SHOW);
    DEBUG_ASSERT(window_was_visible == FALSE)

    const HGLRC render_context = wglCreateContext(window_device_context);
    DEBUG_ASSERT(render_context != NULL)

    const BOOL made_render_context_current = wglMakeCurrent(window_device_context, render_context);
    DEBUG_ASSERT(made_render_context_current != FALSE)

    RECT client_rect = {};
    const BOOL got_client_rect = GetClientRect(window, &client_rect);
    DEBUG_ASSERT(got_client_rect != FALSE)

    const int client_width = client_rect.right - client_rect.left;
    const int client_height = client_rect.bottom - client_rect.top;
    DEBUG_ASSERT(client_width > 0)
    DEBUG_ASSERT(client_height > 0)

    glViewport(0, 0, client_width, client_height);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"

    GET_OPENGL_PROC(glGenBuffers, void(*)(GLsizei, GLuint*))
    GET_OPENGL_PROC(glBindBuffer, void(*)(GLenum, GLuint))
    GET_OPENGL_PROC(glEnableVertexAttribArray, void(*)(GLuint))
    GET_OPENGL_PROC(glVertexAttribPointer, void(*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*))
    GET_OPENGL_PROC(glBufferData, void(*)(GLenum, GLsizeiptr, const GLvoid*, GLenum))
    GET_OPENGL_PROC(glCreateShader, GLuint(*)(GLenum))
    GET_OPENGL_PROC(glShaderSource, void(*)(GLuint, GLsizei, const GLchar**, const GLint*))
    GET_OPENGL_PROC(glCompileShader, void(*)(GLuint))
    GET_OPENGL_PROC(glCreateProgram, GLuint(*)())
    GET_OPENGL_PROC(glAttachShader, void(*)(GLuint, GLuint))
    GET_OPENGL_PROC(glLinkProgram, void(*)(GLuint))
    GET_OPENGL_PROC(glUseProgram, void(*)(GLuint))
    GET_OPENGL_PROC(glGetUniformLocation, GLint(*)(GLuint, const GLchar*))
    GET_OPENGL_PROC(glUniform1i, void (*)(GLint, GLint))
    GET_OPENGL_PROC(glActiveTexture, void(*)(GLenum))

#pragma clang diagnostic pop

    // TODO: use OpenGL types??
    u32 vertex_buffer_object = 0;
    glGenBuffers(1, &vertex_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(Vec2)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(Vec2)));

    u32 index_buffer_object = 0;
    glGenBuffers(1, &index_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_object);

    const u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const Resource vertex_shader_source = load_resource(instance, ID_VERTEX_SHADER);
    const char* vertex_shader_source_text = static_cast<const char*>(vertex_shader_source.data);
    const i32 vertex_shader_source_size = static_cast<i32>(vertex_shader_source.size);
    glShaderSource(vertex_shader, 1, &vertex_shader_source_text, &vertex_shader_source_size);
    glCompileShader(vertex_shader);

    const u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const Resource fragment_shader_source = load_resource(instance, ID_FRAGMENT_SHADER);
    const char* fragment_shader_source_text = static_cast<const char*>(fragment_shader_source.data);
    const i32 fragment_shader_source_size = static_cast<i32>(fragment_shader_source.size);
    glShaderSource(fragment_shader, 1, &fragment_shader_source_text, &fragment_shader_source_size);
    glCompileShader(fragment_shader);

    const u32 shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glUseProgram(shader_program);

    const i32 sampler_uniform_location = glGetUniformLocation(shader_program, "u_sampler");

    static constexpr u32 BLOCK_SIZE_PIXELS = 300;
    const Resource block_texture_resource = load_resource(instance, ID_BLOCK_TEXTURE);
    DEBUG_ASSERT(block_texture_resource.size == 4 * BLOCK_SIZE_PIXELS * BLOCK_SIZE_PIXELS)

    u32 block_texture_id = 0;
    glGenTextures(1, &block_texture_id);
    glBindTexture(GL_TEXTURE_2D, block_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BLOCK_SIZE_PIXELS, BLOCK_SIZE_PIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, block_texture_resource.data);

    const Resource font_texture_resource = load_resource(instance, ID_FONT_TEXTURE);
    DEBUG_ASSERT(font_texture_resource.size == 4 * FONT_SET_WIDTH_PIXELS * FONT_SET_HEIGHT_PIXELS)

    u32 font_texture_id = 0;
    glGenTextures(1, &font_texture_id);
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONT_SET_WIDTH_PIXELS, FONT_SET_HEIGHT_PIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_texture_resource.data);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    LARGE_INTEGER performance_counter_frequency = {};
    const BOOL read_performance_frequency = QueryPerformanceFrequency(&performance_counter_frequency);
    DEBUG_ASSERT(read_performance_frequency != FALSE)

    LARGE_INTEGER previous_tick_count = {};
    const BOOL initial_read_performance_counter = QueryPerformanceCounter(&previous_tick_count);
    DEBUG_ASSERT(initial_read_performance_counter != FALSE)

    u32 rng_seed = static_cast<u32>(previous_tick_count.QuadPart);

    static constexpr Coordinates TETRIMINO_SPAWN_LOCATION = {4, 0};
    static constexpr Coordinates NEXT_TETRIMINO_DISPLAY_LOCATION = {15, 13};

    Tetris::Grid grid = {};

    rng_seed = random_number(rng_seed);
    const Tetris::Tetrimino::Type tetrimino_type = static_cast<Tetris::Tetrimino::Type>(rng_seed % Tetris::Tetrimino::Type::COUNT);
    Tetris::Tetrimino tetrimino = construct_tetrimino(tetrimino_type, TETRIMINO_SPAWN_LOCATION);

    rng_seed = random_number(rng_seed);
    const Tetris::Tetrimino::Type initial_next_tetrimino_type = static_cast<Tetris::Tetrimino::Type>(rng_seed % Tetris::Tetrimino::Type::COUNT);
    Tetris::Tetrimino next_tetrimino = construct_tetrimino(initial_next_tetrimino_type, TETRIMINO_SPAWN_LOCATION);

    i32 player_score = 0;
    i32 total_rows_cleared = 0;
    i32 updates_since_last_drop = 0;

    f32 accumulated_time = 0.0f;
    static constexpr f32 DELTA_TIME = 1000.0f / 60.0f;

    u16 indices[6 * SCENE_TILE_COUNT] = {};
    for (u16 tile_index = 0; tile_index < SCENE_TILE_COUNT; ++tile_index) {
        indices[6 * tile_index + 0] = 4 * tile_index + 0;
        indices[6 * tile_index + 1] = 4 * tile_index + 1;
        indices[6 * tile_index + 2] = 4 * tile_index + 2;
        indices[6 * tile_index + 3] = 4 * tile_index + 2;
        indices[6 * tile_index + 4] = 4 * tile_index + 3;
        indices[6 * tile_index + 5] = 4 * tile_index + 0;
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), static_cast<const void*>(indices), GL_STATIC_DRAW);

    ApplicationState previous_application_state = application_state;

    u32 updates_down_held_count = 0;
    u32 updates_left_held_count = 0;
    u32 updates_right_held_count = 0;
    u32 updates_clockwise_held_count = 0;
    u32 updates_anti_clockwise_held_count = 0;

    // f32 clear_colour = 0.0f;

    bool quit = false;
    while (!quit) {
        LARGE_INTEGER tick_count = {};
        const BOOL read_performance_counter = QueryPerformanceCounter(&tick_count);
        DEBUG_ASSERT(read_performance_counter != FALSE)

        const f32 frame_duration = static_cast<f32>(tick_count.QuadPart - previous_tick_count.QuadPart) / static_cast<float>(performance_counter_frequency.QuadPart) * 1000.0f;
        accumulated_time += frame_duration;

        previous_tick_count = tick_count;

        while (accumulated_time >= DELTA_TIME) {
            const KeyboardInput& keyboard_input = application_state.keyboard_input;
            const KeyboardInput& previous_keyboard_input = previous_application_state.keyboard_input;

            updates_down_held_count = update_held_count(keyboard_input.s, previous_keyboard_input.s, updates_down_held_count);
            updates_left_held_count = update_held_count(keyboard_input.a, previous_keyboard_input.a, updates_left_held_count);
            updates_right_held_count = update_held_count(keyboard_input.d, previous_keyboard_input.d, updates_right_held_count);
            updates_clockwise_held_count = update_held_count(keyboard_input.k, previous_keyboard_input.k, updates_clockwise_held_count);
            updates_anti_clockwise_held_count = update_held_count(keyboard_input.j, previous_keyboard_input.j, updates_anti_clockwise_held_count);

            const i32 difficulty_level = calculate_difficulty_level(total_rows_cleared);

            const bool should_move_tetrimino_left = is_actionable_input(keyboard_input.a, previous_keyboard_input.a, updates_left_held_count);
            if (should_move_tetrimino_left) {
                tetrimino = shift(tetrimino, Coordinates{-1, 0});
                if (collision(tetrimino, grid)) {
                    tetrimino = shift(tetrimino, Coordinates{1, 0});
                }
            }

            const bool should_move_tetrimino_right = is_actionable_input(keyboard_input.d, previous_keyboard_input.d, updates_right_held_count);
            if (should_move_tetrimino_right) {
                tetrimino = shift(tetrimino, Coordinates{1, 0});
                if (collision(tetrimino, grid)) {
                    tetrimino = shift(tetrimino, Coordinates{-1, 0});
                }
            }

            // Fastest we can get is 6 drops per second
            const i32 updates_allowed = 120 - 5 * (difficulty_level - 1);
            const i32 updates_allowed_before_drop = (updates_allowed < 10) ? 10 : updates_allowed;

            bool should_merge_tetrimino_with_grid = false;
            const bool should_move_tetrimino_down = is_actionable_input(keyboard_input.s, previous_keyboard_input.s, updates_down_held_count) || updates_since_last_drop >= updates_allowed_before_drop;
            if (should_move_tetrimino_down) {
                tetrimino = shift(tetrimino, Coordinates{0, 1});
                if (collision(tetrimino, grid)) {   // Then we need to merge tetrimino to grid and spawn another
                    tetrimino = shift(tetrimino, Coordinates{0, -1});
                    should_merge_tetrimino_with_grid = true;
                }
                
                updates_since_last_drop = 0;
            }

            // TODO: should this go before attempting to drop tetrimino down/left/right?
            if (!should_merge_tetrimino_with_grid) {
                const bool should_rotate_tetrimino_clockwise = is_actionable_input(keyboard_input.k, previous_keyboard_input.k, updates_clockwise_held_count);
                if (should_rotate_tetrimino_clockwise) {
                    tetrimino = rotate(tetrimino, Tetris::Rotation::CLOCKWISE);
                    if (collision(tetrimino, grid) && !resolve_rotation_collision(tetrimino, grid)) {
                        tetrimino = rotate(tetrimino, Tetris::Rotation::ANTI_CLOCKWISE);
                    }
                }

                const bool should_rotate_tetrimino_anti_clockwise = is_actionable_input(keyboard_input.j, previous_keyboard_input.j, updates_anti_clockwise_held_count);
                if (should_rotate_tetrimino_anti_clockwise) {
                    tetrimino = rotate(tetrimino, Tetris::Rotation::ANTI_CLOCKWISE);
                    if (collision(tetrimino, grid) && !resolve_rotation_collision(tetrimino, grid)) {
                        tetrimino = rotate(tetrimino, Tetris::Rotation::CLOCKWISE);
                    }
                }
            } else {
                merge(tetrimino, grid);
                tetrimino = next_tetrimino;
                if (collision(tetrimino, grid)) {
                    // game over, reset
                    player_score = 0;
                    total_rows_cleared = 0;
                    updates_since_last_drop = 0;
                    grid = Tetris::Grid{};
                }

                rng_seed = random_number(rng_seed);
                const Tetris::Tetrimino::Type next_tetrimino_type = static_cast<Tetris::Tetrimino::Type>(rng_seed % Tetris::Tetrimino::Type::COUNT);
                next_tetrimino = construct_tetrimino(next_tetrimino_type, TETRIMINO_SPAWN_LOCATION);
            }

            const i32 rows_cleared = remove_completed_rows(grid);
            total_rows_cleared += rows_cleared;
            player_score += rows_cleared * 100 * difficulty_level;

            ++updates_since_last_drop;
            accumulated_time -= DELTA_TIME;
        }

        // glClearColor(clear_colour, clear_colour, clear_colour, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // if (application_state.keyboard_input.s && !previous_application_state.keyboard_input.s) {
        //     if (clear_colour == 0.0f) {
        //         clear_colour = 1.0f;
        //     } else {
        //         clear_colour = 0.0f;
        //     }
        // }

        Scene scene = {};
        for (i32 y = 0; y < Tetris::Grid::ROW_COUNT; ++y) {
            for (i32 x = 0; x < Tetris::Grid::COLUMN_COUNT; ++x) {
                const Tetris::Grid::Cell& cell = grid.cells[static_cast<u64>(y)][static_cast<u64>(x)];
                render_tetrimino_block(scene, static_cast<f32>(x), static_cast<f32>(y), cell);
            }
        }

        const Colour tetrimino_colour = piece_colour(tetrimino.type);
        for (i32 top_left_index = 0; top_left_index < 4; ++top_left_index) {
            const Coordinates& block_top_left = tetrimino.blocks.top_left_coordinates[top_left_index];
            render_tetrimino_block(scene, static_cast<f32>(block_top_left.x), static_cast<f32>(block_top_left.y), tetrimino_colour);
        }

        const Colour next_tetrimino_colour = piece_colour(next_tetrimino.type);
        const Tetris::Tetrimino::Blocks next_tetrimino_display_blocks = Tetris::block_locations(next_tetrimino.type, NEXT_TETRIMINO_DISPLAY_LOCATION);
        for (i32 top_left_index = 0; top_left_index < 4; ++top_left_index) {
            const Coordinates& block_top_left = next_tetrimino_display_blocks.top_left_coordinates[top_left_index];
            render_tetrimino_block(scene, static_cast<f32>(block_top_left.x), static_cast<f32>(block_top_left.y), next_tetrimino_colour);
        }

        render_text(scene, "SCORE", 13.0f, 1.0f);
        render_integer(scene, player_score, 18.0f, 2.0f);

        render_text(scene, "LEVEL", 13.0f, 4.0f);
        const i32 difficulty_level = calculate_difficulty_level(total_rows_cleared);
        render_integer(scene, difficulty_level, 16.0f, 5.0f);

        render_text(scene, "NEXT", 13.0f, 10.0f);

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(scene.vertices), reinterpret_cast<const void*>(scene.vertices), GL_STATIC_DRAW);

        // render blocks
        glUniform1i(sampler_uniform_location, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, block_texture_id);

        static constexpr u32 BLOCK_COUNT = Tetris::Grid::ROW_COUNT * Tetris::Grid::COLUMN_COUNT + 8;
        glDrawElements(GL_TRIANGLES, 6 * BLOCK_COUNT, GL_UNSIGNED_SHORT, nullptr);

        // render ui
        glUniform1i(sampler_uniform_location, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, font_texture_id);

        static constexpr u32 UI_TILE_COUNT = SCENE_TILE_COUNT - BLOCK_COUNT;
        glDrawElements(GL_TRIANGLES, 6 * UI_TILE_COUNT, GL_UNSIGNED_SHORT, reinterpret_cast<const void*>(6 * BLOCK_COUNT * sizeof(u16)));

        SwapBuffers(window_device_context);

        // TODO: need to rethink this idea. If things like a keydown and keyup happen in 1 polling session,
        // the user's input gets eaten; we may receive a keydown but if the game isn't ready to update
        // (because not enough time has accumulated) the user's input gets eaten again since (potentially)
        // a future polling session gets the keyup message; etc...
        previous_application_state = application_state;

        MSG window_message = {};
        while (PeekMessageA(&window_message, NULL, 0, 0, PM_REMOVE) != FALSE) {
            TranslateMessage(&window_message);
            DispatchMessageA(&window_message);

            if (window_message.message == WM_QUIT) {
                quit = true;
            }
        }
    }
}
