#include "RenderSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::components;
using namespace rz::game::systems;
using namespace rz::eventstream;

RenderSystem::RenderSystem(sol::state_view lua, sf::RenderWindow *window)
    : _lua{ lua }, _window{ window }
{
    _interval = 0.01;

    _componentTypes.insert(ComponentType::SHAPE);

    _lua.new_usertype<ShapeComponent>("ShapeComponent",
                                      sol::constructors<ShapeComponent()>(),
                                      "sides", &ShapeComponent::sides,
                                      "colour", &ShapeComponent::colour);

    sf::Font font;
    if (!font.loadFromFile("res/arial.ttf"))
    {
        // error...
    }

    _lua["Render"] = sol::new_table();
    _lua["Render"]["draw"] = sol::new_table();
    _lua["Render"]["draw"]["text"] = [window, font](std::string str, glm::vec2 pos, unsigned size, glm::u8vec3 col) {
        sf::Text text;

        text.setFont(font);
        text.setString(str);
        text.setPosition(pos.x, pos.y);
        text.setCharacterSize(size);
        text.setColor(sf::Color{ col.r, col.g, col.b });

        window->draw(text);
    };

    // Load game script
    auto result = _lua.script_file("renderer.lua", [](lua_State *, sol::protected_function_result pfr) {
        sol::error err = pfr;
        std::cerr << err.what() << std::endl;
        return pfr;
    });

    // Initialize game script globals
    try
    {
        sol::function func = _lua["renderer"]["init"];
        func();
    }
    catch (const sol::error &err)
    {
        std::cerr << err.what() << std::endl;
    }

    registerHandler(EVENTTYPE_MODEL, [&](const Event &e) {
        auto data = std::static_pointer_cast<EVENTDATA_MODEL>(e.data);

        _models[e.recipient] = data->model;
    });
}

RenderSystem::~RenderSystem()
{
}

Task RenderSystem::update(EntityMap &entities, double)
{
    _data.clear();

    for (auto & [ id, entity ] : entities)
    {
        if (!entity.has(ComponentType::SHAPE)) continue;
        if (_models.find(id) == _models.end()) continue;

        auto model = _models[id];
        auto shape = *getObject<ShapeComponent>(entity[ComponentType::SHAPE]);

        _data.push_back(std::make_pair(model, shape));
    }

    return Task{};
}

ComponentHandle RenderSystem::createComponent(ComponentType type, std::shared_ptr<void> tuplePtr)
{
    Handle h;

    switch (type)
    {
        case ComponentType::SHAPE:
        {
            ShapeArgs args = *(std::static_pointer_cast<ShapeArgs>(tuplePtr));

            h = emplaceObject<ShapeComponent>(std::get<1>(args), std::get<2>(args));

            sol::table obj = std::get<0>(args);
            obj["shape"] = getObject<ShapeComponent>(h);

            break;
        }
        default:
        {
            h = Handle{};
            break;
        }
    }

    return ComponentHandle{ type, h };
}

bool RenderSystem::removeComponent(ComponentHandle ch)
{
    Handle h;

    switch (ch.first)
    {
        case ComponentType::SHAPE:
            removeObject<ShapeComponent>(ch.second);
            break;
        default:
            return false;
            break;
    }

    return true;
}

void RenderSystem::render()
{
    _window->clear();

    std::vector<sf::Vertex> va;

    for (auto & [ model, shape ] : _data)
    {
        float theta = .0f;
        float delta = 2 * 3.14159 / shape.sides;
        for (int i = 0; i < shape.sides; i++)
        {
            float x = std::cos(theta);
            float y = std::sin(theta);

            glm::vec4 v1{ std::cos(theta), std::sin(theta), 0.f, 1.f };
            glm::vec4 v2{ std::cos(theta + delta), std::sin(theta + delta), 0.f, 1.f };
            glm::vec4 v3{ 0.f, 0.f, 0.f, 1.f };

            v1 = model * v1;
            v2 = model * v2;
            v3 = model * v3;

            va.emplace_back(sf::Vertex(sf::Vector2f{ v1.x, v1.y }, sf::Color(shape.colour.r, shape.colour.g, shape.colour.b)));
            va.emplace_back(sf::Vertex(sf::Vector2f{ v2.x, v2.y }, sf::Color(shape.colour.r, shape.colour.g, shape.colour.b)));
            va.emplace_back(sf::Vertex(sf::Vector2f{ v3.x, v3.y }, sf::Color(shape.colour.r, shape.colour.g, shape.colour.b)));

            theta += delta;
        }
    }

    if (va.size())
        _window->draw(&va[0], va.size(), sf::Triangles);

    // Run game script
    try
    {
        sol::function func = _lua["renderer"]["update"];
        func();
    }
    catch (const sol::error &err)
    {
        std::cerr << err.what() << std::endl;
        std::cin.get();
    }

    _window->display();

    // Cleanup
    _models.clear();
}
