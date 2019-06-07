#ifndef RZ_GAME2_INPUTSYSTEM_HPP
#define RZ_GAME2_INPUTSYSTEM_HPP

#include "LuaHooks.hpp"
#include "System.hpp"

#include <SFML/Graphics.hpp>

namespace rz::game::systems
{
    class InputSystem : public rz::core::System
    {
      public:
        InputSystem(sol::state_view lua, sf::RenderWindow *window);

        ~InputSystem();

        rz::taskscheduler::Task update(rz::core::EntityMap &entities, double delta);
        rz::core::ComponentHandle createComponent(rz::core::ComponentType type, std::shared_ptr<void> tuplePtr);
        bool removeComponent(rz::core::ComponentHandle ch);

      private:
        sol::state_view _lua;
        sf::RenderWindow *_window;

        const double _pollingRate;
        double _elapsedKeyboard;
        double _elapsedMouse;
    };
}

#endif //RZ_GAME2_INPUTSYSTEM_HPP