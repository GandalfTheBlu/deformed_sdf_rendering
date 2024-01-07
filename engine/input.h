#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>

namespace Engine
{
	struct Key
	{
	private:
		friend class Input;
		friend class Mouse;

		enum class KeyStatus : char
		{
			E_Inactive,
			E_Pressed,
			E_Held,
			E_Released
		};

		KeyStatus status;

		void LateUpdate();

	public:
		Key();

		bool WasPressed() const;
		bool IsDown() const;
		bool WasReleased() const;
	};

	struct Mouse
	{
	private:
		friend class Input;

		bool firstUpdate;

		void LateUpdate();

	public:
		Key leftButton;
		Key rightButton;
		struct
		{
			double x;
			double y;
		} position;
		struct
		{
			double dx;
			double dy;
		} movement;

		Mouse();
	};

	class Input
	{
	public:
		typedef int keyId_t;

	private:
		std::map<keyId_t, Key> idToKey;
		Mouse mouse;

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		Input();

		friend class Window;
		void HandleKeyEvent(int key, int scancode, int action, int mods);
		void HandleMousePressEvent(int button, int action, int mods);
		void HandleMouseMoveEvent(double x, double y);
		void LateUpdate();

	public:
		static Input& Instance();

		Key GetKey(keyId_t keyId);
		const Mouse& GetMouse();
	};
}