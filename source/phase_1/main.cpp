#include "GameSystem.hpp"
#include "InputSystem.hpp"
#include "RenderSystem.hpp"
#include "Space.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

using namespace rz::core;
using namespace rz::game::systems;
using namespace std::chrono_literals;

int main()
{
    std::clog.setstate(std::ios_base::failbit);

    sf::RenderWindow window(sf::VideoMode(1024, 1024), "Phase 1: Lua Prototype");

    sol::state lua;
    lua["log"] = false;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::math);
    rz::lua::maths::hook(lua);

    SystemGraph g;
    g.addEdge(0, 1);
    g.addEdge(1, 2);

    g[0].data = std::make_shared<InputSystem>(lua, &window);
    g[1].data = std::make_shared<GameSystem>(lua);
    g[2].data = std::make_shared<RenderSystem>(lua, &window);

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

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;

        delta = diff.count();
        elapsed += delta;
        ++frames;

        if (elapsed >= 10.0)
        {
            std::cout << "FPS: " << frames / 10 << "\tFrames: " << frames << std::endl;
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