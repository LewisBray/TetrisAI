// TODO:
//  - remove std lib + c runtime
//  - fix bug when holding out of grid and rotating
//  - fix user input being eaten by world update loop
//  - move non-platform code out of this file
//  - stop drawing things individually, move into single buffer and do one draw call
//  - handle resizing window
//  - add padding around viewport to prevent skewing of square shapes
//  - move .png files into resource file (remove stb_image)
//  - refactor neural network stuff to not use awful lal
//  - bring neural network stuff back into game
//  - update types to use types defined by the API (DWORD, GLint, etc...)
//  - change to different font file (dwarf fortress again?)
//  - investigate using SIMD/multithreading for neural network heavy lifting

#include "tetris.h"
#include "maths.h"
#include "types.h"
#include "util.h"

#include "tetris.cpp"
#include "maths.cpp"
#include "util.cpp"

#include "resource.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_IMAGE_IMPLEMENTATION
#include "..\STBImage\stb_image.h"
#pragma clang diagnostic pop

#include <Windows.h>
#include <gl\GL.h>

// game
static i32 calculate_difficulty_level(const i32 total_rows_cleared) {
    return total_rows_cleared / 10 + 1;
}

// maths
struct Vertex {
    Vec2 position;
    Vec2 texture_coords;
};

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

// Rendering
static constexpr u32 FONT_SET_WIDTH = 16;
static constexpr u32 FONT_SET_HEIGHT = 16;
static constexpr u32 FONT_WIDTH_PIXELS = 16;
static constexpr u32 FONT_HEIGHT_PIXELS = 16;
static constexpr u32 FONT_SET_WIDTH_PIXELS = FONT_SET_WIDTH * FONT_WIDTH_PIXELS;
static constexpr u32 FONT_SET_HEIGHT_PIXELS = FONT_SET_HEIGHT * FONT_HEIGHT_PIXELS;
static constexpr f32 FONT_WIDTH_NORMALISED = static_cast<f32>(FONT_WIDTH_PIXELS) / static_cast<f32>(FONT_SET_WIDTH_PIXELS);
static constexpr f32 FONT_HEIGHT_NORMALISED = static_cast<f32>(FONT_HEIGHT_PIXELS) / static_cast<f32>(FONT_SET_HEIGHT_PIXELS);

