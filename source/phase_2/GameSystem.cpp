#include "GameSystem.hpp"

using namespace rz::core;
using namespace rz::taskscheduler;
using namespace rz::game::systems;
using namespace rz::eventstream;
using namespace rz::game::components;

GameSystem::GameSystem(sol::state_view lua)
    : _lua(lua)
{
    _interval = 0.01;

    // Valid ComponentTypes
    _componentTypes.insert(ComponentType{ "BASE" });

    // EVENT HANDLERS
    registerHandler(game::event::type::MODEL, [&](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::MODEL>(e.data);

        _positions[e.recipient] = glm::vec2{ data->model[3] };
    });

    registerHandler(game::event::type::COLLISION, [&](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::COLLISION>(e.data);

        _collisions.push_back(std::make_pair(e.recipient, *data));
    });

    // Lua hooks
    lua["newBase"] = [this, &lua](std::string team, glm::vec2 pos, glm::u8vec3 col) {
        Event e{
            UUID64{ 0 },                     // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 25.6f, 25.6f } / 2.f, -3.14159 / 4) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(4, col) },
                ComponentArgs{ ComponentType{ "BASE" }, std::make_shared<BaseArgs>(team, 10) },
            })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    // Load game script
    auto result = _lua.script_file("game.lua", [](lua_State *, sol::protected_function_result pfr) {
        sol::error err = pfr;
        std::cerr << err.what() << std::endl;
        return pfr;
    });

    // Run game initializer script
    try
    {
        sol::function func = _lua["game"]["init"];
        func();
    }
    catch (const sol::error &err)
    {
        std::cerr << err.what() << std::endl;
    }
}

GameSystem::~GameSystem()
{
}

Task GameSystem::update(EntityMap &entities, double delta)
{
    static double elapsed{};
    elapsed += delta;
    if (elapsed < _interval)
        return Task{};
    else
        elapsed -= _interval;

    std::vector<Event> events;

    std::vector<Entity> bases;
    std::vector<Entity> deposits;
    std::vector<Entity> agents;
    std::vector<Entity> bullets;

    for (auto &[id, entity] : entities)
    {
        if (entity.has(ComponentType{ "BASE" }))
            bases.push_back(entity);
        else if (entity.has(ComponentType{ "DEPOSIT" }))
            bases.push_back(entity);
        else if (entity.has(ComponentType{ "AGENT" }))
            bases.push_back(entity);
        else if (entity.has(ComponentType{ "BULLET" }))
            bases.push_back(entity);
    }

    for (auto &base : bases)
    {
        auto b = getObject<BaseComponent>(base[ComponentType{ "BASE" }]);

        b->metal++;

        if (_positions.count(base.getID()))
        {
            events.emplace_back(
                base.getID(),
                game::event::type::TEXT,
                std::make_shared<game::event::data::TEXT>(std::to_string(b->metal), _positions[base.getID()], glm::u8vec3{ 255, 0, 0 }, 30));
        }
    }

	pushEvents(events, StreamType::OUTGOING);

	_positions.clear();
    _collisions.clear();

    return Task{};
}

ComponentHandle GameSystem::createComponent(ComponentType type, std::shared_ptr<void> tuplePtr)
{
    Handle h{};

    if (type == "BASE")
    {
        BaseArgs args = *(std::static_pointer_cast<BaseArgs>(tuplePtr));

        h = emplaceObject<BaseComponent>(std::get<0>(args), std::get<1>(args));
    }
    else if (type == "DEPOSIT")
    {
    }
    else if (type == "AGENT")
    {
    }
    else if (type == "BULLET")
    {
    }

    return ComponentHandle{ type, h };
}

bool GameSystem::removeComponent(ComponentHandle ch)
{
    if (ch.first == "BASE")
    {
        removeObject<BaseComponent>(ch.second);
    }
    else if (ch.first == "DEPOSIT")
    {
    }
    else if (ch.first == "AGENT")
    {
    }
    else if (ch.first == "BULLET")
    {
    }
    else
        return false;

    return true;
}
