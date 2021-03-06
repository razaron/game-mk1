#include "GameSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::systems;
using namespace rz::eventstream;

GameSystem::GameSystem(sol::state_view lua)
	: _lua(lua)
{
	_interval = 0.01;

	lua.new_usertype<game::event::data::COLLISION>("COLLISION",
										  sol::constructors<game::event::data::COLLISION(UUID64, float, int)>(),
										  "target", &game::event::data::COLLISION::target,
										  "distance", &game::event::data::COLLISION::distance);

	lua["getCollisions"] = [&](UUID64 id, int group) {
		std::vector<game::event::data::COLLISION> collisions;

		_collisions.erase(std::remove_if(_collisions.begin(), _collisions.end(), [&collisions, id, group](const std::pair<UUID64, game::event::data::COLLISION> &e) {
							  if (e.first == id && e.second.group == group)
							  {
								  collisions.push_back(e.second);
								  return true;
							  }
							  else
								  return false;
						  }),
						  _collisions.end());

		return collisions;
	};

	// Load game script
	auto result = _lua.script_file("game.lua", [](lua_State *, sol::protected_function_result pfr) {
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

	registerHandler(game::event::type::COLLISION, [&](const Event &e) {
		auto data = std::static_pointer_cast<game::event::data::COLLISION>(e.data);

		_collisions.push_back(std::make_pair(e.recipient, *data));
	});
}

GameSystem::~GameSystem()
{
}

Task GameSystem::update(EntityMap &, double delta)
{
	WorkFunc main = [this, delta]() {
		static double elapsed{};
		elapsed += delta;
		if (elapsed < _interval)
			return;
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
	};

	_parentTask = _taskScheduler->push(main, _parentTask);

	return{};
}

ComponentHandle GameSystem::createComponent(ComponentType, std::shared_ptr<void>)
{
	return {};
}

bool GameSystem::removeComponent(ComponentHandle)
{
	return false;
}
