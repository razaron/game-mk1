#ifndef RZ_GAME3_SHAPECOMPONENT_HPP
#define RZ_GAME3_SHAPECOMPONENT_HPP

#include <glm/glm.hpp>

#include "Component.hpp"

namespace rz::game::components
{
    class ShapeComponent : public rz::core::Component
    {
      public:
        ShapeComponent();
        ShapeComponent(int sides, glm::u8vec3 colour);
        ~ShapeComponent();

        int sides;
        glm::u8vec3 colour;
    };
}

#endif //RZ_GAME3_SHAPECOMPONENT_HPP
