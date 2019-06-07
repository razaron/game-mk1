#include "InputSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::systems;

InputSystem::InputSystem(sol::state_view lua, sf::RenderWindow *window)
	: _lua(lua), _window{ window }, _pollingRate{0.1}, _elapsedKeyboard{}
{
	_interval = 0.01;

	// Load game script
	auto result = _lua.script_file("input.lua", [](lua_State*, sol::protected_function_result pfr) {
		sol::error err = pfr;
		std::cerr << err.what() << std::endl;
		return pfr;
	});

	// Initialize game script globals
	try
	{
		sol::function func = _lua["input"]["init"];
		func();
	}
	catch (const sol::error &err)
	{
		std::cerr << err.what() << std::endl;
	}
}

InputSystem::~InputSystem()
{
}

Task InputSystem::update(EntityMap &, double delta)
{
	_elapsedKeyboard += delta;
	_elapsedMouse += delta;

	try
	{
		sol::function func = _lua["input"]["update"];
		func(delta);
	}
	catch (const sol::error &err)
	{
		std::cerr << err.what() << std::endl;
	}

	auto processKey = [&](std::string key, bool isReleased = false) {
		try
		{
			sol::function func = _lua["input"]["handlers"][key];
			func(isReleased);
		}
		catch (const sol::error &err)
		{
			std::cerr << err.what() << std::endl;
			std::cin.get();
		}
	};

	auto processMouse = [&](std::string button, int x, int y) {
		try
		{
			sol::function func = _lua["input"]["handlers"][button];
			func(x, y);
		}
		catch (const sol::error &err)
		{
			std::cerr << err.what() << std::endl;
			std::cin.get();
		}
	};

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		processKey("w");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		processKey("s");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		processKey("a");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		processKey("d");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1))
		processKey("1");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2))
		processKey("2");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3))
		processKey("3");
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4))
		processKey("4");

	sf::Event event;
	while (_window->pollEvent(event))
	{
		switch (event.type)
		{
			case sf::Event::Closed:
			{
				_window->close();
				break;
			}
			case sf::Event::KeyReleased:
			{
				switch (event.key.code)
				{
					case sf::Keyboard::W:
					{
						processKey("w", true);
						break;
					}
					case sf::Keyboard::S:
					{
						processKey("s", true);
						break;
					}
					case sf::Keyboard::A:
					{
						processKey("a", true);
						break;
					}
					case sf::Keyboard::D:
					{
						processKey("d", true);
						break;
					}
					case sf::Keyboard::Num1:
					{
						processKey("1", true);
						break;
					}
					case sf::Keyboard::Num2:
					{
						processKey("2", true);
						break;
					}
					case sf::Keyboard::Num3:
					{
						processKey("3", true);
						break;
					}
					case sf::Keyboard::Num4:
					{
						processKey("4", true);
						break;
					}
				}
				break;
			}
			case sf::Event::MouseButtonPressed:
			{
				switch (event.mouseButton.button)
				{
					case sf::Mouse::Left:
					{
						processMouse("m1", event.mouseButton.x, event.mouseButton.y);
						break;
					}
					case sf::Mouse::Right:
					{
						processMouse("m2", event.mouseButton.x, event.mouseButton.y);
						break;
					}
				}
				break;
			}
			case sf::Event::MouseMoved:
			{
				processMouse("move", event.mouseMove.x, event.mouseMove.y);
				break;
			}
		}
	}

	return Task{};
}

ComponentHandle InputSystem::createComponent(ComponentType type, std::shared_ptr<void>)
{
	Handle h;

	switch (type)
	{
		default:
			h = Handle{};
			break;
	}

	return ComponentHandle{ type, h };
}

bool InputSystem::removeComponent(ComponentHandle ch)
{
	Handle h;

	switch (ch.first)
	{
		default:
			return false;
			break;
	}

	return true;
}
