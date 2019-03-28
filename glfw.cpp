#include "glfw.h"

#include <exception>

namespace GLFW
{
    GLFW::GLFW()
    {
        if (!glfwInit())
            throw std::exception{ "Failed to initialise GLFW" };
    }

    GLFW::~GLFW() noexcept
    {
        glfwTerminate();
    }

    void pollEvents() noexcept
    {
        glfwPollEvents();
    }

    Window::Window(const int width, const int height, const std::string& title)
        : window_{ glfwCreateWindow(width, height, title.c_str(), NULL, NULL) }
    {
        if (window_ == nullptr)
            throw std::exception{ "Failed to create window" };
    }

    bool Window::shouldClose() const noexcept
    {
        return glfwWindowShouldClose(window_);
    }

    void Window::swapBuffers() const noexcept
    {
        glfwSwapBuffers(window_);
    }

    void Window::makeCurrentContext() const noexcept
    {
        glfwMakeContextCurrent(window_);
    }
}
