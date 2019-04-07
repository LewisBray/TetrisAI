#pragma once

#include "GLFW\\glfw3.h"

#include "keystate.h"

#include <string>

namespace GLFW
{
    class GLFW
    {
    public:
        GLFW();
        ~GLFW() noexcept;
    };

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
		KeyState getKeyState(char key) const noexcept;

    private:
        GLFWwindow* window_ = nullptr;
    };

    void pollEvents() noexcept;
}
