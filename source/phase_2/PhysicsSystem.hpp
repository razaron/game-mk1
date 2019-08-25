#ifndef RZ_GAME2_PHYSICSSYSTEM_HPP
#define RZ_GAME2_PHYSICSSYSTEM_HPP

#include "ColliderComponent.hpp"
#include "MotionComponent.hpp"
#include "System.hpp"
#include "TransformComponent.hpp"
#include "config.hpp"

#include <glm/glm.hpp>
#include <sol.hpp>

namespace rz::game::systems
{
    using TransformArgs = std::tuple<glm::vec2, glm::vec2, float>;
    using MotionArgs = std::tuple<glm::vec2, glm::vec2, float, float, float>;
    using ColliderArgs = std::tuple<float, int>;

    class PhysicsSystem : public rz::core::System
    {
      public:
        PhysicsSystem(sol::state_view lua);
        ~PhysicsSystem();

        rz::taskscheduler::Task update(rz::core::EntityMap &, double);
        rz::core::ComponentHandle createComponent(rz::core::ComponentType type, std::shared_ptr<void>);
        bool removeComponent(rz::core::ComponentHandle ch);

      private:
        sol::state_view _lua;
        std::map<UUID64, std::pair<SteeringBehaviour, UUID64>, UUID64Cmp> _behaviours;
    };
}

#endif //RZ_GAME2_PHYSICSSYSTEM_HPP