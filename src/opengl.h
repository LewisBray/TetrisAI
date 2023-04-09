#ifndef OPENGL_H
#define OPENGL_H

#include "types.h"

using GLenum = u32;
using GLboolean = u8;
using GLint = i32;
using GLsizei = i32;
using GLuint = u32;
using GLvoid = void;
using GLchar = char;
using GLfloat = f32;
using GLsizeiptr = i64;
using GLbitfield = u32;

static constexpr GLboolean GL_FALSE = 0;

static constexpr GLint GL_LINEAR = 0x2601;
static constexpr GLint GL_REPEAT = 0x2901;

static constexpr GLenum GL_TRIANGLES = 0x0004;
static constexpr GLenum GL_SRC_ALPHA = 0x0302;
static constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
static constexpr GLenum GL_BLEND = 0x0BE2;
static constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
static constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
static constexpr GLenum GL_UNSIGNED_SHORT = 0x1403;
static constexpr GLenum GL_FLOAT = 0x1406;
static constexpr GLenum GL_RGBA = 0x1908;
static constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
static constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
static constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
static constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;
static constexpr GLenum GL_TEXTURE0 = 0x84C0;
static constexpr GLenum GL_TEXTURE1 = GL_TEXTURE0 + 1;
static constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
static constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
static constexpr GLenum GL_STATIC_DRAW = 0x88E4;
static constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
static constexpr GLenum GL_VERTEX_SHADER = 0x8B31;

static constexpr GLbitfield GL_COLOR_BUFFER_BIT = 0x00004000;

#endif
