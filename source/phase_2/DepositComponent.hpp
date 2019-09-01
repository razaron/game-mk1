#ifndef RZ_GAME2_DEPOSITCOMPONENT_HPP
#define RZ_GAME2_DEPOSITCOMPONENT_HPP

#include "Component.hpp"
#include "config.hpp"

namespace rz::game::components
{
    class DepositComponent : public rz::core::Component
    {
      public:
        DepositComponent(Team team, int metal) : team{ team }, metal{ metal } {if(team != Team::NONE) control[static_cast<int>(team)] = 1.f; }
        DepositComponent() {}
        ~DepositComponent() {}

        Team team{Team::NONE};
        int metal;
        double elapsed;
        std::array<float, 4> control;
        int serving;
    };
} // namespace rz::game::components

#endif //RZ_GAME2_DEPOSITCOMPONENT_HPP