#include "GameSystem.hpp"
#include "InputSystem.hpp"
#include "LuaHooks.hpp"
#include "LuaCore.hpp"
#include "LuaFramework.hpp"
#include "PhysicsSystem.hpp"
#include "RenderSystem.hpp"
#include "Space.hpp"

using namespace rz::core;
using namespace rz::eventstream;
using namespace rz::game::systems;
using namespace rz::game::components;
using namespace rz::taskscheduler;

int main()
{
    std::clog.setstate(std::ios_base::failbit);

    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Phase 2: C++ Component-System");

    // Create a new Lua state and load libraries
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::math, sol::lib::os, sol::lib::debug, sol::lib::string, sol::lib::io);
    rz::lua::maths::hook(lua);
    rz::lua::planner::bind(lua);

    // Create a new space
    SystemGraph g;
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);

    g[0].data = std::make_shared<InputSystem>(lua, &window);
    g[1].data = std::make_shared<GameSystem>(lua);
    g[2].data = std::make_shared<PhysicsSystem>(lua);
    auto r = std::make_shared<RenderSystem>(lua, &window);
    g[3].data = r;

    Space s{ g };

    double delta{};
    double elapsed{};
    double elapsedT{};
    unsigned frames{};
    unsigned framesT{};

    while (window.isOpen())
    {
        auto start = std::chrono::high_resolution_clock::now();

        s.update(delta);

        r->render();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;

        delta = diff.count();
        elapsed += delta;
        ++frames;

        if (elapsed >= 10.0)
        {
            std::cout << "FPS: " << frames / 10 << "\tFrames: " << frames << "\tEntities: " << s.getEntities().size() << std::endl;
            elapsedT += elapsed;
            elapsed = 0.0;
            framesT += frames;
            frames = 0;
        }
    }

    std::cout << "Average FPS:" << framesT / elapsedT << std::endl;

    std::cout << "Press enter to continue." << std::endl;
    std::cin.get();

    return 0;
}
