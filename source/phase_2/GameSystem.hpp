#ifndef RZ_GAME2_GAMESYSTEM_HPP
#define RZ_GAME2_GAMESYSTEM_HPP

#include "LuaHooks.hpp"
#include "System.hpp"
#include "config.hpp"
#include "BaseComponent.hpp"

namespace rz::game::systems
{
    using BaseArgs = std::tuple<std::string, int>;

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
        std::vector<std::pair<UUID64, rz::game::event::data::COLLISION>> _collisions;
        std::map<UUID64, glm::vec2, UUID64Cmp> _positions;

        std::map<Team, int> _wins;
    };
}

#endif //RZ_GAME2_GAMESYSTEM_HPP