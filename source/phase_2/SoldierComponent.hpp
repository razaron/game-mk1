#ifndef RZ_GAME2_SOLDIER_AGENTCOMPONENT_HPP
#define RZ_GAME2_SOLDIER_AGENTCOMPONENT_HPP

#include "AgentComponent.hpp"

namespace rz::game::components
{
    class SoldierComponent
    {
      public:
        SoldierComponent() {}
        ~SoldierComponent() {}

        int ammo{3};
        double elapsed{};
    };
} // namespace rz::game::components

#endif //RZ_GAME2_SOLDIER_AGENTCOMPONENT_HPP