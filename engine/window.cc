#include "window.h"
#include "input.h"
#include <string>
#include <iostream>
#include <cassert>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

void GLAPIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::string msg = "[OPENGL] ";

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		msg.append("[LOW SEVERITY] ");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		msg.append("[MEDIUM SEVERITY] ");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		msg.append("[HIGH SEVERITY] ");
		break;
	}

	msg.append(message);

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "[ERROR] " << msg << std::endl;
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "[WARNING] " << msg << std::endl;
		break;
	}
}

namespace Engine
{
	Window::Window() :
		width(-1),
		height(-1),
		window(nullptr)
	{}

	Window::~Window() {}

	void Window::StaticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
		Input::Instance().HandleKeyEvent(key, scancode, action, mods);
	}

	void Window::StaticMousePressCallback(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		Input::Instance().HandleMousePressEvent(button, action, mods);
	}

	void Window::StaticMouseMoveCallback(GLFWwindow* win, double x, double y)
	{
		Input::Instance().HandleMouseMoveEvent(x, y);
	}

	void Window::Init(int _width, int _height, const char* title)
	{
		// setup glfw to get a window
		assert(glfwInit());

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_RED_BITS, 8);
		glfwWindowHint(GLFW_GREEN_BITS, 8);
		glfwWindowHint(GLFW_BLUE_BITS, 8);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

		width = _width;
		height = _height;
		window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		glfwMakeContextCurrent(window);

		glfwSetKeyCallback(window, Window::StaticKeyCallback);
		glfwSetMouseButtonCallback(this->window, Window::StaticMousePressCallback);
		glfwSetCursorPosCallback(this->window, Window::StaticMouseMoveCallback);

		glfwSwapInterval(1);

		// setup glew to get opengl functions
		assert(glewInit() == GLEW_OK);

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		if (glDebugMessageCallback)
		{
			glDebugMessageCallback(GLDebugCallback, nullptr);
		}

		// setup imgui
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init();
		glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glViewport(0, 0, width, height);
	}

	void Window::Deinit()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	bool Window::ShouldClose()
	{
		return glfwWindowShouldClose(window);
	}

	void Window::BeginUpdate()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Window::EndUpdate()
	{
		Input::Instance().LateUpdate();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	int Window::Width() const
	{
		return width;
	}

	int Window::Height() const
	{
		return height;
	}

	void Window::SetMouseVisible(bool visible)
	{
		glfwSetInputMode(window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
}