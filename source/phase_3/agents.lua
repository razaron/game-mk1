local actions = require("actions")

Bullet = {
    class = "Bullet",

    transform = nil, pos = {},
    shape = nil,

    owner = "BLUE",
    direction = glm.vec2.new(0, 0),
    origin = glm.vec2.new(0, 0),
    isDead = false
}

function Bullet.new(owner, pos, direction)
    local self = {}
    setmetatable(self, {__index = Bullet})

    self.transform = nil
    self.shape = nil
    self.uuid = nil

    self.pos = pos

    self.owner = owner or Bullet.owner
    self.direction = direction or Bullet.direction
    self.origin = pos or Bullet.origin
    self.isDead = false

    newBullet(self, pos or glm.vec2.new(512, 512), glm.u8vec3.new(255, 255, 255), glm.normalize(direction))

    return self
end

function Bullet:update()
    -- update pos
    self.pos = glm.vec2.new(self.transform.translation)

    -- delete if max dist traveled
    if(glm.length(self.origin - self.pos) > 64) then
        self.isDead = true
        return
    end

    if self.owner == "RED" then self.collisions = getCollisions(self.uuid, 2)
    elseif self.owner == "BLUE" then self.collisions = getCollisions(self.uuid, 1) end

    -- handle bullet-agent collision
    local uid = 0;
    local dist = math.huge
    for _, col in pairs(self.collisions) do
        if col.distance < dist then
            dist = col.distance
            uid = col.target
        end
    end

    if dist ~= math.huge then
        for _, agent in pairs(game.agents) do
            if agent.uuid == uid then
                self.isDead = true
                agent.isDead = true
                break
            end
        end
    end
end

--[[===============================
		AGENT BASE CLASS
===============================]]--
Agent = {
    class = "Agent",
    name = "Agent",

    -- Components
    transform = nil,
    shape = nil,
    motion = nil,
    collider = nil,

    -- AI
    planner = {},
    blackboard = {},

    actions = ActionSet(),
    effects = {},

    curPlan = {},
    curAction = 0,

    isDead = false,

    debug = {
        savePlan = false
    }
}

function Agent.new(name, pos, sides, col, team)
    local self = {}

    setmetatable(self, {__index = Agent})

    self.name = name or Agent.name

    self.transform = nil
    self.shape = nil
    self.uuid = nil
    self.pos = pos

    self.planner = Planner.new()
    self.blackboard = {}

    self.actions = ActionSet()
    self.effects = {}

    self.curPlan = {}
    self.curAction = 0

    self.team = team

    local group = 0
    if team == "RED" then group = 1
    elseif team == "BLUE" then group = 2 end

    newAgent(self, pos or glm.vec2.new(512, 512), sides or 3, col or glm.u8vec3.new(200, 200, 200), group)

    return self
end

function Agent:update()
    self.pos = glm.vec2.new(self.transform.translation)
    if self.team == "RED" then self.collisions = getCollisions(self.uuid, 2)
    elseif self.team == "BLUE" then self.collisions = getCollisions(self.uuid, 1) end

    if self.curAction > 0 then
        local action = self.curPlan[self.curAction]

        if not action.target then self.curAction = 0 return end

        if action.effect() then
            if self.curAction < #self.curPlan then
                self.curAction = self.curAction + 1
                self.curPlan[self.curAction] = (self.curPlan[self.curAction])(self)

                if not self.curPlan[self.curAction].target then self.curAction = 0 return end

                updateSteering(self.uuid, self.curPlan[self.curAction].target.uuid, STEERING_BEHAVIOUR[self.curPlan[self.curAction].behaviour])
            else
                self.curAction = 0
            end
        end
    else
        self:plan()
    end
end

function Agent:sense() -- returns world state
    ws = ConditionSet()
    return ws
end

function Agent:plan()
    self.planner.worldState = self:sense()

    local plan = self.planner:plan(self.actions, self:decideGoal())

    if plan:size() > 0 then
        if Agent.debug.savePlan then
            self.planner:savePlan("dot/"..self.name.."_"..frame..".dot")
        end

        -- Add target and effect function to curPlan
        self.curPlan = {}
        for k, v in pairs(plan) do
            local effect = self.effects[v.name]

            table.insert(self.curPlan, effect)
        end

        self.curAction = 1
        self.curPlan[1] = (self.curPlan[1])(self)

        updateSteering(self.uuid, self.curPlan[1].target.uuid, STEERING_BEHAVIOUR[self.curPlan[1].behaviour])
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
    ammo = 4,
    team = "BLUE",
    blackboard = {
        threat = 0,
        ammoAvailable = false
    }
}
setmetatable(Soldier, {__index = Agent})

function Soldier.new(name, pos, sides, col, team)
    local self = Agent.new(name, pos or glm.vec2.new(0, 0), sides, col or glm.u8vec3.new(math.random(255), math.random(255), math.random(255)), team)
    setmetatable(self, {__index = Soldier})

    self.class = "Soldier"

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
    self.blackboard = {
        threat = 0,
        ammoAvailable = false
    }

    return self
end

function Soldier:update()
    self.lastShot = self.lastShot + game.delta

    Agent.update(self)
end

function Soldier:shoot(target)
    if self.lastShot > 1 and self.ammo > 0 then
        self.lastShot = 0
        self.ammo = self.ammo - 1

        local dest = glm.normalize(target - self.pos) * 256 / 5
        self.theta = glm.angle(dest, glm.vec2.new(1, 0))

        if self.theta > self.transform.rotation - 3.14159 / 2 and self.theta < self.transform.rotation + 3.14159 / 2 then
            factory.construct(
                Bullet.new(self.team, glm.vec2.new(self.pos.x, self.pos.y), glm.normalize(target - self.pos)),
                "bullets",
                {"transform", "shape"}
            )
        end
    end
end

function threat(self)
    self.blackboard.threat = 0
    self.blackboard.threat = #self.collisions
end

function Soldier:sense()
    local ws = ConditionSet()

    threat(self)

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

function Attacker.new(name, pos, col, team)
    local self = Soldier.new(name, pos, 3, col, team)
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

function Defender.new(name, pos, col, team)
    local self = Soldier.new(name, pos, 4, col, team)
    setmetatable(self, {__index = Defender})

    self.class = "Defender"

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

function Worker.new(name, pos, col, team)
    local self = Agent.new(name, pos or glm.vec2.new(0, 0), 8, col or glm.u8vec3.new(math.random(255), math.random(255), math.random(255)), team)
    setmetatable(self, {__index = Worker})

    self.class = "Worker"

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

function Worker:update()
    self.lastMine = self.lastMine + game.delta
    self.lastCraft = self.lastCraft + game.delta

    Agent.update(self)
end

function Worker:sense()
    local ws = ConditionSet()

    threat(self)

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
