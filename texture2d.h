#pragma once

#include <string>

class Texture2d
{
public:
    enum class ImageType { JPG, PNG };

    Texture2d(int number, const std::string& filepath, ImageType imageType);

    Texture2d(const Texture2d& other) = delete;
    Texture2d& operator=(const Texture2d& other) = delete;

    Texture2d(Texture2d&& other) noexcept;
    Texture2d& operator=(Texture2d&& other) noexcept;

    ~Texture2d() noexcept;

    constexpr int width() const noexcept { return width_; }
    constexpr int height() const noexcept { return height_; }
    constexpr int number() const noexcept { return number_; }
    constexpr unsigned id() const noexcept { return id_; }
    std::string filepath() const { return filepath_; }

    void bind() const noexcept;
    void makeActive() const noexcept;
    static void unbind() noexcept;

private:
    unsigned id_ = 0;
    int number_ = 0;
    int width_ = 0;
    int height_ = 0;
    int numChans_ = 0;
    unsigned char* data_ = nullptr;
    std::string filepath_;
};
