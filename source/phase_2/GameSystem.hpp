#ifndef RZ_GAME2_GAMESYSTEM_HPP
#define RZ_GAME2_GAMESYSTEM_HPP

#include "AgentComponent.hpp"
#include "BaseComponent.hpp"
#include "BulletComponent.hpp"
#include "DepositComponent.hpp"
#include "Entity.hpp"
#include "LuaHooks.hpp"
#include "SoldierComponent.hpp"
#include "System.hpp"
#include "WorkerComponent.hpp"
#include "config.hpp"

namespace rz::game::systems
{
    using BaseArgs = std::tuple<Team, int>;
    using DepositArgs = std::tuple<Team, int>;
    using DepositArgs = std::tuple<Team, int>;
    using AgentArgs = std::tuple<std::string, Team, rz::planner::ActionSet>;
    using WorkerArgs = std::tuple<>;
    using SoldierArgs = std::tuple<>;
    using BulletArgs = std::tuple<UUID64, Team, glm::vec2>;

    class GameSystem : public rz::core::System
    {
      using EffectFunction = std::function<bool(const rz::core::EntityMap &, const rz::core::Entity &)>;
      
      public:
        GameSystem(sol::state_view lua);
        ~GameSystem();

        rz::taskscheduler::Task update(rz::core::EntityMap &entities, double delta);
        rz::core::ComponentHandle createComponent(rz::core::ComponentType type, std::shared_ptr<void> tuplePtr);
        bool removeComponent(rz::core::ComponentHandle ch);

      private:
        bool checkGameOver();
        void killAgent(const rz::core::Entity &entity);

        sol::state_view _lua;
        std::map<UUID64, std::vector<rz::game::event::data::COLLISION>, UUID64Cmp> _collisions;
        std::map<UUID64, glm::vec2, UUID64Cmp> _positions;

        std::map<Team, int> _wins;
        std::map<std::string, std::tuple<EffectFunction, SteeringBehaviour, EffectFunction>> _effects;
    };
} // namespace rz::game::systems

#endif //RZ_GAME2_GAMESYSTEM_HPP