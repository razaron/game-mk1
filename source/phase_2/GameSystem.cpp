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
    lua["TEAM"] = sol::new_table();
    lua["TEAM"]["RED"] = Team::RED;
    lua["TEAM"]["GREEN"] = Team::GREEN;
    lua["TEAM"]["BLUE"] = Team::BLUE;
    lua["TEAM"]["YELLOW"] = Team::YELLOW;
    lua["TEAM"]["NONE"] = Team::NONE;

    lua["newBase"] = [this](Team team, glm::vec2 pos, glm::u8vec3 col) {
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

    lua["newDeposit"] = [this](Team team, glm::vec2 pos, glm::u8vec3 col) {
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

    lua["newWorker"] = [this](glm::vec2 pos, int sides, glm::u8vec3 col, Team team) {
        Action mine{
            "mine",
            1,
            ConditionSet{ Condition{ "team", "hasDeposit", true, Operation::EQUAL } },
            ConditionSet{ Condition{ "self", "hasResource", true, Operation::ASSIGN } }
        };

        Action craft{
            "craft",
            1,
            ConditionSet{ Condition{ "self", "hasResource", true, Operation::EQUAL } },
            ConditionSet{ Condition{ "self", "craft", true, Operation::ASSIGN } }
        };

        ActionSet actions{ mine, craft };

        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 3.2f }, 0.f) },
                ComponentArgs{ ComponentType{ "MOTION" }, std::make_shared<MotionArgs>(glm::vec2{}, glm::vec2{}, 64.f, 64.f, 1.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(sides, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(128.f, static_cast<int>(team)) },
                ComponentArgs{ ComponentType{ "AGENT" }, std::make_shared<AgentArgs>(team, actions) },
                ComponentArgs{ ComponentType{ "WORKER" }, {} } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    EffectFunction mineEffect = [&](const EntityMap &entities, const Entity &self) {
        auto w = getObject<WorkerComponent>(self[ComponentType{ "WORKER" }]);
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto d = getObject<DepositComponent>(a->target[ComponentType{ "DEPOSIT" }]);

        if (a->blackboard.threat)
        {
            d->serving--;
            return true;
        }
        else if (a->team != d->team)
        {
            d->serving--;
            return true;
        }
        else if (!d->metal)
        {
            d->serving--;
            return true;
        }
        else if (w->elapsed > 0.1 && glm::distance(_positions[self.getID()], _positions[a->target.getID()]) < 8)
        {
            w->elapsed = 0;

            d->metal--;
            w->metal++;

            if (!d->metal || w->metal == 5)
            {
                d->serving--;
                return true;
            }
            else
                return false;
        }

        return false;
    };

    EffectFunction mineTarget = [&](const EntityMap &entities, const Entity &self) {
        auto w = getObject<WorkerComponent>(self[ComponentType{ "WORKER" }]);
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);

        std::vector<Entity> deposits;
        std::vector<Entity> bases;
        for (auto &[id, entity] : entities)
        {
            if (entity.has(ComponentType{ "DEPOSIT" }))
                deposits.push_back(entity);
            else if (entity.has(ComponentType{ "BASE" }))
                bases.push_back(entity);
        }

        Entity base{};
        for (auto &entity : bases)
        {
            auto b = getObject<BaseComponent>(entity[ComponentType{ "BASE" }]);

            if (b->team == a->team)
                base = entity;
        }

        Entity target{};
        for (auto &dep : deposits)
        {
            auto depD = getObject<DepositComponent>(dep[ComponentType{ "DEPOSIT" }]);
            if (target == Entity{} && depD->team == a->team)
                target = dep;
            else if (depD->team == a->team)
            {
                auto targetD = getObject<DepositComponent>(target[ComponentType{ "DEPOSIT" }]);

                float cur = depD->serving / glm::distance(_positions[base.getID()], _positions[dep.getID()]);
                float prev = targetD->serving / glm::distance(_positions[base.getID()], _positions[target.getID()]);

                if (depD->team == a->team && cur < prev)
                    target = dep;
            }
        }

        a->target = target;

        if (a->target != Entity{})
        {
            auto d = getObject<DepositComponent>(a->target[ComponentType{ "DEPOSIT" }]);
			d->serving++;
		}

        return true;
    };

    EffectFunction craftEffect = [&](const EntityMap &entities, const Entity &self) {
        auto w = getObject<WorkerComponent>(self[ComponentType{ "WORKER" }]);
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);

        if (!w->metal)
            return true;
        else if (w->elapsed > 1.0 && glm::distance(_positions[self.getID()], _positions[a->target.getID()]) < 8)
        {
            auto b = getObject<BaseComponent>(a->target[ComponentType{ "BASE" }]);

            w->elapsed = 0;
            w->metal--;
            b->metal++;

            return !w->metal;
        }
    };

    EffectFunction craftTarget = [&](const EntityMap &entities, const Entity &self) {
        auto w = getObject<WorkerComponent>(self[ComponentType{ "WORKER" }]);
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);

        std::vector<Entity> bases;
        for (auto &[id, entity] : entities)
        {
            if (entity.has(ComponentType{ "BASE" }))
                bases.push_back(entity);
        }

        Entity target{};
        for (auto &base : bases)
        {
            auto b = getObject<BaseComponent>(base[ComponentType{ "BASE" }]);

            if (b->team == a->team)
                target = base;
        }

        a->target = target;

        return true;
    };

    _effects["mine"] = std::make_tuple(mineTarget, SteeringBehaviour::ARRIVE, mineEffect);
    _effects["craft"] = std::make_tuple(craftTarget, SteeringBehaviour::ARRIVE, craftEffect);

    lua["newBullet"] = [&](glm::vec2 pos, glm::u8vec3 col, glm::vec2 dir, UUID64 owner, Team team) {
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

    lua["newAttacker"] = [this](glm::vec2 pos, int sides, glm::u8vec3 col, Team team) {
        // TODO ProceduralCondition need to be able to take arguments. In this case, EntityMap and Entity
        Action findTarget{
            "findTarget",
            1,
            {},
            {},
            {},
            ProceduralConditionSet{ []() { return Condition{ "target", "isAvailable", true, Operation::ASSIGN }; } }
        };

        Action getInRange{
            "getInRange",
            1,
            ConditionSet{ Condition{ "target", "isAvailable", true, Operation::EQUAL } },
            ConditionSet{ Condition{ "target", "isInRange", true, Operation::ASSIGN } }
        };

        Action shoot{
            "shoot",
            1,
            ConditionSet{
                Condition{ "target", "isInRange", true, Operation::EQUAL },
                Condition{ "self", "ammo", 0, Operation::GREATER } },
            ConditionSet{
                Condition{ "target", "isDead", true, Operation::ASSIGN },
                Condition{ "self", "threat", 1, Operation::MINUS } }
        };

        Action melee{
            "melee",
            2,
            ConditionSet{ Condition{ "target", "isInRange", true, Operation::EQUAL } },
            ConditionSet{
                Condition{ "target", "isDead", true, Operation::ASSIGN },
                Condition{ "self", "threat", 1, Operation::MINUS } }
        };
        Action scout{
            "scout",
            1,
            {},
            ConditionSet{ Condition{ "self", "isScouting", true, Operation::ASSIGN } }
        };

        Action resupply{
            "resupply",
            2,
            ConditionSet{ Condition{ "base", "hasAmmo", true, Operation::EQUAL } },
            ConditionSet{ Condition{ "self", "ammo", 1, Operation::PLUS } }
        };

        Action capture{
            "capture",
            1,
            ConditionSet{ Condition{ "world", "hasDeposit", true, Operation::EQUAL } },
            ConditionSet{ Condition{ "team", "hasDeposit", true, Operation::ASSIGN } }

        };

        ActionSet actions{ findTarget, getInRange, shoot, melee, resupply, scout, capture };

        Event e{
            UUID64{ 0 },                         // Entity ID. 0 because unneeded
            core::event::type::SPACE_NEW_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_NEW_ENTITY>(std::list<ComponentArgs>{
                ComponentArgs{ ComponentType{ "TRANSFORM" }, std::make_shared<TransformArgs>(pos, glm::vec2{ 3.2f }, 0.f) },
                ComponentArgs{ ComponentType{ "MOTION" }, std::make_shared<MotionArgs>(glm::vec2{}, glm::vec2{}, 64.f, 64.f, 1.f) },
                ComponentArgs{ ComponentType{ "SHAPE" }, std::make_shared<ShapeArgs>(sides, col) },
                ComponentArgs{ ComponentType{ "COLLIDER" }, std::make_shared<ColliderArgs>(128.f, static_cast<unsigned>(team)) },
                ComponentArgs{ ComponentType{ "AGENT" }, std::make_shared<AgentArgs>(team, actions) },
                ComponentArgs{ ComponentType{ "SOLDIER" }, {} } })
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

    // TODO EffectFunction begin_<<name>>, run_<<name>>, end_<<name>>
    EffectFunction findTargetEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType("AGENT")]);

        if (!entities.count(a->target.getID())) return true;

        return true;
    };

    EffectFunction findTargetTarget = [&](const EntityMap &entities, const Entity &self) {
        std::vector<Entity> agents;

        for (auto &[id, entity] : entities)
        {
            if (entity.has(ComponentType{ "AGENT" }))
                agents.push_back(entity);
        }

        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType{ "SOLDIER" }]);
        Entity target{};
        for (auto &agent : agents)
        {
            auto a2 = getObject<AgentComponent>(agent[ComponentType{ "AGENT" }]);

            if (target == Entity{} && a->team != a2->team)
                target = agent;
            if (a->team != a2->team && glm::distance(_positions[agent.getID()], _positions[self.getID()]) < glm::distance(_positions[target.getID()], _positions[self.getID()]))
                target = agent;
        }

        a->target = target;

        if (target == Entity{})
            return false;
        else
            return true;
    };

    EffectFunction getInRangeEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType("AGENT")]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (!entities.count(a->target.getID())) return true;

        if (glm::distance(_positions[a->target.getID()], _positions[self.getID()]) < 64)
            return true;
        else
            return false;
    };

    EffectFunction getInRangeTarget = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (!entities.count(a->target.getID()))
        {
            a->target = Entity{};
            return true;
        }

        auto t = getObject<AgentComponent>(a->target[ComponentType{ "AGENT" }]);

        if (a->team != t->team)
            return true;
        else
            return false;
    };

    EffectFunction shootEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType("AGENT")]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (!entities.count(a->target.getID())) return true;

        auto pos1 = _positions[a->target.getID()];
        auto pos2 = _positions[self.getID()];
        auto distance = glm::distance(pos1, pos2);

        if (distance > 64 || !s->ammo)
        {
            a->target = Entity{};

            return true;
        }
        else if (s->elapsed > 1.0 && distance < 32)
        {
            sol::function func = _lua["newBullet"];

            auto start = _positions[self.getID()];
            auto dir = glm::normalize(_positions[a->target.getID()] - _positions[self.getID()]);

            func(start, glm::u8vec3{ 255 }, dir, self.getID(), a->team);
            s->ammo--;
            s->elapsed = 0;

            return false;
        }

        return false;
    };

    EffectFunction shootTarget = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (!entities.count(a->target.getID()))
        {
            a->target = Entity{};
            return true;
        }

        auto t = getObject<AgentComponent>(a->target[ComponentType{ "AGENT" }]);

        if (a->team != t->team)
            return true;
        else
            return false;
    };

    EffectFunction meleeEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (!entities.count(a->target.getID())) return true;

        auto pos1 = _positions[a->target.getID()];
        auto pos2 = _positions[self.getID()];
        auto distance = glm::distance(pos1, pos2);

        if (distance > 64)
        {
            a->target = Entity{};

            return true;
        }
        else if (s->elapsed > 1.0 && distance < 16)
        {
            sol::function func = _lua["deleteEntity"];
            func(a->target.getID());

            s->elapsed = 0;
            a->target = Entity{};

            return true;
        }

        return false;
    };

    EffectFunction meleeTarget = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (!entities.count(a->target.getID()))
        {
            a->target = Entity{};
            return true;
        }

        auto t = getObject<AgentComponent>(a->target[ComponentType{ "AGENT" }]);

        if (a->team != t->team)
            return true;
        else
            return false;
    };

    // TODO integrate Lua "resupplying" and "serving" (ctrl-f in actions.lua)
    // TODO Add seperate blackboard for AgentComponent, WorkerComponent etc.
    EffectFunction resupplyEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        auto b = getObject<BaseComponent>(a->target[ComponentType{ "BASE" }]);

        if (!b->metal || a->blackboard.threat)
        {
            return true;
        }
        else if (glm::distance(_positions[a->target.getID()], _positions[self.getID()]) < 8)
        {
            s->ammo++;
            b->metal--;

            return true;
        }

        return false;
    };

    EffectFunction resupplyTarget = [&](const EntityMap &entities, const Entity &self) {
        std::vector<Entity> bases;

        for (auto &[id, entity] : entities)
        {
            if (entity.has(ComponentType{ "BASE" }))
                bases.push_back(entity);
        }

        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);
        Entity target{};
        for (auto &base : bases)
        {
            auto b = getObject<BaseComponent>(base[ComponentType{ "BASE" }]);

            if (b->team == a->team)
                target = base;
        }

        a->target = target;

        return true;
    };

    EffectFunction scoutEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        if (a->blackboard.threat != 0 || s->ammo < 3 /* && w->blackBoard.ammoAvailable*/)
            return true;
        else
            return false;
    };

    EffectFunction scoutTarget = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        a->target = self;

        return true;
    };

    EffectFunction captureEffect = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);
        auto d = getObject<DepositComponent>(a->target[ComponentType{ "DEPOSIT" }]);

        if (d->team == a->team || a->blackboard.threat != 0)
            return true;
        else
            return false;
    };

    EffectFunction captureTarget = [&](const EntityMap &entities, const Entity &self) {
        auto a = getObject<AgentComponent>(self[ComponentType{ "AGENT" }]);
        auto s = getObject<SoldierComponent>(self[ComponentType("SOLDIER")]);

        std::vector<Entity> deposits;
        for (auto &[id, entity] : entities)
        {
            if (entity.has(ComponentType{ "DEPOSIT" }))
            {
                auto d = getObject<DepositComponent>(entity[ComponentType{ "DEPOSIT" }]);
                if (d->team != a->team)
                    deposits.push_back(entity);
            }
        }

        if (!deposits.size())
            return false;

        std::vector<Entity> target;

        std::sample(deposits.begin(), deposits.end(), std::back_inserter(target), 1, std::mt19937{ std::random_device{}() });

        a->target = target[0];

        return true;
    };

    _effects["findTarget"] = std::make_tuple(findTargetTarget, SteeringBehaviour::STOP, findTargetEffect);
    _effects["getInRange"] = std::make_tuple(getInRangeTarget, SteeringBehaviour::SEEK, getInRangeEffect);
    _effects["shoot"] = std::make_tuple(shootTarget, SteeringBehaviour::MAINTAIN, shootEffect);
    _effects["melee"] = std::make_tuple(meleeTarget, SteeringBehaviour::SEEK, meleeEffect);
    _effects["resupply"] = std::make_tuple(resupplyTarget, SteeringBehaviour::ARRIVE, resupplyEffect);
    _effects["scout"] = std::make_tuple(scoutTarget, SteeringBehaviour::WANDER, scoutEffect);
    _effects["capture"] = std::make_tuple(captureTarget, SteeringBehaviour::ARRIVE, captureEffect);

    lua["deleteEntity"] = [&](const UUID64 &id) {
        Event e{
            id,                                     // Entity ID.
            core::event::type::SPACE_DELETE_ENTITY, // Event type enum
            std::make_shared<core::event::data::SPACE_DELETE_ENTITY>()
        };

        _eventStream.pushEvent(e, StreamType::OUTGOING);
    };

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
    if ((elapsed += delta) < _interval)
        return {};
    else
        elapsed -= _interval;

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
        std::vector<Entity> soldiers;
        for (auto &agent : agents)
        {
            auto a = getObject<AgentComponent>(agent[ComponentType{ "AGENT" }]);

            if (a->team != b->team) continue;

            if (agent.has(ComponentType{ "WORKER" }))
                workers.push_back(agent);
            else if (agent.has(ComponentType{ "SOLDIER" }))
                soldiers.push_back(agent);
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

        // build worker
        if (b->metal >= 5)
        {
            if (workers.size() <= soldiers.size())
            {
                b->metal -= 2;
                sol::function build = _lua["newWorker"];
                build(_positions[base.getID()], 16, col, static_cast<unsigned>(b->team));
            }
            else
            {
                b->metal -= 5;
                sol::function build = _lua["newAttacker"];
                build(_positions[base.getID()], 3, col, static_cast<unsigned>(b->team));
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

                if (a->blackboard.threat)
                    goal = Action{ "GOAL", 0, ConditionSet{ { "self", "threat", a->blackboard.threat, Operation::LESS } }, {} };
                else if (s->ammo < 3 && hasAmmo)
                    goal = Action{ "GOAL", 0, ConditionSet{ { "self", "ammo", s->ammo, Operation::GREATER } }, {} };
                else if (hasDeposit)
                    goal = Action{ "GOAL", 0, ConditionSet{ { "team", "hasDeposit", true, Operation::EQUAL } }, {} };
                else
                    goal = Action{ "GOAL", 0, ConditionSet{ { "self", "isScouting", true, Operation::EQUAL } }, {} };
            }

            a->planner.setWorldState(worldState);
            if ((a->curPlan = a->planner.plan(a->actions, goal)) != ActionSet{})
            {
                a->curAction = *a->curPlan.begin();

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
                if (collision.group != static_cast<int>(b->team) && collision.group != GROUP_DEPOSIT && collision.group != GROUP_BULLET)
                {
                    uuid = collision.target;
                    dist = collision.distance;
                }
            }

            if (dist != std::numeric_limits<int>::max())
            {
                sol::function func = _lua["deleteEntity"];
                func(uuid);
            }
        }
    }

    pushEvents(events, StreamType::OUTGOING);

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
        DepositArgs args = *(std::static_pointer_cast<DepositArgs>(tuplePtr));

        h = emplaceObject<DepositComponent>(std::get<0>(args), std::get<1>(args));
    }
    else if (type == "AGENT")
    {
        AgentArgs args = *(std::static_pointer_cast<AgentArgs>(tuplePtr));

        h = emplaceObject<AgentComponent>(std::get<0>(args), std::get<1>(args));
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
