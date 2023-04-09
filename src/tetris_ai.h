#ifndef TETRIS_AI_H
#define TETRIS_AI_H

#include "opengl.h"

struct Resource {
    const void* data;
    u32 size;
};

struct Platform {
    void(*show_error_box)(const char* title, const char* text);
    i64(*query_performance_frequency)();
    i64(*query_performance_counter)();
    Resource(*load_resource)(i32 resource_id);

    void(*glViewport)(GLint, GLint, GLsizei, GLsizei);
    void(*glGenBuffers)(GLsizei, GLuint*);
    void(*glBindBuffer)(GLenum, GLuint);
    void(*glEnableVertexAttribArray)(GLuint);
    void(*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
    void(*glBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
    GLuint(*glCreateShader)(GLenum);
    void(*glShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
    void(*glCompileShader)(GLuint);
    GLuint(*glCreateProgram)();
    void(*glAttachShader)(GLuint, GLuint);
    void(*glLinkProgram)(GLuint);
    void(*glUseProgram)(GLuint);
    GLint(*glGetUniformLocation)(GLuint, const GLchar*);
    void(*glUniform1i)(GLint, GLint);
    void(*glGenTextures)(GLsizei, GLuint*);
    void(*glBindTexture)(GLenum, GLuint);
    void(*glTexParameteri)(GLenum, GLenum, GLint);
    void(*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
    void(*glActiveTexture)(GLenum);
    void(*glEnable)(GLenum);
    void(*glBlendFunc)(GLenum, GLenum);
    void(*glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
    void(*glClear)(GLbitfield);
    void(*glDrawElements)(GLenum, GLsizei, GLenum, const void*);
};

struct PlayerInput {
    bool left;
    bool down;
    bool right;
    bool clockwise;
    bool anti_clockwise;
};

struct GameMemory {
    static constexpr u64 permanent_storage_size = 1024 * 1024;
    void* permanent_storage;
};

extern "C" {
    void initialise_game(const GameMemory& game_memory, i32 client_width, i32 client_height, const Platform& platform);
    void update_game(const GameMemory& game_memory, const PlayerInput& player_input, const Platform& platform);
    void render_game(const GameMemory& game_memory, const Platform& platform);
}

#endif
