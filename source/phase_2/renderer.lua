renderer = {
	overlay = false
}

function renderer.init()
end

function renderer.update()
	Render.draw.text(tostring(game.wins.blue), glm.vec2.new(0,0), 32, glm.u8vec3.new(0, 0, 255))
    Render.draw.text(tostring(game.wins.red), glm.vec2.new(0,32), 32, glm.u8vec3.new(255, 0, 0))

	for _, base in pairs(game.bases) do
		Render.draw.text(tostring(base.ammo), base.pos - glm.vec2.new(15,20), 32, glm.u8vec3.new(0, 0, 0))
        Render.draw.text(tostring(base.resupplying), base.pos + glm.vec2.new(32, 32) - glm.vec2.new(15,20), 32, glm.u8vec3.new(255, 255, 255))
	end

	for _, resource in pairs(game.deposits) do
		Render.draw.text(tostring(resource.value), resource.pos + glm.vec2.new(32, 0) - glm.vec2.new(15,20) , 32, glm.u8vec3.new(255, 255, 255))
		Render.draw.text(tostring(resource.serving), resource.pos + glm.vec2.new(32, 32) - glm.vec2.new(15,20) , 32, glm.u8vec3.new(255, 255, 255))
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
