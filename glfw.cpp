#include "glfw.h"

#include <exception>
#include <map>

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

    Window::Window(const int width, const int height, const std::string& title)
        : window_{ glfwCreateWindow(width, height, title.c_str(), NULL, NULL) }
    {
        if (window_ == nullptr)
            throw std::exception{ "Failed to create window" };
	
		glfwSetKeyCallback(window_, keyStateCallback);
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

	static std::map<char, KeyState> keyStates;

	KeyState Window::keyState(char key) const
	{
		key = std::toupper(key);
		const auto keyState = keyStates.find(key);
		if (keyState == keyStates.end())
		{
			keyStates.insert({ key, KeyState::Released });
			return KeyState::Released;
		}
		else
			return keyState->second;
	}

	void Window::keyStateCallback(GLFWwindow* const window,
		const int key, const int scancode, const int action, const int mods)
	{
		const KeyState newState = std::invoke([action]() {
			if (action == GLFW_PRESS)
				return KeyState::Pressed;
			else if (action == GLFW_REPEAT)
				return KeyState::Held;
			else
				return KeyState::Released;
		});

		const char keyAsChar = std::toupper(static_cast<char>(key));
		const auto prevState = keyStates.find(keyAsChar);
		if (prevState == keyStates.end())
			keyStates.insert({ keyAsChar, newState });
		else
			prevState->second = newState;
	}

	void pollEvents() noexcept
	{
		glfwPollEvents();
	}
}
