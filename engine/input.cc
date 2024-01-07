#include "input.h"

namespace Engine
{
	Key::Key() :
		status(KeyStatus::E_Inactive)
	{}

	void Key::LateUpdate()
	{
		if (WasPressed())
			status = Key::KeyStatus::E_Held;
		else if (WasReleased())
			status = Key::KeyStatus::E_Inactive;
	}

	bool Key::WasPressed() const
	{
		return status == KeyStatus::E_Pressed;
	}

	bool Key::IsDown() const
	{
		return status == KeyStatus::E_Pressed || status == KeyStatus::E_Held;
	}

	bool Key::WasReleased() const
	{
		return status == KeyStatus::E_Pressed;
	}


	Mouse::Mouse() :
		firstUpdate(true)
	{
		position.x = 0.;
		position.y = 0.;
		movement.dx = 0.;
		movement.dy = 0.;
	}

	void Mouse::LateUpdate()
	{
		leftButton.LateUpdate();
		rightButton.LateUpdate();
		movement.dx = 0.;
		movement.dy = 0.;
	}


	Input::Input()
	{}

	Input& Input::Instance()
	{
		static Input instance;
		return instance;
	}

	void Input::HandleKeyEvent(int key, int scancode, int action, int mods)
	{
		if (action == 1)
			idToKey[key].status = Key::KeyStatus::E_Pressed;
		else if (action == 0)
			idToKey[key].status = Key::KeyStatus::E_Released;
	}

	void Input::HandleMousePressEvent(int button, int action, int mods)
	{
		if (button == 0)
		{
			if (action == 1)
				mouse.leftButton.status = Key::KeyStatus::E_Pressed;
			else if (action == 0)
				mouse.leftButton.status = Key::KeyStatus::E_Released;
		}
		if (button == 1)
		{
			if (action == 1)
				mouse.rightButton.status = Key::KeyStatus::E_Pressed;
			else if (action == 0)
				mouse.rightButton.status = Key::KeyStatus::E_Released;
		}
	}

	void Input::HandleMouseMoveEvent(double x, double y)
	{
		if (mouse.firstUpdate)
			mouse.firstUpdate = false;
		else
		{
			mouse.movement.dx = x - mouse.position.x;
			mouse.movement.dy = y - mouse.position.y;
		}

		mouse.position.x = x;
		mouse.position.y = y;
	}

	void Input::LateUpdate()
	{
		for (auto& elem : idToKey)
			elem.second.LateUpdate();

		mouse.LateUpdate();
	}

	Key Input::GetKey(keyId_t keyId)
	{
		return idToKey[keyId];
	}

	const Mouse& Input::GetMouse()
	{
		return mouse;
	}
}