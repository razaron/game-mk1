#include "RenderSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::components;
using namespace rz::game::systems;
using namespace rz::eventstream;

RenderSystem::RenderSystem(sol::state_view lua, sf::RenderWindow *window)
    : _lua{ lua }, _window{ window }
{
    _interval = 1.0 / 60;

    _componentTypes.insert(ComponentType{ "SHAPE" });

    _lua.new_usertype<ShapeComponent>("ShapeComponent",
                                      sol::constructors<ShapeComponent()>(),
                                      "sides", &ShapeComponent::sides,
                                      "colour", &ShapeComponent::colour);

    sf::Font font;
    if (!font.loadFromFile("res/arial.ttf"))
    {
        // error...
    }

    // EVENT HANDLERS
    registerHandler(game::event::type::MODEL, [&](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::MODEL>(e.data);

        _models[e.recipient] = data->model;
    });

    registerHandler(game::event::type::COLOUR, [&](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::COLOUR>(e.data);

        _colours.emplace_back(e.recipient, data->colour);
    });

    registerHandler(game::event::type::TEXT, [this, font](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::TEXT>(e.data);

        sf::Text text;
        text.setFont(font);
        text.setString(data->str);
        text.setPosition(data->pos.x, data->pos.y);
        text.setCharacterSize(data->size);
        text.setFillColor(sf::Color{ data->col.r, data->col.g, data->col.b });

        _text[e.recipient] = text;
    });

    registerHandler(core::event::type::SPACE_DELETE_ENTITY, [&](const Event &e) {
        _models.erase(e.recipient);
        _text.erase(e.recipient);
    });

    // LUA HOOKS
    _lua["Render"] = sol::new_table();
    _lua["Render"]["draw"] = sol::new_table();
    _lua["Render"]["draw"]["text"] = [window, font](std::string str, glm::vec2 pos, unsigned size, glm::u8vec3 col) {
        sf::Text text;

        text.setFont(font);
        text.setString(str);
        text.setPosition(pos.x, pos.y);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color{ col.r, col.g, col.b });

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
}

RenderSystem::~RenderSystem()
{
}

Task RenderSystem::update(EntityMap &entities, double delta)
{
    static double elapsed{ 0.0 };
    if ((elapsed += delta) < _interval)
        return {};
    else
        elapsed = 0.0;


    for (auto &[id, colour] : _colours)
    {
        if (!entities[id].has(ComponentType{ "SHAPE" })) continue;

        auto shape = getObject<ShapeComponent>(entities[id][ComponentType{ "SHAPE" }]);
        shape->colour = colour;
	}


	_colours.clear();
    _data.clear();
    for (auto &[id, entity] : entities)
    {
        sf::Text text;
        if (_text.find(id) != _text.end())
            text = _text[id];

        if (entity.has(ComponentType{ "SHAPE" }) && _models.find(id) != _models.end())
        {
            auto model = _models[id];
            auto shape = getObject<ShapeComponent>(entity[ComponentType{ "SHAPE" }]);
            _data.push_back(std::make_tuple(model, *shape, text));
        }
    }

    return Task{};
}

void RenderSystem::render()
{
    _window->clear();

    std::vector<sf::Vertex> va;

    // Draw the things
    for (auto &[model, shape, text] : _data)
    {
        float theta = .0f;
        float delta = 2 * 3.14159 / shape.sides;
        for (int i = 0; i < shape.sides; i++)
        {
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

    for (auto &[id, text] : _text)
        _window->draw(text);

    _window->display();
}

ComponentHandle RenderSystem::createComponent(ComponentType type, std::shared_ptr<void> tuplePtr)
{
    Handle h{};

    if (type == "SHAPE")
    {
        ShapeArgs args = *(std::static_pointer_cast<ShapeArgs>(tuplePtr));

        h = emplaceObject<ShapeComponent>(std::get<0>(args), std::get<1>(args));
    }

    return ComponentHandle{ type, h };
}

bool RenderSystem::removeComponent(ComponentHandle ch)
{
    if (ch.first == "SHAPE")
        removeObject<ShapeComponent>(ch.second);
    else
        return false;

    return true;
}