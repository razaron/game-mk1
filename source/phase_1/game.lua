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
    wins = {blue = 0, red = 0}
}

function game.init()
    -- BASES
    table.insert(game.bases, Base.new(glm.vec2.new(256, 256), "BLUE", glm.u8vec3.new(0, 0, 255)))
    table.insert(game.bases, Base.new(glm.vec2.new(4096 - 256, 4096 - 256), "RED", glm.u8vec3.new(255, 0, 0)))

    game.bases[1].cycle = 0
    game.bases[2].cycle = 0

    -- RESOURCE DEPOSITS
    table.insert(game.deposits, Deposit.new(glm.vec2.new(1024, 1024), 1.0))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(1024, 3072), 0.0))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(2048, 2048), 0.0))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(3072, 1024), 0.0))
    table.insert(game.deposits, Deposit.new(glm.vec2.new(3072, 3072), - 1.0))
end

function game.update(delta)
    game.delta = delta
    if game.running then
        game.frame = game.frame + 1

        local redDead = true
        local blueDead = true
        for _, a in pairs(game.agents) do
            if not redDead and not blueDead then break
            elseif a.team == "RED" then redDead = false
            elseif a.team == "BLUE" then blueDead = false end
        end

        if redDead or blueDead then
            if blueDead then game.wins.blue = game.wins.blue + 1 end
            if redDead then game.wins.red = game.wins.red + 1 end

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

            if glm.length(bullet.target - bullet.pos) < 2000 * delta then
                table.remove(game.bullets, i)
            end

            bullet.pos = bullet.pos + dir * 2000 * delta

            -- CHECK AGENT-BULLET COLLISION
            for j = #game.agents, 1, - 1 do
                local agent = game.agents[j]

                if agent.team ~= bullet.owner and bullet.pos.x > agent.pos.x - 16 and bullet.pos.x < agent.pos.x + 16 and bullet.pos.y > agent.pos.y - 16 and bullet.pos.y < agent.pos.y + 16 then
                    table.remove(game.bullets, i)

                    agent.isDead = true
                end
            end
        end
    end
end
