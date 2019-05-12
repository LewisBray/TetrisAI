#include "GLEW\\glew.h"

#define STB_IMAGE_IMPLEMENTATION
#include "STBImage\\stb_image.h"

#include "texture2d.h"

#include <stdexcept>
#include <utility>

Texture2d::Texture2d(const char* const filepath, const ImageType imageType)
    : number_{ totalTextureCount++ }
    , filepath_{ filepath }
{
    if (imageType != ImageType::JPG && imageType != ImageType::PNG)
        throw std::domain_error{ "Unhandled texture image type" };

    if (imageType == ImageType::PNG)
        stbi_set_flip_vertically_on_load(true);
    else
        stbi_set_flip_vertically_on_load(false);

    data_ = stbi_load(filepath_.c_str(), &width_, &height_, &numChans_, 0);
    if (data_ == nullptr)
    {
        const std::string errorMessage = "Failed to load image: " + filepath_;
        throw std::runtime_error{ errorMessage.c_str() };
    }

    glGenTextures(1, &id_);
    this->bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int internalFormat = (imageType == ImageType::PNG) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
        width_, height_, 0, internalFormat, GL_UNSIGNED_BYTE, data_);

    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture2d::Texture2d(Texture2d&& other) noexcept
    : id_{ other.id_ }
    , width_{ other.width_ }
    , height_{ other.height_ }
    , numChans_{ other.numChans_ }
    , data_{ other.data_ }
    , filepath_{ other.filepath_ }
{
    other.id_ = 0;
    other.width_ = 0;
    other.height_ = 0;
    other.numChans_ = 0;
    other.data_ = nullptr;
    other.filepath_.clear();
}

Texture2d& Texture2d::operator=(Texture2d&& other) noexcept
{
    if (this == &other)
        return *this;

    std::swap(id_, other.id_);
    std::swap(width_, other.width_);
    std::swap(height_, other.height_);
    std::swap(numChans_, other.numChans_);
    std::swap(data_, other.data_);
    std::swap(filepath_, other.filepath_);

    return *this;
}

Texture2d::~Texture2d() noexcept
{
    --totalTextureCount;
    glDeleteTextures(1, &id_);
    if (data_ != nullptr)
        stbi_image_free(data_);
}

void Texture2d::bind() const noexcept
{
    glBindTexture(GL_TEXTURE_2D, id_);
}

void Texture2d::makeActive() const noexcept
{
    glActiveTexture(GL_TEXTURE0 + number_);
    this->bind();
}

void Texture2d::unbind() noexcept
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
