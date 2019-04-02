#pragma once

#include "GLEW\\glew.h"

#include <type_traits>
#include <utility>
#include <array>

template <typename T, std::size_t Size>
class IndexBuffer
{
    static_assert(std::is_arithmetic_v<T>, "IndexBuffer must be of numeric type");

public:
    explicit IndexBuffer(const std::array<T, Size>& data)
        : data_{ data }
    {
        glGenBuffers(1, &id_);
        this->bind();
        this->bufferData(GL_STATIC_DRAW);
    }

    IndexBuffer(const IndexBuffer& other) = delete;
    IndexBuffer& operator=(const IndexBuffer& other) = delete;

    IndexBuffer(IndexBuffer&& other)
        : id_{ other.id_ }
        , data_{ std::move(other.data_) }
    {
        other.id_ = 0;
    }

    IndexBuffer& operator=(IndexBuffer&& other)
    {
        if (this == &other)
            return *this;

        std::swap(id_, other.id_);
        std::swap(data_, other.data_);

        return *this;
    }

    ~IndexBuffer() noexcept
    {
        this->unbind();
        glDeleteBuffers(1, &id_);
    }

    constexpr unsigned id() const noexcept
    {
        return id_;
    }

    void bufferData(const GLenum usage) const noexcept
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            data_.size() * sizeof(T), data_.data(), usage);
    }

    void bind() const noexcept
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
    }

    static void unbind() noexcept
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

private:
    unsigned id_ = 0;
    std::array<T, Size> data_;
};
