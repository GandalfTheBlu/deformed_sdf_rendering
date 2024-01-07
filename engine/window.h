#pragma once
#include "imgui.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Engine
{
	class Window
	{
	private:
		int width;
		int height;
		GLFWwindow* window;

		static void StaticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void StaticMousePressCallback(GLFWwindow* window, int button, int action, int mods);
		static void StaticMouseMoveCallback(GLFWwindow* win, double x, double y);

	public:
		Window();
		~Window();

		void Init(int _width, int _height, const char* title);
		void Deinit();

		bool ShouldClose();
		void BeginUpdate();
		void EndUpdate();

		int Width() const;
		int Height() const;

		void SetMouseVisible(bool visible);
	};
}