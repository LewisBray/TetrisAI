#include "GLEW\\glew.h"

#define STB_IMAGE_IMPLEMENTATION
#include "STBImage\\stb_image.h"

#include "position.hpp"
#include "render.h"
#include "tetris.h"

#include <functional>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cctype>

// Manages lifetime of OpenGL resources (like programs, buffers, etc...) via RAII
template <typename Function>
class OpenGLID
{
public:
    OpenGLID(const unsigned id, Function release)
        : id_{ id }
        , release_{ release }
    {}

    ~OpenGLID() { release_(id_); }

    OpenGLID(const OpenGLID&) = delete;
    OpenGLID(OpenGLID&&) = default;
    OpenGLID& operator=(const OpenGLID&) = delete;
    OpenGLID& operator=(OpenGLID&&) = default;

    unsigned get() const noexcept { return id_; }

private:
    unsigned id_ = 0u;
    Function release_{};
};

unsigned generateBuffer()
{
    unsigned id = 0;
    glGenBuffers(1, &id);
    return id;
};

static unsigned generateShader(const char* const sourceFilepath, const GLenum type)
{
    const unsigned id = glCreateShader(type);

    const std::string sourceString = std::invoke([sourceFilepath]() {
        std::ifstream inputFile{ sourceFilepath };

        std::stringstream fileContents;
        fileContents << inputFile.rdbuf();

        return fileContents.str();
        });

    const char* const source = sourceString.c_str();
    glShaderSource(id, 1, &source, 0);
    glCompileShader(id);

    return id;
};

enum class ImageType { JPG, PNG };

static unsigned generateTexture(const char* const filepath, const ImageType imageType)
{
    if (imageType != ImageType::JPG && imageType != ImageType::PNG)
        throw std::domain_error{ "Unhandled texture image type" };

    if (imageType == ImageType::PNG)
        stbi_set_flip_vertically_on_load(true);
    else
        stbi_set_flip_vertically_on_load(false);

    int width_ = 0, height_ = 0, numChans_ = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> data{
        stbi_load(filepath, &width_, &height_, &numChans_, 0), stbi_image_free };
    if (!data)
    {
        const std::string errorMessage = std::string("Failed to load image: ") + filepath;
        throw std::runtime_error{ errorMessage.c_str() };
    }

    unsigned id = 0u;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int internalFormat = (imageType == ImageType::PNG) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
        width_, height_, 0, internalFormat, GL_UNSIGNED_BYTE, data.get());

    glGenerateMipmap(GL_TEXTURE_2D);

    return id;
};

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
        const int alphabetPosition = static_cast<int>(c - 'A');
        const bool inFirstHalfOfAlphabet = (alphabetPosition / 13 == 0);
        const float top = 1.0f - (inFirstHalfOfAlphabet ? 0.0f : oneThird);
        const float left = (alphabetPosition % 13) * oneThirteenth;

        return { left, top };
    }
    else
    {
        const int numericalValue = static_cast<int>(c - '0');
        const float left = numericalValue * oneThirteenth;
        return { left, oneThird };
    }
}

