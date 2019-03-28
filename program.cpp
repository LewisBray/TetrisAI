#include "GLEW\\glew.h"

#include "program.h"

#include <algorithm>
#include <exception>
#include <utility>

Program::Program() noexcept
    : id_{ glCreateProgram() }
{}

Program::Program(Program&& other) noexcept
{
    std::swap(id_, other.id_);
}

Program& Program::operator=(Program&& other) noexcept
{
    if (this == &other)
        return *this;

    std::swap(id_, other.id_);

    return *this;
}

Program::~Program() noexcept
{
    glDeleteProgram(id_);
}

void Program::use() const noexcept
{
    glUseProgram(id_);
}

void Program::link() const noexcept
{
    glLinkProgram(id_);
}

void Program::validate() const noexcept
{
    glValidateProgram(id_);
}

void Program::attach(const Shader& shader) const noexcept
{
    glAttachShader(id_, shader.id());
}

void Program::setTextureUniform(const std::string& name, const int value) const
{
    glUniform1i(uniformLocation(name), value);
}

void Program::setUniform(const std::string& name, const std::array<float, 4>& values) const
{
    glUniform4f(uniformLocation(name), values[0], values[1], values[2], values[3]);
}

int Program::uniformLocation(const std::string& name) const
{
    const int uniformLocation = glGetUniformLocation(id_, name.c_str());
    if (uniformLocation == -1)
    {
        const std::string errorMessage = "Could not find uniform: " + name;
        throw std::exception{ errorMessage.c_str() };
    }

    return uniformLocation;
}
