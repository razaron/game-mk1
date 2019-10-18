#include "GameSystem.hpp"

using namespace rz::core;
using namespace rz::planner;
using namespace rz::taskscheduler;
using namespace rz::game::systems;
using namespace rz::eventstream;
using namespace rz::game::components;

GameSystem::GameSystem(sol::state_view lua)
    : _lua(lua)
{
    _interval = 0.1;

    // Valid ComponentTypes
    _componentTypes.insert(ComponentType{ "BASE" });
    _componentTypes.insert(ComponentType{ "DEPOSIT" });
    _componentTypes.insert(ComponentType{ "AGENT" });
    _componentTypes.insert(ComponentType{ "WORKER" });
    _componentTypes.insert(ComponentType{ "SOLDIER" });
    _componentTypes.insert(ComponentType{ "BULLET" });

    // EVENT HANDLERS
    registerHandler(game::event::type::MODEL, [&](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::MODEL>(e.data);

        _positions[e.recipient] = glm::vec2{ data->model[3] };
    });

    registerHandler(game::event::type::COLLISION, [&](const Event &e) {
        auto data = std::static_pointer_cast<game::event::data::COLLISION>(e.data);

        auto &vec = _collisions[e.recipient];

        for (auto &col : vec)
        {
            if (col.target == data->target)
            {
                col = *data;
                return;
            }
        }

        vec.push_back(*data);
    });

    registerHandler(core::event::type::SPACE_DELETE_ENTITY, [&](const Event &e) {
        _positions.erase(e.recipient);
        _collisions.erase(e.recipient);
    });

    // Lua hooks
    _lua["TEAM"] = sol::new_table();
    _lua["TEAM"]["RED"] = Team::RED;
    _lua["TEAM"]["GREEN"] = Team::GREEN;
    _lua["TEAM"]["BLUE"] = Team::BLUE;
    _lua["TEAM"]["YELLOW"] = Team::YELLOW;
    _lua["TEAM"]["NONE"] = Team::NONE;

    _lua["newBase"] = [this](Team team, glm::vec2 pos, glm::u8vec3 col) {
        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 12.8f }, -3.14159 / 4) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(4, col) },
                ComponentArgs{ ComponentType{ "BASE" }, std::make_shared<BaseArgs>(team, 10) },
            })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    _lua["newDeposit"] = [this](Team team, glm::vec2 pos, glm::u8vec3 col) {
        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 6.4f }, 0.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(4, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(8.f, GROUP_DEPOSIT) },
                ComponentArgs{ ComponentType{ "DEPOSIT" }, std::make_shared<DepositArgs>(team, 10) } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    _lua["newWorker"] = [&](glm::vec2 pos, int sides, glm::u8vec3 col, Team team) {
        Action mine = _lua["Actions"]["mine"]();

        Action craft = _lua["Actions"]["craft"]();

        ActionSet actions{ mine, craft };

        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 3.2f }, 0.f) },
                ComponentArgs{ ComponentType{ "MOTION" }, std::make_shared<MotionArgs>(glm::vec2{}, glm::vec2{}, 64.f, 64.f, 1.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(sides, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(128.f, static_cast<int>(team)) },
                ComponentArgs{ ComponentType{ "AGENT" }, std::make_shared<AgentArgs>("WORKER", team, actions) },
                ComponentArgs{ ComponentType{ "WORKER" }, {} } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    EffectFunction mineTarget = _lua["Effects"]["mine"]["start"];
    EffectFunction mineEffect = _lua["Effects"]["mine"]["run"];

    EffectFunction craftTarget = _lua["Effects"]["craft"]["start"];
    EffectFunction craftEffect = _lua["Effects"]["craft"]["run"];

    _effects["mine"] = std::make_tuple(mineTarget, SteeringBehaviour::ARRIVE, mineEffect);
    _effects["craft"] = std::make_tuple(craftTarget, SteeringBehaviour::ARRIVE, craftEffect);

    _lua["newBullet"] = [&](glm::vec2 pos, glm::u8vec3 col, glm::vec2 dir, UUID64 owner, Team team) {
        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 0.8f }, 0.f) },
                ComponentArgs{ ComponentType{ "MOTION" }, std::make_shared<MotionArgs>(dir * 512.f, glm::vec2{}, 32.f, 32.f, 1.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(16, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(8.f, GROUP_BULLET) },
                ComponentArgs{ ComponentType{ "BULLET" }, std::make_shared<BulletArgs>(owner, team, pos) } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    _lua["newAttacker"] = [&](glm::vec2 pos, int sides, glm::u8vec3 col, Team team) {
        Action findTarget = _lua["Actions"]["findTarget"]();
        Action getInRange = _lua["Actions"]["getInRange"]();
        Action shoot = _lua["Actions"]["shoot"]();
        Action melee = _lua["Actions"]["melee"]();
        Action scout = _lua["Actions"]["scout"]();
        Action resupply = _lua["Actions"]["resupply"]();
        Action capture = _lua["Actions"]["capture"]();

        ActionSet actions{ findTarget, getInRange, shoot, melee, resupply, scout, capture };

        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 3.2f }, 0.f) },
                ComponentArgs{ ComponentType{ "MOTION" }, std::make_shared<MotionArgs>(glm::vec2{}, glm::vec2{}, 64.f, 64.f, 1.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(sides, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(128.f, static_cast<unsigned>(team)) },
                ComponentArgs{ ComponentType{ "AGENT" }, std::make_shared<AgentArgs>("ATTACKER", team, actions) },
                ComponentArgs{ ComponentType{ "SOLDIER" }, {} } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    _lua["newDefender"] = [this](glm::vec2 pos, int sides, glm::u8vec3 col, Team team) {
        
        Action findTarget = _lua["Actions"]["findTarget"]();
        Action getInRange = _lua["Actions"]["getInRange"]();
        Action shoot = _lua["Actions"]["shoot"]();
        Action melee = _lua["Actions"]["melee"]();
        Action patrol = _lua["Actions"]["patrol"]();

        ActionSet actions{ findTarget, getInRange, shoot, melee, patrol };

        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 3.2f }, 0.f) },
                ComponentArgs{ ComponentType{ "MOTION" }, std::make_shared<MotionArgs>(glm::vec2{}, glm::vec2{}, 64.f, 64.f, 1.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(sides, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(128.f, static_cast<unsigned>(team)) },
                ComponentArgs{ ComponentType{ "AGENT" }, std::make_shared<AgentArgs>("DEFENDER", team, actions) },
                ComponentArgs{ ComponentType{ "SOLDIER" }, {} } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    // TODO integrate Lua "resupplying" and "serving" (ctrl-f in old actions.lua)
    // TODO Add seperate blackboard for AgentComponent, WorkerComponent etc.
    EffectFunction findTargetTarget = _lua["Effects"]["findTarget"]["start"];
    EffectFunction findTargetEffect = _lua["Effects"]["findTarget"]["run"];
    EffectFunction getInRangeTarget = _lua["Effects"]["getInRange"]["start"];
    EffectFunction getInRangeEffect = _lua["Effects"]["getInRange"]["run"];
    EffectFunction shootTarget = _lua["Effects"]["shoot"]["start"];
    EffectFunction shootEffect = _lua["Effects"]["shoot"]["run"];
    EffectFunction meleeTarget = _lua["Effects"]["melee"]["start"];
    EffectFunction meleeEffect = _lua["Effects"]["melee"]["run"];
    EffectFunction scoutTarget = _lua["Effects"]["scout"]["start"];
    EffectFunction scoutEffect = _lua["Effects"]["scout"]["run"];
    EffectFunction resupplyTarget = _lua["Effects"]["resupply"]["start"];
    EffectFunction resupplyEffect = _lua["Effects"]["resupply"]["run"];
    EffectFunction captureTarget = _lua["Effects"]["capture"]["start"];
    EffectFunction captureEffect = _lua["Effects"]["capture"]["run"];
    EffectFunction patrolTarget = _lua["Effects"]["patrol"]["start"];
    EffectFunction patrolEffect = _lua["Effects"]["patrol"]["run"];

    _effects["findTarget"] = std::make_tuple(findTargetTarget, SteeringBehaviour::STOP, findTargetEffect);
    _effects["getInRange"] = std::make_tuple(getInRangeTarget, SteeringBehaviour::SEEK, getInRangeEffect);
    _effects["shoot"] = std::make_tuple(shootTarget, SteeringBehaviour::MAINTAIN, shootEffect);
    _effects["melee"] = std::make_tuple(meleeTarget, SteeringBehaviour::SEEK, meleeEffect);
    _effects["resupply"] = std::make_tuple(resupplyTarget, SteeringBehaviour::ARRIVE, resupplyEffect);
    _effects["scout"] = std::make_tuple(scoutTarget, SteeringBehaviour::WANDER, scoutEffect);
    _effects["capture"] = std::make_tuple(captureTarget, SteeringBehaviour::ARRIVE, captureEffect);
    _effects["patrol"] = std::make_tuple(patrolTarget, SteeringBehaviour::ARRIVE, patrolEffect);

    _lua["deleteEntity"] = [&](const UUID64 &id) {
        Event e{
            id,                                     // Entity ID.
            core::event::type::SPACE_DELETE_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_DELETE_ENTITY>()
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    _lua["getPosition"] = [&](const Entity &entity){ 
        if (_positions.count(entity.getID()))
            return _positions[entity.getID()];
        else
            return glm::vec2{};
    };

    _lua.new_usertype<BaseComponent>("BaseComponent",
                                    sol::constructors<BaseComponent()>(),
                                    "team", &BaseComponent::team,
                                    "metal", &BaseComponent::metal,
                                    "serving", &BaseComponent::serving);

    _lua["getBase"] = [&](const Handle &handle){
        return getObject<BaseComponent>(handle);
    };

    _lua.new_usertype<DepositComponent>("DepositComponent",
                                    sol::constructors<DepositComponent()>(),
                                    "team", &DepositComponent::team,
                                    "metal", &DepositComponent::metal,
                                    "control", &DepositComponent::control,
                                    "serving", &DepositComponent::serving);

    _lua["getDeposit"] = [&](const Handle &handle){
        return getObject<DepositComponent>(handle);
    };

    _lua.new_usertype<AgentComponent::Blackboard>("AgentBlackboard",
                                                sol::constructors<AgentComponent::Blackboard>(),
                                                "threat", &AgentComponent::Blackboard::threat                                                    
                                                );

    _lua.new_usertype<AgentComponent>("AgentComponent",
                                    sol::constructors<AgentComponent()>(),
                                    "name", &AgentComponent::name,
                                    "team", &AgentComponent::team,
                                    "blackboard", &AgentComponent::blackboard,
                                    "target", &AgentComponent::target,
                                    "isDead", &AgentComponent::isDead
                                    );

    _lua["getAgent"] = [&](const Handle &handle){
        return getObject<AgentComponent>(handle);
    };

    _lua.new_usertype<WorkerComponent>("WorkerComponent",
                                    sol::constructors<WorkerComponent()>(),
                                    "metal", &WorkerComponent::metal,
                                    "elapsed", &WorkerComponent::elapsed
                                    );

    _lua["getWorker"] = [&](const Handle &handle){
        return getObject<WorkerComponent>(handle);
    };

    _lua.new_usertype<SoldierComponent>("SoldierComponent",
                                    sol::constructors<SoldierComponent()>(),
                                    "ammo", &SoldierComponent::ammo,
                                    "elapsed", &SoldierComponent::elapsed
                                    );

    _lua["getSoldier"] = [&](const Handle &handle){
        return getObject<SoldierComponent>(handle);
    };

    _lua["killAgent"] = [&](const Entity &e){ killAgent(e); };

    // Load actions script
    auto result = _lua.script_file("actions.lua", [](lua_State *, sol::protected_function_result pfr) {
        sol::error err = pfr;
        std::cerr << err.what() << std::endl;
        return pfr;
    });

    // Load game script
    result = _lua.script_file("game.lua", [](lua_State *, sol::protected_function_result pfr) {
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
    static double elapsed{ 0.0 };
    static double gameTime{ 0.0 };
    gameTime += delta;

    if ((elapsed += delta) < _interval)
        return {};
    else
        elapsed -= _interval;

    if (!entities.size() && gameTime > 1.0)
    {
        sol::function func = _lua["game"]["init"];
        func();

        gameTime = 0;
    }

    _entities = entities;

    std::vector<Event> events;

    std::vector<Entity> bases;
    std::vector<Entity> deposits;
    std::vector<Entity> agents;
    std::vector<Entity> bullets;

    for (auto &[id, entity] : entities)
    {
        if (!_positions.count(entity.getID()))
            continue;
        if (entity.has(ComponentType{ "BASE" }))
            bases.push_back(entity);
        else if (entity.has(ComponentType{ "DEPOSIT" }))
            deposits.push_back(entity);
        else if (entity.has(ComponentType{ "AGENT" }))
            agents.push_back(entity);
        else if (entity.has(ComponentType{ "BULLET" }))
            bullets.push_back(entity);
    }

    for (auto &base : bases)
    {
        auto b = getObject<BaseComponent>(base[ComponentType{ "BASE" }]);

        std::vector<Entity> workers;
        std::vector<Entity> attackers;
        std::vector<Entity> defenders;
        for (auto &agent : agents)
        {
            auto a = getObject<AgentComponent>(agent[ComponentType{ "AGENT" }]);

            if (a->team != b->team) continue;

            if (agent.has(ComponentType{ "WORKER" }))
                workers.push_back(agent);
            else if (agent.has(ComponentType{ "SOLDIER" }))
            {
                if(a->actions.size() == 7)
                    attackers.push_back(agent);
                else if(a->actions.size() == 5)
                    defenders.push_back(agent);
            }
        }

        glm::u8vec3 col{ 50, 50, 50 };
        if (b->team == Team::RED)
            col.r = 255;
        else if (b->team == Team::GREEN)
            col.g = 255;
        else if (b->team == Team::BLUE)
            col.b = 255;
        else if (b->team == Team::YELLOW)
            col = { 255, 255, 0 };

        // build stuff
        if (b->metal >= 5)
        {
            if (workers.size() <= attackers.size())
            {
                b->metal -= 2;
                sol::function build = _lua["newWorker"];
                build(_positions[base.getID()], 16, col, static_cast<unsigned>(b->team));
            }
            else if(attackers.size() <= defenders.size())
            {
                b->metal -= 5;
                sol::function build = _lua["newAttacker"];
                build(_positions[base.getID()], 3, col, static_cast<unsigned>(b->team));
            }
            else
            {
                b->metal -= 5;
                sol::function build = _lua["newDefender"];
                build(_positions[base.getID()], 4, col, static_cast<unsigned>(b->team));
            }
        }

        events.emplace_back(
            base.getID(),
            game::event::type::TEXT,
            std::make_shared<game::event::data::TEXT>(std::to_string(b->metal), _positions[base.getID()], glm::u8vec3{ 255, 0, 0 }, 30));
    }

    for (auto &deposit : deposits)
    {
        auto d = getObject<DepositComponent>(deposit[ComponentType{ "DEPOSIT" }]);

        if ((d->elapsed += _interval) > 1.0)
        {
            d->metal++;
            d->elapsed = 0;
        }

        int red{}, green{}, blue{}, yellow{};
        auto collisions = _collisions[deposit.getID()];
        for (auto &collision : collisions)
        {
            if (collision.group == GROUP_RED)
                red++;
            else if (collision.group == GROUP_GREEN)
                green++;
            else if (collision.group == GROUP_BLUE)
                blue++;
            else if (collision.group == GROUP_YELLOW)
                yellow++;
        }

        int max{};
        Team leader{ Team::NONE };
        if (red > max)
        {
            max = red;
            leader = Team::RED;
        }
        if (green > max)
        {
            max = green;
            leader = Team::GREEN;
        }
        if (blue > max)
        {
            max = blue;
            leader = Team::BLUE;
        }
        if (yellow > max)
        {
            max = yellow;
            leader = Team::YELLOW;
        }

        if (leader == Team::RED)
        {
            float average = (green + blue + yellow) / 3;
            if (d->control[static_cast<int>(Team::GREEN)] || d->control[static_cast<int>(Team::BLUE)] || d->control[static_cast<int>(Team::YELLOW)])
            {
                d->control[static_cast<int>(Team::GREEN)] -= (red - green) * _interval / 10;
                d->control[static_cast<int>(Team::BLUE)] -= (red - blue) * _interval / 10;
                d->control[static_cast<int>(Team::YELLOW)] -= (red - yellow) * _interval / 10;
            }
            else
                d->control[static_cast<int>(Team::RED)] += (red - average) * _interval / 10;
        }
        else if (leader == Team::GREEN)
        {
            float average = (red + blue + yellow) / 3;
            if (d->control[static_cast<int>(Team::RED)] || d->control[static_cast<int>(Team::BLUE)] || d->control[static_cast<int>(Team::YELLOW)])
            {
                d->control[static_cast<int>(Team::RED)] -= (green - red) * _interval / 10;
                d->control[static_cast<int>(Team::BLUE)] -= (green - blue) * _interval / 10;
                d->control[static_cast<int>(Team::YELLOW)] -= (green - yellow) * _interval / 10;
            }
            else
                d->control[static_cast<int>(Team::GREEN)] += (green - average) * _interval / 10;
        }
        else if (leader == Team::BLUE)
        {
            float average = (green + red + yellow) / 3;
            if (d->control[static_cast<int>(Team::GREEN)] || d->control[static_cast<int>(Team::RED)] || d->control[static_cast<int>(Team::YELLOW)])
            {
                d->control[static_cast<int>(Team::GREEN)] -= (blue - green) * _interval / 10;
                d->control[static_cast<int>(Team::RED)] -= (blue - red) * _interval / 10;
                d->control[static_cast<int>(Team::YELLOW)] -= (blue - yellow) * _interval / 10;
            }
            else
                d->control[static_cast<int>(Team::BLUE)] += (blue - average) * _interval / 10;
        }
        else if (leader == Team::YELLOW)
        {
            float average = (green + blue + red) / 3;
            if (d->control[static_cast<int>(Team::GREEN)] || d->control[static_cast<int>(Team::BLUE)] || d->control[static_cast<int>(Team::RED)])
            {
                d->control[static_cast<int>(Team::GREEN)] -= (yellow - green) * _interval / 10;
                d->control[static_cast<int>(Team::BLUE)] -= (yellow - blue) * _interval / 10;
                d->control[static_cast<int>(Team::RED)] -= (yellow - red) * _interval / 10;
            }
            else
                d->control[static_cast<int>(Team::YELLOW)] += (yellow - average) * _interval / 10;
        }

        if (d->control[static_cast<int>(Team::RED)] < 0.f) d->control[static_cast<int>(Team::RED)] = 0.f;
        if (d->control[static_cast<int>(Team::GREEN)] < 0.f) d->control[static_cast<int>(Team::GREEN)] = 0.f;
        if (d->control[static_cast<int>(Team::BLUE)] < 0.f) d->control[static_cast<int>(Team::BLUE)] = 0.f;
        if (d->control[static_cast<int>(Team::YELLOW)] < 0.f) d->control[static_cast<int>(Team::YELLOW)] = 0.f;

        if (d->control[static_cast<int>(Team::RED)] >= 1.f)
        {
            d->control[static_cast<int>(Team::RED)] = 1.f;
            d->team = Team::RED;

            events.emplace_back(
                deposit.getID(),
                game::event::type::COLOUR,
                std::make_shared<game::event::data::COLOUR>(glm::u8vec3{ 255, 100, 100 }));
        }
        else if (d->control[static_cast<int>(Team::GREEN)] >= 1.f)
        {
            d->control[static_cast<int>(Team::GREEN)] = 1.f;
            d->team = Team::GREEN;

            events.emplace_back(
                deposit.getID(),
                game::event::type::COLOUR,
                std::make_shared<game::event::data::COLOUR>(glm::u8vec3{ 100, 255, 100 }));
        }
        else if (d->control[static_cast<int>(Team::BLUE)] >= 1.f)
        {
            d->control[static_cast<int>(Team::BLUE)] = 1.f;
            d->team = Team::BLUE;

            events.emplace_back(
                deposit.getID(),
                game::event::type::COLOUR,
                std::make_shared<game::event::data::COLOUR>(glm::u8vec3{ 100, 100, 255 }));
        }
        else if (d->control[static_cast<int>(Team::YELLOW)] >= 1.f)
        {
            d->control[static_cast<int>(Team::YELLOW)] = 1.f;
            d->team = Team::YELLOW;

            events.emplace_back(
                deposit.getID(),
                game::event::type::COLOUR,
                std::make_shared<game::event::data::COLOUR>(glm::u8vec3{ 255, 255, 100 }));
        }
        else
        {
            d->team = Team::NONE;

            events.emplace_back(
                deposit.getID(),
                game::event::type::COLOUR,
                std::make_shared<game::event::data::COLOUR>(glm::u8vec3{ 100, 100, 100 }));
        }

        std::stringstream str;
        str << d->metal << std::endl
            << d->serving;

        events.emplace_back(
            deposit.getID(),
            game::event::type::TEXT,
            std::make_shared<game::event::data::TEXT>(str.str(), _positions[deposit.getID()], glm::u8vec3{ 255, 0, 0 }, 30));
    }

    for (auto &agent : agents)
    {
        auto a = getObject<AgentComponent>(agent[ComponentType{ "AGENT" }]);

        // SENSING AND GENERAL UPDATES
        // AGENT
        a->blackboard.threat = 0;
        auto collisions = _collisions[agent.getID()];
        for (auto &collision : collisions)
        {
            if (collision.group != static_cast<int>(a->team) && collision.group != GROUP_DEPOSIT && collision.group != GROUP_BULLET)
            {
                a->blackboard.threat++;
            }
        }

        // WORKER
        if (agent.has(ComponentType{ "WORKER" }))
        {
            auto w = getObject<WorkerComponent>(agent[ComponentType{ "WORKER" }]);

            w->elapsed += _interval;
        }
        // SOLDIER
        else if (agent.has(ComponentType{ "SOLDIER" }))
        {
            auto s = getObject<SoldierComponent>(agent[ComponentType{ "SOLDIER" }]);

            s->elapsed += _interval;
        }

        if (!a->curPlan.size())
        {

            bool dep = false;
            for (auto &deposit : deposits)
            {
                auto d = getObject<DepositComponent>(deposit[ComponentType{ "DEPOSIT" }]);
                if (d->team == a->team)
                {
                    dep = true;
                    break;
                }
            }

            ConditionSet worldState;
            Action goal;

            // AGENT ALL

            // WORKER
            if (agent.has(ComponentType{ "WORKER" }))
            {
                auto w = getObject<WorkerComponent>(agent[ComponentType{ "WORKER" }]);

                worldState = ConditionSet{
                    { "self", "threat", a->blackboard.threat },
                    { "self", "hasResource", (w->metal > 0) ? true : false },
                    { "team", "hasDeposit", dep }
                };

                if (a->blackboard.threat)
                    goal = Action{ "GOAL", 0, ConditionSet{ { "self", "threat", a->blackboard.threat, Operation::LESS } }, {} };
                else
                    goal = Action{ "GOAL", 0, ConditionSet{ { "self", "craft", true, Operation::EQUAL } }, {} };
            }
            // SOLDIER
            else if (agent.has(ComponentType{ "SOLDIER" }))
            {
                auto s = getObject<SoldierComponent>(agent[ComponentType{ "SOLDIER" }]);

                bool hasAmmo = false;
                for (auto &base : bases)
                {
                    auto b = getObject<BaseComponent>(base[ComponentType{ "BASE" }]);
                    if (b->team == a->team && b->metal /* - b->serving */ > 0)
                    {
                        hasAmmo = true;
                        break;
                    }
                }

                bool hasDeposit = false;
                for (auto &depo : deposits)
                {
                    auto d = getObject<DepositComponent>(depo[ComponentType{ "DEPOSIT" }]);
                    if (d->team != a->team)
                    {
                        hasDeposit = true;
                        break;
                    }
                }

                worldState = ConditionSet{
                    { "self", "threat", a->blackboard.threat },
                    { "self", "ammo", s->ammo },
                    { "base", "hasAmmo", hasAmmo },
                    { "world", "hasDeposit", hasDeposit }
                };

                if(a->name == "ATTACKER")
                {
                    if (a->blackboard.threat)
                        goal = Action{ "GOAL", 0, ConditionSet{ { "self", "threat", a->blackboard.threat, Operation::LESS } }, {} };
                    else if (s->ammo < 3 && hasAmmo)
                        goal = Action{ "GOAL", 0, ConditionSet{ { "self", "ammo", s->ammo, Operation::GREATER } }, {} };
                    else if (hasDeposit)
                        goal = Action{ "GOAL", 0, ConditionSet{ { "team", "hasDeposit", true, Operation::EQUAL } }, {} };
                    else
                        goal = Action{ "GOAL", 0, ConditionSet{ { "self", "isScouting", true, Operation::EQUAL } }, {} };
                }
                else if(a->name == "DEFENDER")
                {
                    if (a->blackboard.threat)
                        goal = Action{ "GOAL", 0, ConditionSet{ { "self", "threat", a->blackboard.threat, Operation::LESS } }, {} };
                    else if (s->ammo < 3 && hasAmmo)
                        goal = Action{ "GOAL", 0, ConditionSet{ { "self", "ammo", s->ammo, Operation::GREATER } }, {} };
                    else
                        goal = Action{ "GOAL", 0, ConditionSet{ { "self", "isPatrolling", true, Operation::EQUAL } }, {} };
                }
            }

            a->planner.setWorldState(worldState);
            if ((a->curPlan = a->planner.plan(a->actions, goal)) != ActionSet{})
            {
                a->curAction = *a->curPlan.begin();

                if (!std::get<0>(_effects[a->curAction.name])(entities, agent))
                {
                    a->curAction = {};
                    a->curPlan = {};
                    a->planner.savePlan(std::to_string(agent.getID().uuid) + ".dot");
                }
                else
                {
                    events.emplace_back(
                        agent.getID(),
                        game::event::type::STEERING,
                        std::make_shared<game::event::data::STEERING>(a->target.getID(), std::get<1>(_effects[a->curAction.name])));
                };
            }
        }

        if (a->curPlan.size())
        {
            auto effect = std::get<2>(_effects[a->curAction.name]);
            if (effect(entities, agent))
            {
                auto it = std::find(a->curPlan.begin(), a->curPlan.end(), a->curAction);
                it++;

                if (it != a->curPlan.end())
                {
                    a->curAction = *it;

                    if (!std::get<0>(_effects[a->curAction.name])(entities, agent))
                    {
                        a->curAction = {};
                        a->curPlan = {};
                    }
                    else
                    {
                        events.emplace_back(
                            agent.getID(),
                            game::event::type::STEERING,
                            std::make_shared<game::event::data::STEERING>(a->target.getID(), std::get<1>(_effects[a->curAction.name])));
                    }
                }
                else
                {
                    a->curPlan = ActionSet{};
                    a->curAction = Action{};
                }
            }
        }
    }

    for (auto &bullet : bullets)
    {
        auto b = getObject<BulletComponent>(bullet[ComponentType{ "BULLET" }]);

        if (glm::distance(_positions[bullet.getID()], b->origin) > 64)
        {

            sol::function func = _lua["deleteEntity"];
            func(bullet.getID());

            continue;
        }

        UUID64 uuid;
        int dist = std::numeric_limits<int>::max();

        if (_collisions.count(bullet.getID()))
        {
            auto collisions = _collisions[bullet.getID()];

            for (auto &collision : collisions)
            {
                if (!b->isDead && collision.group != static_cast<int>(b->team) && collision.group != GROUP_DEPOSIT && collision.group != GROUP_BULLET)
                {
                    uuid = collision.target;
                    dist = collision.distance;
                }
            }

            if (dist != std::numeric_limits<int>::max())
            {
                killAgent(entities[uuid]);

                sol::function func = _lua["deleteEntity"];
                func(bullet.getID());

                b->isDead = true;
            }
        }
    }

    // GUI TEXT HACKS (hack because uuid)
    std::vector<Entity> red;
    std::vector<Entity> green;
    std::vector<Entity> blue;
    std::vector<Entity> yellow;

    for (auto &[id, entity] : entities)
    {
        if (entity.has(ComponentType{ "AGENT" }))
        {
            auto a = getObject<AgentComponent>(entity[ComponentType{ "AGENT" }]);

            if (a->isDead)
                continue;
            else if (a->team == Team::RED)
                red.push_back(entity);
            else if (a->team == Team::GREEN)
                green.push_back(entity);
            else if (a->team == Team::BLUE)
                blue.push_back(entity);
            else if (a->team == Team::YELLOW)
                yellow.push_back(entity);
        }
    }

    std::stringstream str;
    str << _wins[Team::RED] << "\t\t\t\t\t\t\t\t" << red.size();

    events.emplace_back(
        UUID64{ 0 },
        game::event::type::TEXT,
        std::make_shared<game::event::data::TEXT>(std::string{str.str()}, glm::vec2{ 0, 0 }, glm::u8vec3{ 255, 0, 0 }, 30));

    str = std::stringstream{};
    str << _wins[Team::GREEN] << "\t\t\t\t\t\t\t\t" << green.size();

    events.emplace_back(
        UUID64{ 1 },
        game::event::type::TEXT,
        std::make_shared<game::event::data::TEXT>(std::string{str.str()}, glm::vec2{ 0, 32 }, glm::u8vec3{ 0, 255, 0 }, 30));

    str = std::stringstream{};
    str << _wins[Team::BLUE] << "\t\t\t\t\t\t\t\t" << blue.size();

    events.emplace_back(
        UUID64{ 2 },
        game::event::type::TEXT,
        std::make_shared<game::event::data::TEXT>(std::string{str.str()}, glm::vec2{ 0, 64 }, glm::u8vec3{ 0, 0, 255 }, 30));

    str = std::stringstream{};
    str << _wins[Team::YELLOW] << "\t\t\t\t\t\t\t\t" << yellow.size();

    events.emplace_back(
        UUID64{ 3 },
        game::event::type::TEXT,
        std::make_shared<game::event::data::TEXT>(std::string{str.str()}, glm::vec2{ 0, 96 }, glm::u8vec3{ 255, 255, 0 }, 30));

    pushEvents(events, StreamType::OUTGOING);

    return Task{};
}

void GameSystem::killAgent(const Entity &entity)
{
    sol::function func = _lua["deleteEntity"];
    func(entity.getID());

    auto a = getObject<AgentComponent>(entity[ComponentType{ "AGENT" }]);
    a->isDead = true;

    if (checkGameOver())
    {
        std::vector<Event> events;
        for (auto &[id, entity] : _entities)
        {
            events.emplace_back(
                id,
                core::event::type::SPACE_DELETE_ENTITY,
                std::make_shared<core::event::data::SPACE_DELETE_ENTITY>());
        }

        for (auto &[id, entity] : _entities)
        {
            if (entity.has(ComponentType{ "AGENT" }))
            {
                auto a = getObject<AgentComponent>(entity[ComponentType{ "AGENT" }]);

                a->isDead = true;
            }

            if (entity.has(ComponentType{ "BULLET" }))
            {
                auto b = getObject<BulletComponent>(entity[ComponentType{ "BULLET" }]);

                b->isDead = true;
            }
        }

        pushEvents(events, StreamType::OUTGOING);
    }
}

bool GameSystem::checkGameOver()
{
    std::vector<Entity> red;
    std::vector<Entity> green;
    std::vector<Entity> blue;
    std::vector<Entity> yellow;

    for (auto &[id, entity] : _entities)
    {
        if (entity.has(ComponentType{ "AGENT" }))
        {
            auto a = getObject<AgentComponent>(entity[ComponentType{ "AGENT" }]);

            if (a->isDead || a->name == "DEFENDER")
                continue;
            else if (a->team == Team::RED)
                red.push_back(entity);
            else if (a->team == Team::GREEN)
                green.push_back(entity);
            else if (a->team == Team::BLUE)
                blue.push_back(entity);
            else if (a->team == Team::YELLOW)
                yellow.push_back(entity);
        }
    }

    bool isGameOver{ false };
    if (red.size() && !green.size() && !blue.size() && !yellow.size())
    {
        _wins[Team::RED]++;
        isGameOver = true;
    }
    else if (!red.size() && green.size() && !blue.size() && !yellow.size())
    {
        _wins[Team::GREEN]++;
        isGameOver = true;
    }
    else if (!red.size() && !green.size() && blue.size() && !yellow.size())
    {
        _wins[Team::BLUE]++;
        isGameOver = true;
    }
    else if (!red.size() && !green.size() && !blue.size() && yellow.size())
    {
        _wins[Team::YELLOW]++;
        isGameOver = true;
    }

    return isGameOver;
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
        DepositArgs args = *(std::static_pointer_cast<DepositArgs>(tuplePtr));

        h = emplaceObject<DepositComponent>(std::get<0>(args), std::get<1>(args));
    }
    else if (type == "AGENT")
    {
        AgentArgs args = *(std::static_pointer_cast<AgentArgs>(tuplePtr));

        h = emplaceObject<AgentComponent>(std::get<0>(args), std::get<1>(args), std::get<2>(args));
    }
    else if (type == "WORKER")
    {
        WorkerArgs args = *(std::static_pointer_cast<WorkerArgs>(tuplePtr));

        h = emplaceObject<WorkerComponent>();
    }
    else if (type == "SOLDIER")
    {
        SoldierArgs args = *(std::static_pointer_cast<SoldierArgs>(tuplePtr));

        h = emplaceObject<SoldierComponent>();
    }
    else if (type == "BULLET")
    {
        // TODO `using BulletComponent::args = std::tuple<...>`. Move ComponentArgs alias to Component classes
        BulletArgs args = *(std::static_pointer_cast<BulletArgs>(tuplePtr));

        h = emplaceObject<BulletComponent>(std::get<0>(args), std::get<1>(args), std::get<2>(args));
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
        removeObject<DepositComponent>(ch.second);
    }
    else if (ch.first == "AGENT")
    {
        removeObject<AgentComponent>(ch.second);
    }
    else if (ch.first == "WORKER")
    {
        removeObject<WorkerComponent>(ch.second);
    }
    else if (ch.first == "SOLDIER")
    {
        removeObject<SoldierComponent>(ch.second);
    }
    else if (ch.first == "BULLET")
    {
        removeObject<BulletComponent>(ch.second);
    }
    else
        return false;

    return true;
 }
