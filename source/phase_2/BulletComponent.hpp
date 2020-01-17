#ifndef RZ_GAME2_BULLETCOMPONENT_HPP
#define RZ_GAME2_BULLETCOMPONENT_HPP

#include "config.hpp"

namespace rz::game::components
{
    class BulletComponent : public rz::core::Component
    {
      public:
        BulletComponent(UUID64 ownerID, Team team, glm::vec2 origin) : ownerID{ownerID}, team{team}, origin{origin} {}
        BulletComponent() {}
        ~BulletComponent() {}

        UUID64 ownerID{0};
        Team team{Team::NONE};
        glm::vec2 origin{};
   };
} // namespace rz::game::components

#endif //RZ_GAME2_BULLETCOMPONENT_HPP