#ifndef RZ_GAME2_GAMESYSTEM_HPP
#define RZ_GAME2_GAMESYSTEM_HPP

#include "LuaHooks.hpp"
#include "System.hpp"
#include "config.hpp"

namespace rz::game::systems
{
    class GameSystem : public rz::core::System
    {
      public:
        GameSystem(sol::state_view lua);
        ~GameSystem();

        rz::taskscheduler::Task update(rz::core::EntityMap &entities, double delta);
        rz::core::ComponentHandle createComponent(rz::core::ComponentType type, std::shared_ptr<void> tuplePtr);
        bool removeComponent(rz::core::ComponentHandle ch);

      private:
        sol::state_view _lua;
        std::vector<std::pair<UUID64, EVENTDATA_COLLISION>> _collisions;
    };
}

#endif //RZ_GAME2_GAMESYSTEM_HPP