#pragma once

#include "GLEW\\glew.h"

#include <type_traits>
#include <utility>
#include <array>

template <typename T, std::size_t Size>
class ArrayBuffer
{
    static_assert(std::is_arithmetic_v<T>, "ArrayBuffer must be of numeric type");

public:
    explicit ArrayBuffer(const std::array<T, Size>& data)
        : data_{ data }
    {
        glGenBuffers(1, &id_);
        this->bind();
        this->bufferData(GL_STATIC_DRAW);
    }

    ArrayBuffer(const ArrayBuffer& other) = delete;
    ArrayBuffer& operator=(const ArrayBuffer& other) = delete;

    ArrayBuffer(ArrayBuffer&& other) noexcept
        : id_{ other.id_ }
        , data_{ std::move(other.data_) }
    {
        other.id_ = 0;
    }

    ArrayBuffer& operator=(ArrayBuffer&& other) noexcept
    {
        if (this == &other)
            return *this;

        std::swap(id_, other.id_);
        std::swap(data_, other.data_);

        return *this;
    }

    ~ArrayBuffer() noexcept
    {
        this->unbind();
        glDeleteBuffers(1, &id_);
    }

    T& operator[](const int index)
    {
        return data_[index];
    }

    const T& operator[](const int index) const
    {
        return data_[index];
    }

    constexpr unsigned id() const noexcept
    {
        return id_;
    }

    void bufferData(const GLenum usage) const noexcept
    {
        glBufferData(GL_ARRAY_BUFFER,
            data_.size() * sizeof(T), data_.data(), usage);
    }

    void bind() const noexcept
    {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
    }

    static void unbind() noexcept
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    static void setVertexAttribute(const unsigned index,
        const int size, const GLenum dataType,
        const int stride, const void* const pointer) noexcept
    {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, size, dataType, GL_FALSE, stride, pointer);
    }

private:
    unsigned id_ = 0;
    std::array<T, Size> data_;
};