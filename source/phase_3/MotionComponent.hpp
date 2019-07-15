#ifndef RZ_GAME2_MOTIONCOMPONENT_HPP
#define RZ_GAME2_MOTIONCOMPONENT_HPP

#include "Component.hpp"

#include <glm/glm.hpp>

namespace rz::game::components
{
    class MotionComponent : public rz::core::Component
    {
      public:
        MotionComponent();
        MotionComponent(glm::vec2 velocity, glm::vec2 acceleration, float maxVelocity, float maxAcceleration, float mass);
        ~MotionComponent();

        glm::vec2 velocity, acceleration;
        float maxVelocity, maxAcceleration, mass, wanderAngle;
    };
}

#endif //RZ_GAME2_MOTIONCOMPONENT_HPP
