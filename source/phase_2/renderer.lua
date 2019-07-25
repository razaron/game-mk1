renderer = {
	overlay = false
}

function renderer.init()
end

function renderer.update()
    Render.draw.text(tostring(game.wins.red), glm.vec2.new(0,0), 32, glm.u8vec3.new(255, 0, 0))
    Render.draw.text(tostring(game.wins.green), glm.vec2.new(0,32), 32, glm.u8vec3.new(0, 255, 0))
	Render.draw.text(tostring(game.wins.blue), glm.vec2.new(0,64), 32, glm.u8vec3.new(0, 0, 255))
    Render.draw.text(tostring(game.wins.yellow), glm.vec2.new(0,96), 32, glm.u8vec3.new(255, 255, 0))

    -- AGENTS
    local r, g, b, y = 0, 0, 0, 0
    for i = 1, #game.agents do
        local agent = game.agents[i]

        if agent.team == "RED" then r = r + 1
        elseif agent.team == "GREEN" then g = g + 1
        elseif agent.team == "BLUE" then b = b + 1
        elseif agent.team == "YELLOW" then y = y + 1 end
    end

    Render.draw.text(tostring(r), glm.vec2.new(512,0), 32, glm.u8vec3.new(255, 0, 0))
    Render.draw.text(tostring(g), glm.vec2.new(512,32), 32, glm.u8vec3.new(0, 255, 0))
    Render.draw.text(tostring(b), glm.vec2.new(512,64), 32, glm.u8vec3.new(0, 0, 255))
    Render.draw.text(tostring(y), glm.vec2.new(512,96), 32, glm.u8vec3.new(255, 255, 0))
    
    -- BASES
	for _, base in pairs(game.bases) do
		Render.draw.text(tostring(base.ammo), base.pos - glm.vec2.new(15,20), 32, glm.u8vec3.new(0, 0, 0))
        Render.draw.text(tostring(base.resupplying), base.pos + glm.vec2.new(32, 32) - glm.vec2.new(15,20), 32, glm.u8vec3.new(255, 255, 255))
	end

    -- DEPOSITS
	for _, resource in pairs(game.deposits) do
		Render.draw.text(tostring(resource.value), resource.pos + glm.vec2.new(32, 0) - glm.vec2.new(15,20) , 16, glm.u8vec3.new(255, 255, 255))
		Render.draw.text(tostring(resource.serving), resource.pos + glm.vec2.new(32, 32) - glm.vec2.new(15,20) , 16, glm.u8vec3.new(255, 255, 255))
	end

	-- OVERLAY
    if renderer.overlay then
        local string = ""
        for i = 1, #game.agents do
            local agent = game.agents[i]

            local action = {name = "nil", behaviour = "nil"}

            if agent.curAction ~= 0 then action = agent.curPlan[agent.curAction] end

            if agent.class == "Attacker" or agent.class == "Defender" then
                string = string .. agent.name .."\t".. action.name .."\t".. action.behaviour .."\t".. glm.length(agent.motion.velocity) .."\t".. agent.ammo .."\n"
            elseif agent.class == "Worker" then
                string = string .. agent.name .."\t".. action.name .."\t".. action.behaviour .."\t".. glm.length(agent.motion.velocity) .."\t".. agent.resource .."\n"
            end
        end

        Render.draw.text(string, glm.vec2.new(0, 0), 20, glm.u8vec3.new(255, 255, 255))
    end
end
