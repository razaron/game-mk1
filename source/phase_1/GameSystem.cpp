#include "GameSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::systems;

GameSystem::GameSystem(sol::state_view lua)
	: _lua(lua)
{
	_interval = 0.01;
	
	rz::lua::planner::hook(lua);

	// Load game script
	auto result = _lua.script_file("game.lua", [](lua_State*, sol::protected_function_result pfr) {
		sol::error err = pfr;
		std::cerr << err.what() << std::endl;
		return pfr;
	});

	// Initialize game script globals
	try
	{
		sol::function func = _lua["game"]["init"];
		func();
	}
	catch (const sol::error &err)
	{
		std::cerr << err.what() << std::endl;
	}
}

GameSystem::~GameSystem()
{
}

Task GameSystem::update(EntityMap &, double delta)
{
	static double elapsed{};
	elapsed += delta;
	if (elapsed < _interval)
		return Task{};
	else
		elapsed -= _interval;

	// Run game script
	try
	{
		sol::function func = _lua["game"]["update"];
		func(_interval);
	}
	catch (const sol::error &err)
	{
		std::cerr << err.what() << std::endl;
		std::cin.get();
	}

	return Task{};
}

ComponentHandle GameSystem::createComponent(ComponentType type, std::shared_ptr<void>)
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

bool GameSystem::removeComponent(ComponentHandle ch)
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
