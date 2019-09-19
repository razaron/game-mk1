#ifndef RZ_GAME2_BASECOMPONENT_HPP
#define RZ_GAME2_BASECOMPONENT_HPP

#include "Component.hpp"
#include "config.hpp"

namespace rz::game::components
{
    class BaseComponent : public rz::core::Component
    {
      public:
        BaseComponent(Team team, int metal) : team{ team }, metal{ metal } {}
        BaseComponent() {}
        ~BaseComponent() {}

        Team team{Team::NONE};
        int metal{5};
        int serving{};
    };
} // namespace rz::game::components

#endif //RZ_GAME2_BASECOMPONENT_HPP