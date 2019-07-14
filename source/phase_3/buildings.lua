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
    elseif self.team == "BLUE" then colour = glm.u8vec3.new(0, 0, 255) end

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
    control = 0.0, -- How close to being taken over
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
    local blue = getCollisions(self.uuid, 2)

    if #red == 0 then
        self.control = self.control + #blue * game.delta / 10
    elseif #blue == 0 then
        self.control = self.control - #red * game.delta / 10
    else
        self.control = self.control + (#blue - #red) * game.delta / 10
    end

    if self.control <= -1.0 then
        self.control = -1.0
        if self.team ~= "RED" then
            self.team = "RED"
            self.shape.colour = glm.u8vec3.new(255, 100, 100)
        end
    elseif self.control >= 1.0 then
        self.control = 1.0

        if self.team ~= "BLUE" then
            self.team = "BLUE"
            self.shape.colour = glm.u8vec3.new(100, 100, 255)
        end
    elseif self.team ~= "DEFAULT" then
        self.team = "DEFAULT"
        self.shape.colour = glm.u8vec3.new(100, 100, 100)
    end
end
