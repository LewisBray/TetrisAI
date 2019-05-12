#pragma once

#include "GLFW\\glfw3.h"

#include "keystate.h"

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
        Window(int width, int height, const char* title);

        Window(const Window& other) = delete;
        Window(Window&& other) = delete;
        Window& operator=(const Window& other) = delete;
        Window& operator=(Window&& other) = delete;

        ~Window() = default;

        bool shouldClose() const noexcept;
        void swapBuffers() const noexcept;
        void makeCurrentContext() const noexcept;
		KeyState keyState(char key) const;

    private:
		static void keyStateCallback(GLFWwindow* window,
			int key, int scancode, int action, int mods);

        GLFWwindow* window_ = nullptr;
    };

    void pollEvents() noexcept;
}
