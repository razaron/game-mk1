local actions = require("actions")
local steering = require("steering")

--[[===============================
		AGENT BASE CLASS
===============================]]--
Agent = {
    name = "Agent",
    class = "Agent",
    texID = 0,
    sides = 3,
    col = glm.u8vec3.new(0, 0, 0),
    pos = glm.vec2.new(0, 0),
    vel = glm.vec2.new(0, 0),
    rot = 0.0,

    steer = Steering.new(),
    blackboard = {},

    actions = ActionSet(),
    effects = {},
    curPlan = {},
    curAction = 0,

    isDead = false,

    debug = {
        ["savePlan"] = false
    }
}

function Agent.new(name)
    local self = {}
    setmetatable(self, {__index = Agent})

    self.name = name
    self.texID = 0
    self.col = glm.u8vec3.new(0, 0, 0)
    self.pos = glm.vec2.new(0, 0)
    self.vel = glm.vec2.new(0, 1)
    self.rot = 0.0

    self.steer = Steering.new()
    self.blackboard = {}

    self.actions = ActionSet()
    self.effects = {}

    self.curPlan = {}
    self.curAction = 0

    self.isDead = false

    return self
end

function Agent:sense() -- returns world state
    ws = ConditionSet()
    return ws
end

function Agent:update()
    if self.curAction > 0 then
        local action = self.curPlan[self.curAction]

		if not action.target then self.curAction = 0 return end

        self:move(action.target.pos, action.behaviour)

        if action.effect() then
            if self.curAction < #self.curPlan then
                self.curAction = self.curAction + 1
                self.curPlan[self.curAction] = (self.curPlan[self.curAction])(self)
            else
                self.curAction = 0
            end
        end
    else
        self:move()
        self:plan()
    end
end

function Agent:move(target, behaviour)
    local steering = glm.vec2.new(0, 0) - self.vel * 10

    if behaviour == "SEEK" then
        steering = self.steer:seek(self.pos, target, self.vel)
    elseif behaviour == "ARRIVE" then
        steering = self.steer:arrive(self.pos, target, self.vel, 500)
    elseif behaviour == "MAINTAIN" then
        steering = self.steer:maintain(self.pos, target, self.vel, 128)
    elseif behaviour == "WANDER" then
        steering = self.steer:wander(self.vel)
    end

    if behaviour ~= "PLAYER" and steering.x ~= 0 and steering.y ~= 0 then
        self.vel = glm.limit(self.vel + (steering * game.delta), self.steer.maxVelocity)
        self.pos = self.pos + (self.vel * game.delta)

        self.rot = glm.angle(self.vel, glm.vec2.new(1, 0))
    else
        -- FOR PLAYERS
        self.pos = self.pos + (self.vel * game.delta)
    end

    if self.pos.x < 0 then self.pos.x = 0 end
    if self.pos.x > 4096 then self.pos.x = 4096 end
    if self.pos.y < 0 then self.pos.y = 0 end
    if self.pos.y > 4096 then self.pos.y = 4096 end
end

function Agent:plan()
    local planner = Planner.new(self:sense())

    local plan = planner:plan(self.actions, self:decideGoal())

    if plan:size() > 0 then
        if Agent.debug.savePlan then
            planner:savePlan("dot/"..self.name.."_"..game.frame..".dot")
        end

        -- Add target and effect function to curPlan
        self.curPlan = {}
        for k, v in pairs(plan) do
            local effect = self.effects[v.name]

            table.insert(self.curPlan, effect)
        end

        self.curAction = 1
        self.curPlan[1] = (self.curPlan[1])(self)
    end
end

function Agent:decideGoal()
    return Action.new("GOAL", 0, ConditionSet(), ConditionSet())
end

--[[===============================
		Soldier SUBCLASS
===============================]]--
Soldier = {
    lastShot = math.huge,
    ammo = 0,
    team = "BLUE",
    blackboard = {
        threat = 0,
        ammoAvailable = false
    }
}
setmetatable(Soldier, {__index = Agent})

