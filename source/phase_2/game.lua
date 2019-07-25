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
    wins = {red = 0, green = 0, blue = 0, yellow = 0}
}

function game.init()
    -- BASES
    local bases = {}
    table.insert(bases, Base.new("RED", glm.vec2.new(128, 128), glm.u8vec3.new(255, 100, 100)))
    table.insert(bases, Base.new("GREEN", glm.vec2.new(896, 128), glm.u8vec3.new(100, 255, 100)))
    table.insert(bases, Base.new("BLUE", glm.vec2.new(896, 896), glm.u8vec3.new(100, 100, 255)))
    table.insert(bases, Base.new("YELLOW", glm.vec2.new(128, 896), glm.u8vec3.new(255, 255, 100)))
    
    for _, base in pairs(bases) do
        base.cycle = 0

        factory.construct(
            base,
            "bases",
            {"transform", "shape"}
        )
    end

    -- RESOURCE DEPOSITS
    local deposits = {}
    table.insert(deposits, Deposit.new(glm.vec2.new(192, 192), {red = 1.0, green = 0.0, blue = 0.0, yellow = 0.0}))
    table.insert(deposits, Deposit.new(glm.vec2.new(832, 192), {red = 0.0, green = 1.0, blue = 0.0, yellow = 0.0}))
    table.insert(deposits, Deposit.new(glm.vec2.new(832, 832), {red = 0.0, green = 0.0, blue = 1.0, yellow = 0.0}))
    table.insert(deposits, Deposit.new(glm.vec2.new(192, 832), {red = 0.0, green = 0.0, blue = 0.0, yellow = 1.0}))

    local start = 256
    local span = 512
    local size = 2
    for i = 0, size, 1 do
        for j = 0, size, 1 do
            local x = start + i * span / size
            local y = start + j * span / size 
            table.insert(deposits, Deposit.new(glm.vec2.new(x, y), {red = 0.0, green = 0.0, blue = 0.0, yellow = 0.0}))
        end
    end

    for _, depo in pairs(deposits) do
        base.cycle = 0

        factory.construct(
            depo,
            "deposits",
            {"transform", "shape"}
        )
    end

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
    local greenDead = true
    local blueDead = true
    local yellowDead = true
    for _, a in pairs(game.agents) do
        if not redDead and not greenDead and not blueDead and not yellowDead then break
        elseif a.team == "RED" then redDead = false
        elseif a.team == "GREEN" then greenDead = false
        elseif a.team == "BLUE" then blueDead = false
        elseif a.team == "YELLOW" then yellowDead = false end
    end

    local remaining = 4;
    if redDead then remaining = remaining - 1 end
    if greenDead then remaining = remaining - 1 end
    if blueDead then remaining = remaining - 1 end
    if yellowDead then remaining = remaining - 1 end

    if remaining == 1 then
        if not redDead then game.wins.red = game.wins.red + 1 end
        if not greenDead then game.wins.green = game.wins.green + 1 end
        if not blueDead then game.wins.blue = game.wins.blue + 1 end
        if not yellowDead then game.wins.yellow = game.wins.yellow + 1 end

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
        print(tostring(game.wins.red)..'\t'..tostring(game.wins.green)..'\t'..tostring(game.wins.blue)..'\t'..tostring(game.wins.yellow))

        return true
    end
    return false
end
