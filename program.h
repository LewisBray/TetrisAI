#pragma once

#include "shader.h"

#include <array>

class Program
{
public:
    Program() noexcept;

    Program(const Program& other) = delete;
    Program& operator=(const Program& other) = delete;

    Program(Program&& other) noexcept;
    Program& operator=(Program&& other) noexcept;

    ~Program() noexcept;

    constexpr unsigned id() const noexcept { return id_; }

    void use() const noexcept;
    void link() const noexcept;
    void validate() const noexcept;
    void attach(const Shader& shader) const noexcept;
    void setTextureUniform(const char* name, int value) const;
    void setUniform(const char* name, const std::array<float, 4>& values) const;

private:
    int uniformLocation(const char* name) const;

    unsigned id_ = 0;
};