function Soldier.new(name, pos, col)
    local self = Agent.new(name)
    setmetatable(self, {__index = Soldier})

    self.class = "Soldier"
    self.texID = 1
    self.pos = pos or glm.vec2.new(0, 0)
    self.col = col or glm.u8vec3.new(math.random(255), math.random(255), math.random(255))

    local action = Actions.findTarget(self)
    self.actions:add(action)
    self.effects[action.name] = Effects.findTarget()

    action = Actions.getInRange()
    self.actions:add(action)
    self.effects[action.name] = Effects.getInRange()

    action = Actions.shoot()
    self.actions:add(action)
    self.effects[action.name] = Effects.shoot()

    action = Actions.melee()
    self.actions:add(action)
    self.effects[action.name] = Effects.melee()

    action = Actions.resupply()
    self.actions:add(action)
    self.effects[action.name] = Effects.resupply()

    self.lastShot = Soldier.lastShot
    self.ammo = Soldier.ammo
    self.team = Soldier.team
    self.blackboard = {
        threat = 0,
        ammoAvailable = false
    }

    return self
end

function Soldier:shoot(target)
    if self.lastShot > 1 and self.ammo > 0 then
        self.lastShot = 0
        self.ammo = self.ammo - 1

        local dest = glm.normalize(target - self.pos) * 256
        self.theta = glm.angle(dest, glm.vec2.new(1, 0))

        if self.theta > self.rot - 3.14159 / 2 and self.theta < self.rot + 3.14159 / 2 then
            local bullet = {
                ["owner"] = self.team,
                ["pos"] = glm.vec2.new(self.pos.x, self.pos.y),
                ["target"] = self.pos + dest
            }

            table.insert(game.bullets, bullet)
        end
    end
end

function Soldier:sense()
    local ws = ConditionSet()
    self.blackboard.threat = 0
    for i = 1, #game.agents do
        local agent = game.agents[i]
        if self.team ~= agent.team and glm.length(self.pos - agent.pos) < 1024 then
            self.blackboard.threat = self.blackboard.threat + 1
        end
    end

    self.blackboard.ammoAvailable = false
    for _, b in pairs(game.bases) do
        if b.team == self.team and b.ammo - b.resupplying > 0 then
            self.blackboard.ammoAvailable = true
            break
        end
    end

    ws:add(Condition.new("self", "threat", self.blackboard.threat))
    ws:add(Condition.new("self", "ammo", self.ammo))
    ws:add(Condition.new("base", "hasAmmo", self.blackboard.ammoAvailable))

    return ws
end

function Soldier:decideGoal()
    self:sense()

    if self.blackboard.threat ~= 0 then
        return Action.new("GOAL", 0, ConditionSet(Condition.new("self", "threat", self.blackboard.threat, OPERATION.LESS)), ConditionSet())
    elseif self.ammo < 3 and self.blackboard.ammoAvailable then
        return Action.new("GOAL", 0, ConditionSet(Condition.new("self", "ammo", self.ammo, OPERATION.GREATER)), ConditionSet())
    else
        return false
    end
end

Attacker = {

}
setmetatable(Attacker, {__index = Soldier})

function Attacker.new(name, pos, col)
    local self = Soldier.new(name, pos, col)
    setmetatable(self, {__index = Attacker})

    self.class = "Attacker"

    local action = Actions.capture()
    self.actions:add(action)
    self.effects[action.name] = Effects.capture()

    action = Actions.scout()
    self.actions:add(action)
    self.effects[action.name] = Effects.scout()

    return self
end

function Attacker:sense()
    -- Find unclaimed deposit
    local deposit = false
    for _, depo in pairs(game.deposits) do
        if depo.team ~= self.team then
            deposit = depo
            break
        end
    end

    -- Add relavent worlt state variable
    local ws = Soldier.sense(self)
    self.blackboard.unclaimedDeposit = false
    if deposit then
        self.blackboard.unclaimedDeposit = true
        ws:add(Condition.new("world", "hasDeposit", true))
    end

    return ws
