// TODO:
//  - handle resizing window
//  - add padding around viewport to prevent skewing of square shapes
//  - investigate using SIMD/multithreading for neural network heavy lifting
//  - menu for different modes (player controlled/recording; ai controller; train ai; playback training data; etc...)

#include "tetris_ai.h"

#include <Windows.h>
#include <gl\GL.h>

#define GET_OPENGL_PROC(name, type) \
auto name = reinterpret_cast<type>(wglGetProcAddress(#name));

#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"

static void show_error_box(const i8* const title, const i8* const text) {
    MessageBoxA(NULL, text, title, MB_ICONERROR);
}

#define DEBUG_ASSERT(condition) if (!(condition)) show_error_box("Debug Assert", #condition)

static Resource load_resource(const i32 id) {
    const HRSRC resource = FindResourceA(NULL, MAKEINTRESOURCEA(id), RT_RCDATA);
    DEBUG_ASSERT(resource != NULL);
    const HGLOBAL resource_data = LoadResource(NULL, resource);
    DEBUG_ASSERT(resource_data != NULL);
    const void* data = LockResource(resource_data);
    DEBUG_ASSERT(data != nullptr);
    const u32 size = SizeofResource(NULL, resource);
    DEBUG_ASSERT(size > 0);

    return Resource{data, size};
}

static i64 query_performance_frequency() {
    LARGE_INTEGER frequency = {};
    const BOOL read_frequency = QueryPerformanceFrequency(&frequency);
    DEBUG_ASSERT(read_frequency != FALSE);
    return frequency.QuadPart;
}

static i64 query_performance_counter() {
    LARGE_INTEGER counter = {};
    const BOOL read_counter = QueryPerformanceCounter(&counter);
    DEBUG_ASSERT(read_counter != FALSE);
    return counter.QuadPart;
}

static bool is_valid_handle(const HANDLE handle) {
    return handle != NULL && handle != INVALID_HANDLE_VALUE;
}

static bool open_file(const i8* const file_name, const FileAccessFlags file_access_flags, const FileCreationFlags file_creation_flag, File& file) {
    DWORD access = 0;
    if ((file_access_flags & FileAccessFlags::READ) != 0) {
        access |= GENERIC_READ;
    }

    if ((file_access_flags & FileAccessFlags::WRITE) != 0) {
        access |= GENERIC_WRITE;
    }

    static constexpr DWORD CREATION_FLAGS_TRANSLATION[] = {OPEN_EXISTING, CREATE_ALWAYS, OPEN_ALWAYS};
    const DWORD creation = CREATION_FLAGS_TRANSLATION[file_creation_flag];

    file.handle = CreateFileA(file_name, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
    return file.handle != INVALID_HANDLE_VALUE;
}

static u32 get_file_size(const File& file) {
    DEBUG_ASSERT(is_valid_handle(file.handle));

    LARGE_INTEGER file_size = {};
    const BOOL got_file_size = GetFileSizeEx(file.handle, &file_size);
    DEBUG_ASSERT(got_file_size != FALSE);

    return static_cast<u32>(file_size.QuadPart);
}

static u32 read_file_into_buffer(const File& file, void* const buffer, const u32 bytes_to_read) {
    DEBUG_ASSERT(is_valid_handle(file.handle));

    DWORD bytes_read = 0;
    const BOOL file_read = ReadFile(file.handle, buffer, static_cast<DWORD>(bytes_to_read), &bytes_read, nullptr);
    DEBUG_ASSERT(file_read != FALSE);
    DEBUG_ASSERT(bytes_read == bytes_to_read);

    return bytes_read;
}

static u32 write_buffer_into_file(const File& file, const void* const buffer, const u32 bytes_to_write) {
    DEBUG_ASSERT(is_valid_handle(file.handle));

    DWORD bytes_written = 0;
    const BOOL file_written = WriteFile(file.handle, buffer, static_cast<DWORD>(bytes_to_write), &bytes_written, nullptr);
    DEBUG_ASSERT(file_written != FALSE);
    DEBUG_ASSERT(bytes_written == bytes_to_write);

    return bytes_written;
}

static void close_file(File& file) {
    DEBUG_ASSERT(is_valid_handle(file.handle));

    CloseHandle(file.handle);
    file.handle = NULL;
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

static ApplicationState& get_application_state(const HWND window) {
    const LONG_PTR user_data = GetWindowLongPtrA(window, GWLP_USERDATA);
    ApplicationState* const application_state = reinterpret_cast<ApplicationState*>(user_data);
    DEBUG_ASSERT(application_state != nullptr);
    return *application_state;
}

static LRESULT main_window_procedure(const HWND window, const UINT message, const WPARAM w_param, const LPARAM l_param) {
    DEBUG_ASSERT(window != NULL);

    switch (message) {
        case WM_CREATE: {
            DEBUG_ASSERT(l_param != 0);
            CREATESTRUCTA* const create_struct = reinterpret_cast<CREATESTRUCTA*>(l_param);
            DEBUG_ASSERT(create_struct->lpCreateParams != nullptr); // window should be created with CreateWindowEx
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_struct->lpCreateParams));

            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        case WM_KEYDOWN: {
            ApplicationState& application_state = get_application_state(window);
            KeyboardInput& keyboard_input = application_state.keyboard_input;
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
            ApplicationState& application_state = get_application_state(window);
            KeyboardInput& keyboard_input = application_state.keyboard_input;
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

static FILETIME find_last_write_time(const i8* const file_name) {
    WIN32_FIND_DATAA find_data = {};
    const HANDLE find_file_handle = FindFirstFileA(file_name, &find_data);
    DEBUG_ASSERT(find_file_handle != NULL);

    const BOOL closed_handle = FindClose(find_file_handle);
    DEBUG_ASSERT(closed_handle != FALSE);

    return find_data.ftLastWriteTime;
}

struct GameCode {
    HMODULE dll;
    decltype(&initialise_game) initialise_proc;
    decltype(&update_game) update_proc;
    decltype(&render_game) render_proc;
};

static constexpr const i8* dll_name = "tetris_ai.dll";

static GameCode load_game_code() {
    static constexpr const i8* temp_dll_name = "tetris_ai_temp.dll";
    
    GameCode game_code = {};

    // This is a bit weird but this routine gets called after building and 
    // freeing the game code .dll, something still has either the .dll or
    // the copied .dll still locked and so this copy file call can fail.
    // Without anyway to synchronise we just call the routine over and over
    // until it works (which presumably means the files are not locked
    // anymore), this won't be in user code anyway so probably fine...
    while (CopyFileA(dll_name, temp_dll_name, FALSE) == FALSE) {}

    game_code.dll = LoadLibraryA(temp_dll_name);
    DEBUG_ASSERT(game_code.dll != NULL);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"

    game_code.initialise_proc = reinterpret_cast<decltype(game_code.initialise_proc)>(GetProcAddress(game_code.dll, "initialise_game"));
    game_code.update_proc = reinterpret_cast<decltype(game_code.update_proc)>(GetProcAddress(game_code.dll, "update_game"));
    game_code.render_proc = reinterpret_cast<decltype(game_code.render_proc)>(GetProcAddress(game_code.dll, "render_game"));

#pragma clang diagnostic pop

    DEBUG_ASSERT(game_code.initialise_proc != nullptr);
    DEBUG_ASSERT(game_code.update_proc != nullptr);
    DEBUG_ASSERT(game_code.render_proc != nullptr);

    return game_code;
}

extern "C" [[noreturn]] void WinMainCRTStartup(); // shuts up compiler warning instructing to make static
extern "C" [[noreturn]] void WinMainCRTStartup() {
    const HINSTANCE instance = GetModuleHandle(NULL);

    GameCode game_code = load_game_code();
    FILETIME previous_game_dll_last_write_time = find_last_write_time(dll_name);

    const HCURSOR window_cursor = LoadCursorA(NULL, IDC_ARROW);

    const char* const main_window_class_name = "Tetris AI Window";
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
    DEBUG_ASSERT(atom != 0);

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

    DEBUG_ASSERT(window != NULL);

    const HDC window_device_context = GetDC(window);
    DEBUG_ASSERT(window_device_context != NULL);

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
    DEBUG_ASSERT(pixel_format != 0);

    const BOOL pixel_format_set = SetPixelFormat(window_device_context, pixel_format, &pixel_format_descriptor);
    DEBUG_ASSERT(pixel_format_set != FALSE);

    const HGLRC render_context = wglCreateContext(window_device_context);
    DEBUG_ASSERT(render_context != NULL);

    const BOOL made_render_context_current = wglMakeCurrent(window_device_context, render_context);
    DEBUG_ASSERT(made_render_context_current != FALSE);

    RECT client_rect = {};
    const BOOL got_client_rect = GetClientRect(window, &client_rect);
    DEBUG_ASSERT(got_client_rect != FALSE);

    const int client_width = client_rect.right - client_rect.left;
    const int client_height = client_rect.bottom - client_rect.top;
    DEBUG_ASSERT(client_width > 0);
    DEBUG_ASSERT(client_height > 0);

    Platform platform = {};
    platform.show_error_box = show_error_box;
    platform.query_performance_frequency = query_performance_frequency;
    platform.query_performance_counter = query_performance_counter;
    platform.load_resource = load_resource;
    platform.open_file = open_file;
    platform.get_file_size = get_file_size;
    platform.read_file_into_buffer = read_file_into_buffer;
    platform.write_buffer_into_file = write_buffer_into_file;
    platform.close_file = close_file;

    platform.glViewport = glViewport;
    platform.glGenTextures = glGenTextures;
    platform.glBindTexture = glBindTexture;
    platform.glTexParameteri = glTexParameteri;
    platform.glTexImage2D = glTexImage2D;
    platform.glEnable = glEnable;
    platform.glBlendFunc = glBlendFunc;
    platform.glClearColor = glClearColor;
    platform.glClear = glClear;
    platform.glDrawElements = glDrawElements;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"

    GET_OPENGL_PROC(glGenBuffers, decltype(platform.glGenBuffers))
    GET_OPENGL_PROC(glBindBuffer, decltype(platform.glBindBuffer))
    GET_OPENGL_PROC(glEnableVertexAttribArray, decltype(platform.glEnableVertexAttribArray))
    GET_OPENGL_PROC(glVertexAttribPointer, decltype(platform.glVertexAttribPointer))
    GET_OPENGL_PROC(glBufferData, decltype(platform.glBufferData))
    GET_OPENGL_PROC(glCreateShader, decltype(platform.glCreateShader))
    GET_OPENGL_PROC(glShaderSource, decltype(platform.glShaderSource))
    GET_OPENGL_PROC(glCompileShader, decltype(platform.glCompileShader))
    GET_OPENGL_PROC(glCreateProgram, decltype(platform.glCreateProgram))
    GET_OPENGL_PROC(glAttachShader, decltype(platform.glAttachShader))
    GET_OPENGL_PROC(glLinkProgram, decltype(platform.glLinkProgram))
    GET_OPENGL_PROC(glUseProgram, decltype(platform.glUseProgram))
    GET_OPENGL_PROC(glGetUniformLocation, decltype(platform.glGetUniformLocation))
    GET_OPENGL_PROC(glUniform1i, decltype(platform.glUniform1i))
    GET_OPENGL_PROC(glActiveTexture, decltype(platform.glActiveTexture))

#pragma clang diagnostic pop

    DEBUG_ASSERT(glGenBuffers != nullptr);
    DEBUG_ASSERT(glBindBuffer != nullptr);
    DEBUG_ASSERT(glEnableVertexAttribArray != nullptr);
    DEBUG_ASSERT(glVertexAttribPointer != nullptr);
    DEBUG_ASSERT(glBufferData != nullptr);
    DEBUG_ASSERT(glCreateShader != nullptr);
    DEBUG_ASSERT(glShaderSource != nullptr);
    DEBUG_ASSERT(glCompileShader != nullptr);
    DEBUG_ASSERT(glCreateProgram != nullptr);
    DEBUG_ASSERT(glAttachShader != nullptr);
    DEBUG_ASSERT(glLinkProgram != nullptr);
    DEBUG_ASSERT(glUseProgram != nullptr);
    DEBUG_ASSERT(glGetUniformLocation != nullptr);
    DEBUG_ASSERT(glUniform1i != nullptr);
    DEBUG_ASSERT(glActiveTexture != nullptr);

    platform.glGenBuffers = glGenBuffers;
    platform.glBindBuffer = glBindBuffer;
    platform.glEnableVertexAttribArray = glEnableVertexAttribArray;
    platform.glVertexAttribPointer = glVertexAttribPointer;
    platform.glBufferData = glBufferData;
    platform.glCreateShader = glCreateShader;
    platform.glShaderSource = glShaderSource;
    platform.glCompileShader = glCompileShader;
    platform.glCreateProgram = glCreateProgram;
    platform.glAttachShader = glAttachShader;
    platform.glLinkProgram = glLinkProgram;
    platform.glUseProgram = glUseProgram;
    platform.glGetUniformLocation = glGetUniformLocation;
    platform.glUniform1i = glUniform1i;
    platform.glActiveTexture = glActiveTexture;

    GameMemory game_memory = {};
    game_memory.permanent_storage = VirtualAlloc(0, game_memory.PERMANENT_STORAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DEBUG_ASSERT(game_memory.permanent_storage != nullptr);
    game_memory.transient_storage = VirtualAlloc(0, game_memory.TRANSIENT_STORAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DEBUG_ASSERT(game_memory.transient_storage != nullptr);

    game_code.initialise_proc(game_memory, client_width, client_height, platform);

    const BOOL window_was_visible = ShowWindow(window, SW_SHOW);
    DEBUG_ASSERT(window_was_visible == FALSE);

    bool quit = false;
    while (!quit) {
        PlayerInput player_input = {};
        player_input.left = application_state.keyboard_input.a;
        player_input.down = application_state.keyboard_input.s;
        player_input.right = application_state.keyboard_input.d;
        player_input.clockwise = application_state.keyboard_input.k;
        player_input.anti_clockwise = application_state.keyboard_input.j;

        game_code.update_proc(game_memory, player_input, platform);
        game_code.render_proc(game_memory, platform);

        SwapBuffers(window_device_context);

        MSG window_message = {};
        while (PeekMessageA(&window_message, NULL, 0, 0, PM_REMOVE) != FALSE) {
            TranslateMessage(&window_message);
            DispatchMessageA(&window_message);

            if (window_message.message == WM_QUIT) {
                quit = true;
            }
        }

        const FILETIME game_dll_last_write_time = find_last_write_time(dll_name);
        const bool game_code_modified = CompareFileTime(&previous_game_dll_last_write_time, &game_dll_last_write_time) != 0;
        if (game_code_modified) {
            FreeLibrary(game_code.dll);
            game_code = load_game_code();
            previous_game_dll_last_write_time = game_dll_last_write_time;
        }
    }

    ExitProcess(0);
}