static std::string formatScore(const int score)
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
    constexpr auto deleteArrayBuffer = [](unsigned& id) { glDeleteBuffers(1, &id); };
    using ArrayBufferID = OpenGLID<decltype(deleteArrayBuffer)>;
    struct QuadrilateralArrayBuffer
    {
        ArrayBufferID id;
        std::array<float, 16> buffer;
    };

    static QuadrilateralArrayBuffer vertices{
        ArrayBufferID(generateBuffer(), deleteArrayBuffer), 
        // vertex       // texture
        0.0f, 0.0f,     0.0f, 0.0f,
        0.0f, 0.0f,     1.0f, 0.0f,
        0.0f, 0.0f,     1.0f, 1.0f,
        0.0f, 0.0f,     0.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vertices.id.get());

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    struct QuadrilateralIndexBuffer
    {
        ArrayBufferID id;
        std::array<unsigned, 6> buffer;
    };

    static QuadrilateralIndexBuffer indices{
        ArrayBufferID(generateBuffer(), deleteArrayBuffer),
        0, 1, 2, 2, 3, 0
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices.id.get());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.buffer.size() * sizeof(unsigned), indices.buffer.data(), GL_STATIC_DRAW);

    using ShaderID = OpenGLID<decltype(glDeleteShader)>;

    static const ShaderID vertexShader(generateShader(".\\Shaders\\vertex.shader", GL_VERTEX_SHADER), glDeleteShader);
    static const ShaderID fragmentShader(generateShader(".\\Shaders\\fragment.shader", GL_FRAGMENT_SHADER), glDeleteShader);

    using ProgramID = OpenGLID<decltype(glDeleteProgram)>;
    static const ProgramID shaderProgram(glCreateProgram(), glDeleteProgram);
    glAttachShader(shaderProgram.get(), vertexShader.get());
    glAttachShader(shaderProgram.get(), fragmentShader.get());

    glValidateProgram(shaderProgram.get());
    glLinkProgram(shaderProgram.get());

    constexpr auto deleteTexture = [](unsigned& id) { glDeleteTextures(1, &id); };
    using TextureID = OpenGLID<decltype(deleteTexture)>;
    struct Texture
    {
        TextureID id;
        unsigned number;
    };

    constexpr auto createTextureID = [deleteTexture](const char* const filepath, const ImageType imageType) {
        return TextureID(generateTexture(filepath, imageType), deleteTexture);
    };

    static const Texture blockTexture{ createTextureID(".\\Images\\block.jpg", ImageType::JPG), 0 };
    static const Texture fontTexture{ createTextureID(".\\Images\\font.png", ImageType::PNG), 1 };

    static const auto getUniformLocation = [](const unsigned program, const char* const name)
    {
        const int uniformLocation = glGetUniformLocation(program, name);
        if (uniformLocation == -1)
        {
            const std::string errorMessage =
                std::string("Could not find uniform: ") + name;
            throw std::invalid_argument{ errorMessage.c_str() };
        }

        return uniformLocation;
    };

    const auto renderTetriminoBlock = [&](const Colour& colour, const Position<int>& topLeft)
    {
        // + 0.0f here for type conversion to avoid casts
        vertices.buffer[0] = topLeft.x + 0.0f;
        vertices.buffer[1] = topLeft.y + 1.0f;
        vertices.buffer[2] = 0.0f;
        vertices.buffer[3] = 0.0f;

        vertices.buffer[4] = topLeft.x + 1.0f;
        vertices.buffer[5] = topLeft.y + 1.0f;
        vertices.buffer[6] = 1.0f;
        vertices.buffer[7] = 0.0f;

        vertices.buffer[8] = topLeft.x + 1.0f;
        vertices.buffer[9] = topLeft.y + 0.0f;
        vertices.buffer[10] = 1.0f;
        vertices.buffer[11] = 1.0f;

        vertices.buffer[12] = topLeft.x + 0.0f;
        vertices.buffer[13] = topLeft.y + 0.0f;
        vertices.buffer[14] = 0.0f;
        vertices.buffer[15] = 1.0f;

        glUseProgram(shaderProgram.get());
        const int colourUniformLocation = getUniformLocation(shaderProgram.get(), "uColour");
        glUniform4f(colourUniformLocation, colour[0], colour[1], colour[2], colour[3]);
        const int blockUniformLocation = getUniformLocation(shaderProgram.get(), "uBlock");
        glUniform1i(blockUniformLocation, blockTexture.number);

        glBindBuffer(GL_ARRAY_BUFFER, vertices.id.get());
        glBufferData(GL_ARRAY_BUFFER,
           vertices.buffer.size() * sizeof(float), vertices.buffer.data(), GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0 + blockTexture.number);
        glBindTexture(GL_TEXTURE_2D, blockTexture.id.get());

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    };

    const auto renderCharacter = [&](const char c, const Position<int>& topLeft)
    {
        const Position<float> textureTopLeft = textureCoordinates(c);

        // + 0.0f here for type conversion to avoid casts
        vertices.buffer[0] = topLeft.x + 0.0f;
        vertices.buffer[1] = topLeft.y + 1.0f;
        vertices.buffer[2] = textureTopLeft.x;
        vertices.buffer[3] = textureTopLeft.y - oneThird;

        vertices.buffer[4] = topLeft.x + 1.0f;
        vertices.buffer[5] = topLeft.y + 1.0f;
        vertices.buffer[6] = textureTopLeft.x + oneThirteenth;
        vertices.buffer[7] = textureTopLeft.y - oneThird;

        vertices.buffer[8] = topLeft.x + 1.0f;
        vertices.buffer[9] = topLeft.y + 0.0f;
        vertices.buffer[10] = textureTopLeft.x + oneThirteenth;
        vertices.buffer[11] = textureTopLeft.y;

        vertices.buffer[12] = topLeft.x + 0.0f;
        vertices.buffer[13] = topLeft.y + 0.0f;
        vertices.buffer[14] = textureTopLeft.x;
        vertices.buffer[15] = textureTopLeft.y;

        glUseProgram(shaderProgram.get());
        const int colourUniformLocation = getUniformLocation(shaderProgram.get(), "uColour");
        glUniform4f(colourUniformLocation, White[0], White[1], White[2], White[3]);
        const int blockUniformLocation = getUniformLocation(shaderProgram.get(), "uBlock");
        glUniform1i(blockUniformLocation, fontTexture.number);

        glBindBuffer(GL_ARRAY_BUFFER, vertices.id.get());
        glBufferData(GL_ARRAY_BUFFER,
            vertices.buffer.size() * sizeof(float), vertices.buffer.data(), GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0 + fontTexture.number);
        glBindTexture(GL_TEXTURE_2D, fontTexture.id.get());

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    };

    const auto renderText = [&renderCharacter](const std::string_view& text, Position<int> topLeft)
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
