ProFi = require 'ProFi'
local agents = require "agents"
local buildings = require "buildings"

factory = {
    constructing = {},
    destructing = {}
}

function factory.construct(obj, dest, components)
    table.insert(factory.constructing, {["obj"] = obj, ["dest"] = dest, ["components"] = components})
end

function factory.destruct(obj)
    table.insert(factory.destructing, obj)
end

function factory.killAll()
    for i = #factory.constructing, 1, - 1 do
        local data = factory.constructing[i]
        table.remove(factory.constructing, i)
        factory.destruct(data.obj)
    end
end

function factory.update()
    for i = #factory.constructing, 1, - 1 do
        local data = factory.constructing[i]

        -- Check if all components have been made
        local valid = true
        for _, c in pairs(data.components) do
            if data.obj[c] == nil then valid = false end
        end

        if valid then
            local x = data.obj.transform.translation.x
            table.insert(game[data.dest], data.obj)
            table.remove(factory.constructing, i)
        end
    end

    for i = #factory.destructing, 1, - 1 do
        local obj = factory.destructing[i]

        if obj.uuid then
            deleteEntity(obj.uuid)
            table.remove(factory.destructing, i)

            if obj.class == "Bullet" then destructedBullets = destructedBullets + 1 end
            if obj.class == "Base" then destructedBases = destructedBases + 1 end
            if obj.class == "Deposit" then destructedDeposits = destructedDeposits + 1 end
            if obj.class == "Agent" or obj.class == "Soldier" or obj.class == "Attacker" or obj.class == "Defender"  or obj.class == "Worker" then destructedAgents = destructedAgents + 1 end
        end
    end
end

game = {
    delta = 0.0,
    agents = {},
    bases = {},
    deposits = {},
    bullets = {},
    wins = {blue = 0, red = 0}
}

function game.init()
    -- BASES
    local blueBase = Base.new("BLUE", glm.vec2.new(256 / 5 + 1024/10, 256 / 5 + 1024/10), glm.u8vec3.new(100, 100, 255))
    blueBase.cycle = 0

    factory.construct(
        blueBase,
        "bases",
        {"transform", "shape"}
    )

    local redBase = Base.new("RED", glm.vec2.new((4096 - 256) / 5 + 1024/10, (4096 - 256) / 5 + 1024/10), glm.u8vec3.new(100, 100, 255))
    redBase.cycle = 0

    factory.construct(
        redBase,
        "bases",
        {"transform", "shape"}
    )

    -- RESOURCE DEPOSITS
    factory.construct(
        Deposit.new(glm.vec2.new(1024 / 5 + 1024/10, 1024 / 5 + 1024/10), 1.0),
        "deposits",
        {"transform", "shape"}
    )
    factory.construct(
        Deposit.new(glm.vec2.new(1024 / 5 + 1024/10, 3072 / 5 + 1024/10), 0.0),
        "deposits",
        {"transform", "shape"}
    )
    factory.construct(
        Deposit.new(glm.vec2.new(2048 / 5 + 1024/10, 2048 / 5 + 1024/10), 0.0),
        "deposits",
        {"transform", "shape"}
    )
    factory.construct(
        Deposit.new(glm.vec2.new(3072 / 5 + 1024/10, 1024 / 5 + 1024/10), 0.0),
        "deposits",
        {"transform", "shape"}
    )
    factory.construct(
        Deposit.new(glm.vec2.new(3072 / 5 + 1024/10, 3072 / 5 + 1024/10), - 1.0),
        "deposits",
        {"transform", "shape"}
    )

    --ProFi:start()
end

deadAgents = 0
deadBullets = 0
deadBases = 0
deadDeposits = 0
destructedAgents = 0
destructedBullets = 0
destructedBases = 0
destructedDeposits = 0
newAgents = 0
newBullets = 0
newBases = 0
newDeposits = 0

frame = 1
elapsed = 1
function game.update(delta)
    factory.update()
    game.delta = delta

    frame = frame + 1

    elapsed = elapsed - delta
    if elapsed < 0 then
        elapsed = 1
    end

    -- UPDATE BASES --
    for i = #game.bases, 1, - 1 do
        local base = game.bases[i]

        base:update()
    end

    -- UPDATE AGENTS
    for i = #game.agents, 1, - 1 do
        local agent = game.agents[i]

        if agent.isDead then
            factory.destruct(agent)
            table.remove(game.agents, i)
            agent.planner = nil
            if isWon() then return end
        else
            agent:update()
        end
    end

    -- UPDATE DEPOSITS
    for i = #game.deposits, 1, - 1 do
        local deposit = game.deposits[i]

        deposit:update()
    end

    -- UPDATE BULLETS
    for i = #game.bullets, 1, - 1 do
        local bullet = game.bullets[i]
        if bullet.isDead then
            factory.destruct(bullet)
            table.remove(game.bullets, i)
        else
            bullet:update()
        end
    end
end

function isWon()
    -- Determine win state
    local redDead = true
    local blueDead = true
    for _, a in pairs(game.agents) do
        if not redDead and not blueDead then break
        elseif a.team == "RED" then redDead = false
        elseif a.team == "BLUE" then blueDead = false end
    end

    if redDead or blueDead or #game.agents > 64 then
        if blueDead then game.wins.red = game.wins.red + 1 end
        if redDead then game.wins.blue = game.wins.blue + 1 end

        for i = #game.agents, 1, - 1 do
            local agent = game.agents[i]
            factory.destruct(agent)
        end
        game.agents = {}

        for i = #game.bullets, 1, - 1 do
            local bullet = game.bullets[i]
            factory.destruct(bullet)
        end
        game.bullets = {}

        for i = #game.deposits, 1, - 1 do
            local deposit = game.deposits[i]
            factory.destruct(deposit)
        end
        game.deposits = {}


        for i = #game.bases, 1, - 1 do
            local base = game.bases[i]
            factory.destruct(base)
        end
        game.bases = {}

        factory.killAll()
        game.init()
        print(tostring(game.wins.blue)..'\t'..tostring(game.wins.red))

        return true
    end
    return false
end
