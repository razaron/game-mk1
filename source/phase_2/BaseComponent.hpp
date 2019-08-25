#ifndef RZ_GAME2_BASECOMPONENT_HPP
#define RZ_GAME2_BASECOMPONENT_HPP

#include "Component.hpp"

namespace rz::game::components
{
    class BaseComponent : public rz::core::Component
    {
      public:
        BaseComponent(std::string team, int metal) : team{ team }, metal{ metal } {}
        BaseComponent() {}
        ~BaseComponent() {}

        std::string team;
        int metal;
        int resuppling;
    };
} // namespace rz::game::components

#endif //RZ_GAME2_COLLIDERCOMPONENT_HPP