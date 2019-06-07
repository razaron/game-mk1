#ifndef RZ_GAME1_RENDERSYSTEM_HPP
#define RZ_GAME1_RENDERSYSTEM_HPP

#include "LuaHooks.hpp"
#include "System.hpp"

#include <SFML/Graphics.hpp>

namespace rz::game::systems
{
    class RenderSystem : public rz::core::System
    {
      public:
        RenderSystem(sol::state_view lua, sf::RenderWindow *window);

        ~RenderSystem();

        rz::taskscheduler::Task update(rz::core::EntityMap &entities, double delta);
        rz::core::ComponentHandle createComponent(rz::core::ComponentType type, std::shared_ptr<void> tuplePtr);
        bool removeComponent(rz::core::ComponentHandle ch);

      private:
        sol::state_view _lua;
        sf::RenderWindow *_window;
    };
}

#endif //RZ_GAME1_RENDERSYSTEM_HPP