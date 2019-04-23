#pragma once

#include "GLEW\\glew.h"

#include <string>

class Shader
{
public:
    Shader(const std::string& sourceFilepath, GLenum type);

    Shader(const Shader& other) = delete;
    Shader& operator=(const Shader& other) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    ~Shader() noexcept;

    constexpr unsigned id() const noexcept { return id_; }

private:
    unsigned id_ = 0;
};
