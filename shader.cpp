#include "GLEW\\glew.h"

#include "shader.h"

#include <utility>
#include <sstream>
#include <fstream>

Shader::Shader(const std::string& sourceFilepath, const GLenum type)
    : id_{ glCreateShader(type) }
{
    static const auto readFileContents = [](const std::string& filepath)
    {
        std::ifstream inputFile{ filepath };

        std::stringstream fileContents;
        fileContents << inputFile.rdbuf();

        return fileContents.str();
    };

    const std::string sourceString = readFileContents(sourceFilepath);
    const char* const source = sourceString.c_str();

    glShaderSource(id_, 1, &source, 0);
    glCompileShader(id_);
}

Shader::Shader(Shader&& other) noexcept
{
    std::swap(id_, other.id_);
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this == &other)
        return *this;

    std::swap(id_, other.id_);

    return *this;
}

Shader::~Shader() noexcept
{
    glDeleteShader(id_);
}
