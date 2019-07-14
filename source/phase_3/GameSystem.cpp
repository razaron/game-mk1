#include "GameSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::systems;
using namespace rz::eventstream;

GameSystem::GameSystem(sol::state_view lua)
	: _lua(lua)
{
	_interval = 0.01;

	lua.new_usertype<EVENTDATA_COLLISION>("EVENTDATA_COLLISION",
		sol::constructors<EVENTDATA_COLLISION(UUID64, float, int)>(),
		"target", &EVENTDATA_COLLISION::target,
		"distance", &EVENTDATA_COLLISION::distance);

	lua["getCollisions"] = [&](UUID64 id, int group) {
		std::vector<EVENTDATA_COLLISION> collisions;

		_collisions.erase(std::remove_if(_collisions.begin(), _collisions.end(), [&collisions, id, group](const std::pair<UUID64, EVENTDATA_COLLISION> &e) {
			if (e.first == id && e.second.group == group)
			{
				collisions.push_back(e.second);
				return true;
			}
			else
				return false;
			}
		),_collisions.end());
		
		return collisions;
	};

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


	registerHandler(EVENTTYPE_COLLISION, [&](const Event &e) {
		auto data = std::static_pointer_cast<EVENTDATA_COLLISION>(e.data);

		_collisions.push_back(std::make_pair(e.recipient, *data));
		}
	);
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

	_collisions.clear();

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
