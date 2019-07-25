renderer = {
    tilemap = {},
    zoom = 0.2,
    camera = glm.vec2.new(2048, 2048),
    overlay = false
}

function renderer.init()
    renderer.tilemap = Render.TileMap()

    renderer.camera = (renderer.camera - glm.vec2.new(512 / renderer.zoom, 512 / renderer.zoom))
end

function renderer.update(delta)
    Render.draw.text(tostring(game.wins.red), glm.vec2.new(0,0), 32, glm.u8vec3.new(255, 0, 0))
    Render.draw.text(tostring(game.wins.green), glm.vec2.new(0,32), 32, glm.u8vec3.new(0, 255, 0))
    Render.draw.text(tostring(game.wins.blue), glm.vec2.new(0,64), 32, glm.u8vec3.new(0, 0, 255))
    Render.draw.text(tostring(game.wins.yellow), glm.vec2.new(0,96), 32, glm.u8vec3.new(255, 255, 0))

    -- BASES
    for k, base in pairs(game.bases) do
        Render.draw.polygon(4, 64 * renderer.zoom, (base.pos - renderer.camera) * renderer.zoom, 3.14159 / 4, glm.u8vec3.new(100, 100, 100))
        Render.draw.text(tostring(base.ammo), ((base.pos - glm.vec2.new(64, 64)) - renderer.camera) * renderer.zoom, 32, glm.u8vec3.new(255, 255, 255))
        Render.draw.text(tostring(base.resupplying), ((base.pos + glm.vec2.new(64, 64)) - renderer.camera) * renderer.zoom, 32, glm.u8vec3.new(255, 255, 255))
    end

    -- RESOURCES
    for k, resource in pairs(game.deposits) do
        local col = glm.u8vec3.new(100, 100, 100)
        if resource.team == "RED" then col = glm.u8vec3.new(255, 100, 100) end
        if resource.team == "GREEN" then col = glm.u8vec3.new(100, 255, 100) end
        if resource.team == "BLUE" then col = glm.u8vec3.new(100, 100, 255) end
        if resource.team == "YELLOW" then col = glm.u8vec3.new(255, 255, 100) end

        Render.draw.polygon(4, 32 * renderer.zoom, (resource.pos - renderer.camera) * renderer.zoom, 0, col)
        Render.draw.text(tostring(resource.value), ((resource.pos + glm.vec2.new(64, -64)) - renderer.camera) * renderer.zoom, 16, glm.u8vec3.new(255, 255, 255))
        Render.draw.text(tostring(resource.serving), ((resource.pos + glm.vec2.new(64, 64)) - renderer.camera) * renderer.zoom, 16, glm.u8vec3.new(255, 255, 255))
    end

    -- AGENTS
    local r, g, b, y = 0, 0, 0, 0
    for i = 1, #game.agents do
        local agent = game.agents[i]

        if agent.team == "RED" then r = r + 1
        elseif agent.team == "GREEN" then g = g + 1
        elseif agent.team == "BLUE" then b = b + 1
        elseif agent.team == "YELLOW" then y = y + 1 end

        Render.draw.polygon(agent.sides, 16 * renderer.zoom, (agent.pos - renderer.camera) * renderer.zoom, agent.rot, agent.col)
    end
    
    Render.draw.text(tostring(r), glm.vec2.new(512,0), 32, glm.u8vec3.new(255, 0, 0))
    Render.draw.text(tostring(g), glm.vec2.new(512,32), 32, glm.u8vec3.new(0, 255, 0))
    Render.draw.text(tostring(b), glm.vec2.new(512,64), 32, glm.u8vec3.new(0, 0, 255))
    Render.draw.text(tostring(y), glm.vec2.new(512,96), 32, glm.u8vec3.new(255, 255, 0))

    -- BULLETS
    for i = 1, #game.bullets do
        local bullet = game.bullets[i]

        Render.draw.polygon(8, 4 * renderer.zoom, (bullet.pos - renderer.camera) * renderer.zoom, 0, glm.u8vec3.new(255, 255, 255))
    end

    -- OVERLAY
    if renderer.overlay then
        local string = ""
        for i = 1, #game.agents do
            local agent = game.agents[i]

            local action = {name = "nil", behaviour = "nil"}

            if agent.curAction ~= 0 then action = agent.curPlan[agent.curAction] end

            if agent.class == "Attacker" or agent.class == "Defender" then
                string = string .. agent.name .."\t".. action.name .."\t".. action.behaviour .."\t".. glm.length(agent.vel) .."\t".. agent.ammo .."\n"
            elseif agent.class == "Worker" then
                string = string .. agent.name .."\t".. action.name .."\t".. action.behaviour .."\t".. glm.length(agent.vel) .."\t".. agent.resource .."\n"
            end
        end

        Render.draw.text(string, glm.vec2.new(0, 0), 20, glm.u8vec3.new(255, 255, 255))
    end
end
