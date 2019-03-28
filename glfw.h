#pragma once

#include "GLFW\\glfw3.h"

#include <string>

namespace GLFW
{
    class GLFW
    {
    public:
        GLFW();
        ~GLFW() noexcept;
    };

    void pollEvents() noexcept;

    class Window
    {
    public:
        Window(int width, int height, const std::string& title);

        Window(const Window& other) = delete;
        Window(Window&& other) = delete;
        Window& operator=(const Window& other) = delete;
        Window& operator=(Window&& other) = delete;

        ~Window() = default;

        bool shouldClose() const noexcept;
        void swapBuffers() const noexcept;
        void makeCurrentContext() const noexcept;

    private:
        GLFWwindow* window_ = nullptr;
    };
}
