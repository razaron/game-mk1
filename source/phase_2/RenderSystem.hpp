#ifndef RZ_GAME2_RENDERSYSTEM_HPP
#define RZ_GAME2_RENDERSYSTEM_HPP

#include "ShapeComponent.hpp"
#include "System.hpp"
#include "config.hpp"

#include <SFML/Graphics.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(push)
#pragma warning(disable : 4996)
#include <sol.hpp>
#pragma warning(pop)

namespace rz::game::systems
{
    using ShapeArgs = std::tuple<sol::object, int, glm::u8vec3>;

    struct Camera
    {
        glm::vec2 pos;
        float zoom;

        glm::mat4 getViewMatrix()
        {
            return glm::translate(glm::mat4{}, glm::vec3{});
        }
    };

    class RenderSystem : public rz::core::System
    {
      public:
        RenderSystem(sol::state_view lua, sf::RenderWindow *window);
        ~RenderSystem();

        rz::taskscheduler::Task update(rz::core::EntityMap &entities, double delta);
        rz::core::ComponentHandle createComponent(rz::core::ComponentType type, std::shared_ptr<void> tuplePtr);
        bool removeComponent(rz::core::ComponentHandle ch);

        void render();

      private:
        sol::state_view _lua;
        sf::RenderWindow *_window;

        std::vector<std::pair<glm::mat4, rz::game::components::ShapeComponent>> _data;
        std::map<UUID64, glm::mat4, UUID64Cmp> _models;
    };
}

#endif //RZ_GAME2_RENDERSYSTEM_HPP