#include "GLEW\\glew.h"

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "position.hpp"
#include "texture2d.h"
#include "program.h"
#include "shader.h"
#include "render.h"

#include <sstream>
#include <iomanip>
#include <string>
#include <cctype>

static constexpr float oneThird = 1.0f / 3.0f;
static constexpr float oneThirteenth = 1.0f / 13.0f;

// Will probably want to cache all these calculated values eventually
static Position<float> textureCoordinates(char c)
{
    const bool isLetter = static_cast<bool>(std::isalpha(c));
    const bool isDigit = static_cast<bool>(std::isdigit(c));

    if (!isLetter && !isDigit)
        throw std::domain_error {
            "Unexpected character attempting to be rendered"
        };

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

std::string formatScore(const int score)
{
    std::stringstream formattedScore;
    formattedScore << std::setw(6) << std::setfill('0') << score;

    return formattedScore.str();
}

using namespace Tetris;

// This will need some refactoring later but at least I can
// remove the rendering logic from the game loop logic
void renderScene(const Tetrimino& tetrimino, const Tetrimino& nextTetrimino,
    const Grid& grid, const int score, const int rowsCleared)
{
    static ArrayBuffer<float, 16> vertices{ {
        // vertex       // texture
        0.0f, 0.0f,     0.0f, 0.0f,
        0.0f, 0.0f,     1.0f, 0.0f,
        0.0f, 0.0f,     1.0f, 1.0f,
        0.0f, 0.0f,     0.0f, 1.0f
    } };

    vertices.setVertexAttribute(0, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(0));
    vertices.setVertexAttribute(1, 2, GL_FLOAT,
        4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    static const IndexBuffer<unsigned, 6> indices_{ { 0, 1, 2, 2, 3, 0 } };

    static const Shader vertexShader{
        ".\\Shaders\\vertex.shader", GL_VERTEX_SHADER };
    static const Shader fragmentShader{
        ".\\Shaders\\fragment.shader", GL_FRAGMENT_SHADER };

    static const Program shaderProgram;
    shaderProgram.attach(vertexShader);
    shaderProgram.attach(fragmentShader);

    shaderProgram.link();
    shaderProgram.validate();

    static const Texture2d blockTexture{
        ".\\Images\\block.jpg", Texture2d::ImageType::JPG };
    static const Texture2d fontTexture{
        ".\\Images\\font.png", Texture2d::ImageType::PNG };

    const auto renderTetriminoBlock = [&](const Colour& colour, const Position<int>& topLeft)
    {
        // + 0.0f here for type conversion to avoid casts
        vertices[0] = topLeft.x + 0.0f;
        vertices[1] = topLeft.y + 1.0f;
        vertices[2] = 0.0f;
        vertices[3] = 0.0f;

        vertices[4] = topLeft.x + 1.0f;
        vertices[5] = topLeft.y + 1.0f;
        vertices[6] = 1.0f;
        vertices[7] = 0.0f;

        vertices[8] = topLeft.x + 1.0f;
        vertices[9] = topLeft.y + 0.0f;
        vertices[10] = 1.0f;
        vertices[11] = 1.0f;

        vertices[12] = topLeft.x + 0.0f;
        vertices[13] = topLeft.y + 0.0f;
        vertices[14] = 0.0f;
        vertices[15] = 1.0f;

        shaderProgram.use();
        shaderProgram.setUniform("uColour", colour);
        shaderProgram.setTextureUniform("uBlock", blockTexture.number());

        vertices.bind();
        vertices.bufferData(GL_STATIC_DRAW);

        blockTexture.makeActive();

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    };

    const auto renderCharacter = [&](const char c, const Position<int>& topLeft)
    {
        const Position<float> textureTopLeft = textureCoordinates(c);

        // + 0.0f here for type conversion to avoid casts
        vertices[0] = topLeft.x + 0.0f;
        vertices[1] = topLeft.y + 1.0f;
        vertices[2] = textureTopLeft.x;
        vertices[3] = textureTopLeft.y - oneThird;

        vertices[4] = topLeft.x + 1.0f;
        vertices[5] = topLeft.y + 1.0f;
        vertices[6] = textureTopLeft.x + oneThirteenth;
        vertices[7] = textureTopLeft.y - oneThird;

        vertices[8] = topLeft.x + 1.0f;
        vertices[9] = topLeft.y + 0.0f;
        vertices[10] = textureTopLeft.x + oneThirteenth;
        vertices[11] = textureTopLeft.y;

        vertices[12] = topLeft.x + 0.0f;
        vertices[13] = topLeft.y + 0.0f;
        vertices[14] = textureTopLeft.x;
        vertices[15] = textureTopLeft.y;

        shaderProgram.use();
        shaderProgram.setUniform("uColour", { 1.0f, 1.0f, 1.0f, 1.0f });
        shaderProgram.setTextureUniform("uBlock", fontTexture.number());

        vertices.bind();
        vertices.bufferData(GL_STATIC_DRAW);

        fontTexture.makeActive();

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    };

    const auto renderText = [&](const std::string_view& text, Position<int> topLeft)
    {
        for (const char c : text) {
            renderCharacter(c, topLeft);
            ++topLeft.x;
        }
    };

    glClear(GL_COLOR_BUFFER_BIT);
    
    const Tetrimino::Blocks& tetriminoBlocks = tetrimino.blocks();
    for (const Position<int>& blockTopLeft : tetriminoBlocks)
    {
        const Position<int> blockDrawPosition = { blockTopLeft.x + 1, blockTopLeft.y };
        renderTetriminoBlock(tetrimino.colour(), blockDrawPosition);
    }

    for (int row = 0; row < Grid::Rows; ++row)
    {
        for (int col = 0; col < Grid::Columns; ++col)
        {
            const Grid::Cell& cell = grid[row][col];
            if (!cell.has_value())
                continue;

            const Position<int> cellDrawPosition = { col + 1, row };
            renderTetriminoBlock(cell.value(), cellDrawPosition);
        }
    }

    for (int y = 0; y < Grid::Rows; ++y)
    {
        const Position<int> leftOfGridBlock = { 0, y };
        const Position<int> rightOfGridBlock = { 1 + Grid::Columns, y };
        renderTetriminoBlock(Grey, leftOfGridBlock);
        renderTetriminoBlock(Grey, rightOfGridBlock);
    }

    const Tetrimino::Blocks nextTetriminoDisplayBlocks =
        nextTetriminoBlockDisplayLocations(nextTetrimino.type());
    for (const Position<int>& blockTopLeft : nextTetriminoDisplayBlocks)
        renderTetriminoBlock(nextTetrimino.colour(), blockTopLeft);

    renderText("SCORE", { 13, 1 });
    renderText(formatScore(score), { 13, 2 });

    const std::string level = std::to_string(rowsCleared / 10 + 1);
    renderText("LEVEL", { 13, 4 });
    renderText(level, { 17 - static_cast<int>(level.length()), 5 });

    const std::string rows = std::to_string(rowsCleared);
    renderText("ROWS", { 13, 7 });
    renderText(rows, { 17 - static_cast<int>(rows.length()), 8 });

    renderText("NEXT", { 13, 10 });
}
