local agents = require("agents")
local base = require("homebase")
local deposit = require("deposit")

game = {
    frame = 0,
    running = true,
    player = {},
    agents = {},
    bullets = {},
    deposits = {},
    bases = {},
    mouse = glm.vec2.new(0, 0),
    delta = 0.0,
    wins = {red = 0, green = 0, blue = 0, yellow = 0}
}

function game.init()
    -- BASES
    table.insert(game.bases, Base.new(glm.vec2.new(256, 256), "RED", glm.u8vec3.new(255, 0, 0)))
    table.insert(game.bases, Base.new(glm.vec2.new(4096 - 256, 256), "GREEN", glm.u8vec3.new(0, 255, 0)))
    table.insert(game.bases, Base.new(glm.vec2.new(4096 - 256, 4096 - 256), "BLUE", glm.u8vec3.new(0, 0, 255)))
    table.insert(game.bases, Base.new(glm.vec2.new(256, 4096 - 256), "YELLOW", glm.u8vec3.new(255, 255, 0)))

    game.bases[1].cycle = 0
    game.bases[2].cycle = 0
    game.bases[3].cycle = 0
    game.bases[4].cycle = 0

    -- RESOURCE DEPOSITS
    table.insert(game.deposits, Deposit.new(glm.vec2.new(512, 512), {red = 1.0, green = 0.0, blue = 0.0, yellow = 0.0}))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(3584, 512), {red = 0.0, green = 1.0, blue = 0.0, yellow = 0.0}))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(3584, 3584), {red = 0.0, green = 0.0, blue = 1.0, yellow = 0.0}))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(512, 3584), {red = 0.0, green = 0.0, blue = 0.0, yellow = 1.0}))

    local start = 1024
    local span = 2048
    local size = 2
    
    for i = 0, size, 1 do
        for j = 0, size, 1 do
            local x = start + i * span / size
            local y = start + j * span / size 
            table.insert(game.deposits, Deposit.new(glm.vec2.new(x, y), {red = 0.0, green = 0.0, blue = 0.0, yellow = 0.0}))
        end
    end
end

function game.update(delta)
    game.delta = delta
    if game.running then
        game.frame = game.frame + 1

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

            game.agents = {}
            game.bullets = {}
            game.deposits = {}
            game.bases = {}
            game.init()
        end

        -- UPDATE AGENTS
        for i = #game.agents, 1, - 1 do
            local agent = game.agents[i]

            if agent.isDead then
                table.remove(game.agents, i)
                local action = agent.curPlan[agent.curAction]

                if type(action) == "function" then
                    action = action()
                end

                if action and action.name == "resupply" then
                    for _, b in pairs(game.bases) do
                        if b.team == agent.team then
                            b.resupplying = b.resupplying - 1
                            break
                        end
                    end
                end
            else
                if agent.lastShot then agent.lastShot = agent.lastShot + delta end
                if agent.lastCraft then agent.lastCraft = agent.lastCraft + delta end
                if agent.lastMine then agent.lastMine = agent.lastMine + delta end

                agent:update()
            end
        end

        -- UPDATE BASES --
        for _, base in pairs(game.bases) do
            local x, y, z = 0, 0, 0

            for _, a in pairs(game.agents) do
                if a.team == base.team then
                    if a.class == "Worker" then x = x + 1
                    elseif a.class == "Attacker" then y = y + 1
                    elseif a.class == "Defender" then z = z + 1 end
                end
            end

            if x <= y + z then
				if base.ammo > 2 then
                	base:build("Worker")
				end
            elseif y <= z then
				if base.ammo > 5 then
                	base:build("Attacker")
				end
            else
				if base.ammo > 5 then
                	base:build("Defender")
				end
            end
        end

        -- UPDATE DEPOSITS
        for _, depo in pairs(game.deposits) do
            depo:update(delta)
        end

        -- UPDATE BULLETS
        for i = #game.bullets, 1, - 1 do
            local bullet = game.bullets[i]

            local dir = glm.normalize(bullet.target - bullet.pos)

            if glm.length(bullet.target - bullet.pos) < 1000 * delta then
                table.remove(game.bullets, i)
            end

            bullet.pos = bullet.pos + dir * 1000 * delta

            -- CHECK AGENT-BULLET COLLISION
            for j = #game.agents, 1, - 1 do
                local agent = game.agents[j]

                if agent.team ~= bullet.owner and bullet.pos.x > agent.pos.x - 8 and bullet.pos.x < agent.pos.x + 8 and bullet.pos.y > agent.pos.y - 8 and bullet.pos.y < agent.pos.y + 8 then
                    table.remove(game.bullets, i)

                    agent.isDead = true
                end
            end
        end
    end
end