static Vec2 texture_coordinates(const char c) {
    const u32 x = static_cast<u32>(c) % FONT_SET_WIDTH;
    const u32 y = FONT_SET_HEIGHT - static_cast<u32>(c) / FONT_SET_WIDTH;
    const f32 x_normalised = FONT_WIDTH_NORMALISED * static_cast<f32>(x);
    const f32 y_normalised = FONT_HEIGHT_NORMALISED * static_cast<f32>(y);

    return Vec2{x_normalised, y_normalised};
}

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
    GET_OPENGL_PROC(glUniform4f, void(*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat))
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

    const u16 indices[6] = { 0, 1, 2, 2, 3, 0 };

    u32 index_buffer_object = 0;
    glGenBuffers(1, &index_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_object);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), static_cast<const void*>(indices), GL_STATIC_DRAW);

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

    const i32 colour_uniform_location = glGetUniformLocation(shader_program, "u_colour");
    const i32 sampler_uniform_location = glGetUniformLocation(shader_program, "u_sampler");

    u32 block_texture_id = 0;
    glGenTextures(1, &block_texture_id);
    glBindTexture(GL_TEXTURE_2D, block_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TODO: put loaded .png files into resource section
    i32 block_texture_width = 0;
    i32 block_texture_height = 0;
    i32 block_texture_chan_count = 0;
    stbi_set_flip_vertically_on_load(true);
    const u8* const block_texture_data = stbi_load("Images\\block.png", &block_texture_width, &block_texture_height, &block_texture_chan_count, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, block_texture_width, block_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, block_texture_data);

    u32 font_texture_id = 0;
    glGenTextures(1, &font_texture_id);
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    i32 font_texture_width = 0;
    i32 font_texture_height = 0;
    i32 font_texture_chan_count = 0;
    stbi_set_flip_vertically_on_load(true);
    const u8* const font_texture_data = stbi_load("Images\\font.png", &font_texture_width, &font_texture_height, &font_texture_chan_count, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_texture_width, font_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_texture_data);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto render_tetrimino_block = [&](const Tetris::Colour& colour, const Coordinates& top_left) {
        Vertex vertices[4] = {};

        vertices[0].position.x = static_cast<f32>(top_left.x) + 0.0f;
        vertices[0].position.y = static_cast<f32>(top_left.y) + 1.0f;
        vertices[0].texture_coords.x = 0.0f;
        vertices[0].texture_coords.y = 0.0f;

        vertices[1].position.x = static_cast<f32>(top_left.x) + 1.0f;
        vertices[1].position.y = static_cast<f32>(top_left.y) + 1.0f;
        vertices[1].texture_coords.x = 1.0f;
        vertices[1].texture_coords.y = 0.0f;

        vertices[2].position.x = static_cast<f32>(top_left.x) + 1.0f;
        vertices[2].position.y = static_cast<f32>(top_left.y) + 0.0f;
        vertices[2].texture_coords.x = 1.0f;
        vertices[2].texture_coords.y = 1.0f;

        vertices[3].position.x = static_cast<f32>(top_left.x) + 0.0f;
        vertices[3].position.y = static_cast<f32>(top_left.y) + 0.0f;
        vertices[3].texture_coords.x = 0.0f;
        vertices[3].texture_coords.y = 1.0f;

        glUniform4f(colour_uniform_location, colour.r, colour.g, colour.b, colour.a);
        glUniform1i(sampler_uniform_location, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, block_texture_id);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    };

    const auto render_character = [&](const char c, const Vec2& top_left) {
        const Vec2 texture_top_left = texture_coordinates(c);

        Vertex vertices[4] = {};

        vertices[0].position.x = top_left.x + 0.0f;
        vertices[0].position.y = top_left.y + 1.0f;
        vertices[0].texture_coords.x = texture_top_left.x;
        vertices[0].texture_coords.y = texture_top_left.y - FONT_HEIGHT_NORMALISED;

        vertices[1].position.x = top_left.x + 1.0f;
        vertices[1].position.y = top_left.y + 1.0f;
        vertices[1].texture_coords.x = texture_top_left.x + FONT_WIDTH_NORMALISED;
        vertices[1].texture_coords.y = texture_top_left.y - FONT_HEIGHT_NORMALISED;

        vertices[2].position.x = top_left.x + 1.0f;
        vertices[2].position.y = top_left.y + 0.0f;
        vertices[2].texture_coords.x = texture_top_left.x + FONT_WIDTH_NORMALISED;
        vertices[2].texture_coords.y = texture_top_left.y;

        vertices[3].position.x = top_left.x + 0.0f;
        vertices[3].position.y = top_left.y + 0.0f;
        vertices[3].texture_coords.x = texture_top_left.x;
        vertices[3].texture_coords.y = texture_top_left.y;

        glUniform4f(colour_uniform_location, Tetris::WHITE.r, Tetris::WHITE.g, Tetris::WHITE.b, Tetris::WHITE.a);
        glUniform1i(sampler_uniform_location, 1);

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, font_texture_id);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    };

    const auto render_text = [&render_character](const char* text, Vec2 top_left) {
        while (*text != '\0') {
            render_character(*text, top_left);
            top_left.x += 1.0f;
            ++text;
        }
    };

    // This renders right-to-left as it comes out the easiest/most convenient, be careful!
    const auto render_integer = [&render_character](i32 integer, Vec2 top_right) {
        static constexpr i32 MAX_INT_DIGITS = 10;   // max value held in 32-bits is O(10^9), i.e. 10 digits
        char digits[MAX_INT_DIGITS] = {};
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
            const char c = digits[i] + '0';
            render_character(c, top_right);
            top_right.x -= 1.0f;
        }
    };

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

        const Tetris::Colour tetrimino_colour = piece_colour(tetrimino.type);
        for (const Coordinates& block_top_left : tetrimino.blocks.top_left_coordinates) {
            render_tetrimino_block(tetrimino_colour, block_top_left);
        }

        for (i32 y = 0; y < Tetris::Grid::ROW_COUNT; ++y) {
            for (i32 x = 0; x < Tetris::Grid::COLUMN_COUNT; ++x) {
                const auto& cell = grid.cells[static_cast<u64>(y)][static_cast<u64>(x)];
                if (!is_empty_cell(cell)) {
                    const Coordinates cell_draw_position = {x, y};
                    render_tetrimino_block(cell, cell_draw_position);
                }
            }
        }

        render_text("SCORE", Vec2{13.0f, 1.0f});
        render_integer(player_score, Vec2{18.0f, 2.0f});

        render_text("LEVEL", Vec2{13.0f, 4.0f});
        const i32 difficulty_level = calculate_difficulty_level(total_rows_cleared);
        render_integer(difficulty_level, Vec2{16.0f, 5.0f});

        render_text("NEXT", Vec2{13.0f, 10.0f});

        const Tetris::Colour next_tetrimino_colour = piece_colour(next_tetrimino.type);
        const Tetris::Tetrimino::Blocks next_tetrimino_display_blocks = Tetris::block_locations(next_tetrimino.type, NEXT_TETRIMINO_DISPLAY_LOCATION);
        for (const Coordinates& block_top_left : next_tetrimino_display_blocks.top_left_coordinates) {
            render_tetrimino_block(next_tetrimino_colour, block_top_left);
        }

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
