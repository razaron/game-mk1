#ifndef RZ_GAME2_TRANSFORMCOMPONENT_HPP
#define RZ_GAME2_TRANSFORMCOMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Component.hpp"

namespace rz::game::components
{
    class TransformComponent : public rz::core::Component
    {
      public:
        TransformComponent();
        TransformComponent(glm::vec2 translation, glm::vec2 scale, float rotation);
        ~TransformComponent();

        glm::mat4 getModel();

        glm::vec2 translation;
        glm::vec2 scale;
        float getRotation() { return _rotation; }
        float setRotation(float rotation);

      private:
        float _rotation; // in radians
    };
}

#endif //RZ_GAME2_TRANSFORMCOMPONENT_HPP