end

function Attacker:decideGoal()
    self:sense()

    if Soldier.decideGoal(self) then
        return Soldier.decideGoal(self)
    elseif self.blackboard.unclaimedDeposit then
        return Action.new("GOAL", 0, ConditionSet(Condition.new("team", "hasDeposit", true, OPERATION.EQUAL)), ConditionSet())
    else
        return Action.new("GOAL", 0, ConditionSet(Condition.new("self", "isScouting", true, OPERATION.EQUAL)), ConditionSet())
    end
end

Defender = {

}
setmetatable(Defender, {__index = Soldier})

function Defender.new(name, pos, col)
    local self = Soldier.new(name, pos, col)
    setmetatable(self, {__index = Defender})

    self.class = "Defender"
    self.sides = 4

    local action = Actions.patrol()
    self.actions:add(action)
    self.effects[action.name] = Effects.patrol()

    return self
end

function Defender:sense()
    -- Find unclaimed deposit
    local deposit = false
    for _, depo in pairs(game.deposits) do
        if depo.team == self.team then
            deposit = true
            break
        end
    end

    -- Add relavent worlt state variable
    local ws = Soldier.sense(self)
    self.blackboard.claimedDeposit = false
    if deposit then
        self.blackboard.claimedDeposit = true
        ws:add(Condition.new("world", "hasDeposit", true))
    end

    return ws
end

function Defender:decideGoal()
    self:sense()

    if Soldier.decideGoal(self) then
        return Soldier.decideGoal(self)
    elseif self.blackboard.claimedDeposit then
        return Action.new("GOAL", 0, ConditionSet(Condition.new("self", "isPatrolling", true, OPERATION.EQUAL)), ConditionSet())
    else
        return Action.new("GOAL", 0, ConditionSet(), ConditionSet())
    end
end

--[[===============================
		Worker SUBCLASS
===============================]]--
Worker = {
    lastCraft = math.huge,
    lastMine = math.huge,
    resource = 0,
    team = "BLUE",
    blackboard = {
        ["threat"] = 0
    }
}
setmetatable(Worker, {__index = Agent})

function Worker.new(name, pos, col)
    local self = Agent.new(name)
    setmetatable(self, {__index = Worker})

    self.class = "Worker"
    self.texID = 1
    self.pos = pos or glm.vec2.new(0, 0)
    self.sides = 16
    self.col = col or glm.u8vec3.new(math.random(255), math.random(255), math.random(255))

    local action = Actions.craft()
    self.actions:add(action)
    self.effects[action.name] = Effects.craft()

    action = Actions.mine(self)
    self.actions:add(action)
    self.effects[action.name] = Effects.mine()

    self.lastCraft = Worker.lastCraft
    self.resource = Worker.resource

    self.team = Worker.team
    self.blackboard = {
        ["threat"] = 0
    }

    return self
end

function Worker:sense()
    local ws = ConditionSet()
    self.blackboard.threat = 0
    for i = 1, #game.agents do
        local agent = game.agents[i]
        if self.team ~= agent.team and glm.length(self.pos - agent.pos) < 1024 then
            self.blackboard.threat = self.blackboard.threat + 1
        end
    end

    ws:add(Condition.new("self", "threat", self.blackboard.threat))

    if self.resource > 0 then
        ws:add(Condition.new("self", "hasResource", true))
    end

    local hasDeposit = false
    for _, d in pairs(game.deposits) do
        if d.team == self.team then hasDeposit = true break end
    end
    ws:add(Condition.new("team", "hasDeposit", hasDeposit))

    return ws
end

function Worker:decideGoal()
    self:sense()
    if self.blackboard.threat ~= 0 then
        return Action.new("GOAL", 0, ConditionSet(Condition.new("self", "threat", self.blackboard.threat, OPERATION.LESS)), ConditionSet())
    else
        return Action.new("GOAL", 0, ConditionSet(Condition.new("self", "craft", true, OPERATION.EQUAL)), ConditionSet())
    end
end
