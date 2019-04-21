#include "GLEW\\glew.h"

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "position.hpp"
#include "texture2d.h"
#include "program.h"
#include "shader.h"
#include "render.h"

#include <cctype>

BlockDrawer::BlockDrawer()
{
    vertices_.setVertexAttribute(0, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(0));
    vertices_.setVertexAttribute(1, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    const Shader vertexShader(".\\Shaders\\vertex.shader", GL_VERTEX_SHADER);
    const Shader fragmentShader(".\\Shaders\\fragment.shader", GL_FRAGMENT_SHADER);

    program_.attach(vertexShader);
    program_.attach(fragmentShader);

    program_.link();
    program_.validate();
}

void BlockDrawer::operator()(const Position<int>& topLeft, const Colour& colour)
{
    // + 0.0f here for type conversion to avoid casts
    vertices_[0] = topLeft.x + 0.0f;
    vertices_[1] = topLeft.y + 1.0f;

    vertices_[4] = topLeft.x + 1.0f;
    vertices_[5] = topLeft.y + 1.0f;

    vertices_[8] = topLeft.x + 1.0f;
    vertices_[9] = topLeft.y + 0.0f;

    vertices_[12] = topLeft.x + 0.0f;
    vertices_[13] = topLeft.y + 0.0f;

    program_.use();
    program_.setUniform("uColour", colour);
    program_.setTextureUniform("uBlock", texture_.number());
    
    vertices_.bind();
    vertices_.bufferData(GL_STATIC_DRAW);

    indices_.bind();
    indices_.bufferData(GL_STATIC_DRAW);

    texture_.makeActive();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

TextDrawer::TextDrawer()
{
    vertices_.setVertexAttribute(0, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(0));
    vertices_.setVertexAttribute(1, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    const Shader vertexShader(".\\Shaders\\vertex.shader", GL_VERTEX_SHADER);
    const Shader fragmentShader(".\\Shaders\\fragment.shader", GL_FRAGMENT_SHADER);

    program_.attach(vertexShader);
    program_.attach(fragmentShader);

    program_.link();
    program_.validate();
}

void TextDrawer::operator()(const char c, const Position<int>& topLeft)
{
    const Position<float> textureTopLeft = textureCoordinates(c);

    // + 0.0f here for type conversion to avoid casts
    vertices_[0] = topLeft.x + 0.0f;
    vertices_[1] = topLeft.y + 1.0f;
    vertices_[2] = textureTopLeft.x;
    vertices_[3] = textureTopLeft.y - oneThird;

    vertices_[4] = topLeft.x + 1.0f;
    vertices_[5] = topLeft.y + 1.0f;
    vertices_[6] = textureTopLeft.x + oneThirteenth;
    vertices_[7] = textureTopLeft.y - oneThird;

    vertices_[8] = topLeft.x + 1.0f;
    vertices_[9] = topLeft.y + 0.0f;
    vertices_[10] = textureTopLeft.x + oneThirteenth;
    vertices_[11] = textureTopLeft.y;

    vertices_[12] = topLeft.x + 0.0f;
    vertices_[13] = topLeft.y + 0.0f;
    vertices_[14] = textureTopLeft.x;
    vertices_[15] = textureTopLeft.y;

    program_.use();
    program_.setUniform("uColour", { 1.0f, 1.0f, 1.0f, 1.0f });
    program_.setTextureUniform("uBlock", texture_.number());

    vertices_.bind();
    vertices_.bufferData(GL_STATIC_DRAW);

    indices_.bind();
    indices_.bufferData(GL_STATIC_DRAW);

    texture_.makeActive();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void TextDrawer::operator()(const std::string_view& text, Position<int> topLeft)
{
    for (const char c : text)
    {
        (*this)(c, topLeft);
        ++topLeft.x;
    }
}

// Will probably want to cache all these calculated values eventually
Position<float> TextDrawer::textureCoordinates(char c)
{
    const bool isLetter = std::isalpha(c);
    const bool isDigit = std::isdigit(c);

    if (!isLetter && !isDigit)
        throw std::exception{ "Unexpected character attempting to be rendered" };

    if (isLetter)
    {
        c = std::toupper(c);
        const int alphabetPosition = c - 'A';
        const bool inFirstHalfOfAlphabet = (alphabetPosition / 13 == 0);
        const float top = 1.0f - (inFirstHalfOfAlphabet ? 0.0f : oneThird);
        const float left = (alphabetPosition % 13) * oneThirteenth;

        return { left, top };
    }
    else
    {
        const int numericalValue = c - '0';
        const float left = numericalValue * oneThirteenth;
        return { left, oneThird };
    }
}
