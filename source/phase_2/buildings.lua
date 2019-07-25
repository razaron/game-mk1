local agents = require "agents"

--[[===============================
		HOMEBASE CLASS
===============================]]--
Base = {
    class = "Base",

    team = "DEFAULT",
    ammo = 10,
    resupplying = 0,

    -- Components
    transform = nil,
    shape = nil,
    pos = nil
}

function Base.new(team, pos)
    local self = {}
    setmetatable(self, {__index = Base})

    self.team = team or Base.team
    self.ammo = Base.ammo
    self.resupplying = Base.resupplying

    self.transform = nil
    self.shape = nil
    self.pos = pos

    newBase(self, pos or glm.vec2.new(64, 64), glm.u8vec3.new(100, 100, 100))

    return self
end

function Base:update()
    local x, y, z = 0, 0, 0

    for _, a in pairs(game.agents) do
        if a.team == self.team then
            if a.class == "Worker" then x = x + 1
            elseif a.class == "Attacker" then y = y + 1
            elseif a.class == "Defender" then z = z + 1 end
        end
    end

    if x <= y + z then
        if self.ammo > 2 then
            self:build("Worker")
        end
    elseif y <= z then
        if self.ammo > 5 then
            self:build("Attacker")
        end
    else
        if self.ammo > 5 then
            self:build("Defender")
        end
    end
end

function Base:build(type)
    local colour = glm.u8vec3.new(100, 100, 100)
    if self.team == "RED" then colour = glm.u8vec3.new(255, 0, 0)
    elseif self.team == "GREEN" then colour = glm.u8vec3.new(0, 255, 0)
    elseif self.team == "BLUE" then colour = glm.u8vec3.new(0, 0, 255)
    elseif self.team == "YELLOW" then colour = glm.u8vec3.new(255, 255, 0) end

    if type == "Worker" then
        local agent = Worker.new(self.team.."_CRAFTER", glm.vec2.new(self.pos), colour, self.team)
        agent.team = self.team

        if self.ammo >= 2 then
            self.ammo = self.ammo - 2
            factory.construct(agent, "agents", {"transform", "shape"})
        end
    elseif type == "Attacker" then
        local agent = Attacker.new(self.team.."_ATTACKER", glm.vec2.new(self.pos), colour, self.team)
        agent.team = self.team
        agent.ammo = 3

        if not logging then logging = agent end

        if self.ammo >= 5 then
            self.ammo = self.ammo - 5
            factory.construct(agent, "agents", {"transform", "shape"})
        end
    elseif type == "Defender" then
        local agent = Defender.new(self.team.."_DEFENDER", glm.vec2.new(self.pos), colour, self.team)
        agent.team = self.team
        agent.ammo = 3

        if self.ammo >= 5 then
            self.ammo = self.ammo - 5
            factory.construct(agent, "agents", {"transform", "shape"})
        end
    end
end

--[[===============================
		DEPOSIT CLASS
===============================]]--
Deposit = {
    class = "Deposit",

    team = "DEFAULT",
    value = 0,
    elapsed = 0,
	control = {red = 0.0, green = 0.0, blue = 0.0, yellow = 0.0}, -- How close to being taken over
    serving = 0,

    -- Components
    transform = nil,
    shape = nil,
    pos = nil
}

function Deposit.new(pos, control)
    local self = {}
    setmetatable(self, {__index = Deposit})

    self.team = Deposit.team
    self.value = Deposit.value
    self.control = control or Deposit.control
    self.serving = Deposit.serving

    self.transform = nil
    self.shape = nil
    self.pos = pos

    newDeposit(self, pos or glm.vec2.new(64, 64), glm.u8vec3.new(100, 100, 100))

    return self
end

function Deposit:update()
    self.elapsed = self.elapsed + game.delta

    if self.elapsed >= 1 then
        self.value = self.value + 1
        self.elapsed = self.elapsed - 1
    end

    local red = getCollisions(self.uuid, 1)
    local green = getCollisions(self.uuid, 2)
    local blue = getCollisions(self.uuid, 3)
    local yellow = getCollisions(self.uuid, 4)

    local max = 0, leader;
	if #red > max then leader = "RED" end
	if #green > max then leader = "GREEN" end
	if #blue > max then leader = "BLUE" end
	if #yellow > max then leader = "YELLOW" end

	if leader == "RED" then
		local average = (#green + #blue + #yellow) / 3
		if self.control.green > 0.0 or self.control.blue > 0.0 or self.control.yellow > 0.0 then
			self.control.green = self.control.green - (#red - #green) * game.delta / 10
			self.control.blue = self.control.blue - (#red - #blue) * game.delta / 10
			self.control.yellow = self.control.yellow - (#red - #yellow) * game.delta / 10
		else
			self.control.red = self.control.red + (#red - average) * game.delta / 10
		end
	elseif leader == "GREEN" then
		local average = (#red + #blue + #yellow) / 3
		if self.control.red > 0.0 or self.control.blue > 0.0 or self.control.yellow > 0.0 then
			self.control.red = self.control.red - (#green - #red) * game.delta / 10
			self.control.blue = self.control.blue - (#green - #blue) * game.delta / 10
			self.control.yellow = self.control.yellow - (#green - #yellow) * game.delta / 10
		else
			self.control.green = self.control.green + (#green - average) * game.delta / 10
		end
	elseif leader == "BLUE" then
		local average = (#green + #red + #yellow) / 3
		if self.control.red > 0.0 or self.control.green > 0.0 or self.control.yellow > 0.0 then
			self.control.green = self.control.green - (#blue - #green) * game.delta / 10
			self.control.red = self.control.red - (#blue - #red) * game.delta / 10
			self.control.yellow = self.control.yellow - (#blue - #yellow) * game.delta / 10
		else
			self.control.blue = self.control.blue + (#blue - average) * game.delta / 10
		end
	elseif leader == "YELLOW" then
		local average = (#green + #blue + #red) / 3
		if self.control.red > 0.0 or self.control.green > 0.0 or self.control.blue > 0.0 then
			self.control.green = self.control.green - (#yellow - #green) * game.delta / 10
			self.control.blue = self.control.blue - (#yellow - #blue) * game.delta / 10
			self.control.red = self.control.red - (#yellow - #red) * game.delta / 10
		else
			self.control.yellow = self.control.yellow + (#yellow - average) * game.delta / 10
		end
	end

	if self.control.red <= 0.0 then self.control.red = 0.0 end
	if self.control.green <= 0.0 then self.control.green = 0.0 end
	if self.control.blue <= 0.0 then self.control.blue = 0.0 end
	if self.control.yellow <= 0.0 then self.control.yellow = 0.0 end

	if self.control.red >= 1.0 then 
		self.control.red = 1.0
		self.team = "RED"
        self.shape.colour = glm.u8vec3.new(255, 100, 100)
	elseif self.control.green >= 1.0 then 
		self.control.green = 1.0 
		self.team = "GREEN"
        self.shape.colour = glm.u8vec3.new(100, 255, 100)
	elseif self.control.blue >= 1.0 then 
		self.control.blue = 1.0 
		self.team = "BLUE"
        self.shape.colour = glm.u8vec3.new(100, 100, 255)
	elseif self.control.yellow >= 1.0 then 
		self.control.yellow = 1.0 
		self.team = "YELLOW"
        self.shape.colour = glm.u8vec3.new(255, 255, 100)
	else
		self.team = "DEFAULT"
        self.shape.colour = glm.u8vec3.new(100, 100, 100)
	end
end
