Actions = {
    mine = function(self)
        local pre = ConditionSet(Condition.new("team", "hasDeposit", true, OPERATION.EQUAL))
        local post = ConditionSet(Condition.new("self", "hasResource", true, OPERATION.ASSIGN))

        return Action.new(
            "mine",
            1,
            pre,
            post
        )
    end,
    craft = function()
        local pre = ConditionSet(Condition.new("self", "hasResource", true, OPERATION.EQUAL))
        local post = ConditionSet(Condition.new("self", "craft", true, OPERATION.ASSIGN))

        return Action.new(
            "craft",
            1,
            pre,
            post
        )
    end,
    findTarget = function()
        local pre = ConditionSet()
        local post = ConditionSet(Condition.new("target", "isAvailable", true, OPERATION.ASSIGN)) -- TODO replace with procCond via bound getEntities

        return Action.new(
            "findTarget",
            1,
            pre,
            post
        )
    end,
    getInRange = function()
        local pre = ConditionSet( Condition.new( "target", "isAvailable", true, OPERATION.EQUAL ))
        local post = ConditionSet( Condition.new( "target", "isInRange", true, OPERATION.ASSIGN ))

        return Action.new(
            "getInRange",
            1,
            pre,
            post
        )
    end,
    shoot = function()
        local pre = ConditionSet( 
            Condition.new( "target", "isInRange", true, OPERATION.EQUAL ),
            Condition.new( "self", "ammo", 0, OPERATION.GREATER )
        )
        local post = ConditionSet( 
            Condition.new("target", "isDead", true, OPERATION.ASSIGN ),
            Condition.new("self", "threat", 1, OPERATION.MINUS )
        )

        return Action.new(
            "shoot",
            10,
            pre,
            post
        )
    end,
    melee = function()
        local pre = ConditionSet( Condition.new( "target", "isInRange", true, OPERATION.EQUAL ))
        local post = ConditionSet( 
            Condition.new("target", "isDead", true, OPERATION.ASSIGN ),
            Condition.new("self", "threat", 1, OPERATION.MINUS )
        )

        return Action.new(
            "melee",
            2,
            pre,
            post
        )
    end,
    scout = function()
        local pre = ConditionSet()
        local post = ConditionSet( Condition.new( "self", "isScouting", true, OPERATION.ASSIGN ))

        return Action.new(
            "scout",
            1,
            pre,
            post
        )
    end,
    resupply = function()
        local pre = ConditionSet()
        local post = ConditionSet( Condition.new( "self", "isScouting", true, OPERATION.ASSIGN ))

        return Action.new(
            "resupply",
            1,
            pre,
            post
        )
    end,
    capture = function()
        local pre = ConditionSet(Condition.new("world", "hasDeposit", true, OPERATION.EQUAL))
        local post = ConditionSet( Condition.new( "team", "hasDeposit", true, OPERATION.ASSIGN ))

        return Action.new(
            "capture",
            1,
            pre,
            post
        )
    end,
    patrol = function()
        local pre = ConditionSet()
        local post = ConditionSet( Condition.new( "self", "isPatrolling", true, OPERATION.ASSIGN ))

        return Action.new(
            "patrol",
            1,
            pre,
            post
        )
    end

}
-- foo = {start, run, finish}
Effects = {
    mine = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])

            local bases = EntityVector()
            local deposits = EntityVector()

            for _, e in pairs(entities) do
                if e:has(ComponentType.new("BASE")) then
                    bases:add(e)
                elseif e:has(ComponentType.new("DEPOSIT")) then
                    deposits:add(e)
                end
            end
            
            local base = BaseComponent.new()
            for _, b in pairs(bases) do
                local temp = getBase(b[ComponentType.new("BASE")])

                if temp.team == a.team then
                    base = b
                end
            end

            local target = Entity.new()

            for _, dep in pairs(deposits) do
                local depD = getDeposit(dep[ComponentType.new("DEPOSIT")])

                if target == Entity.new() and depD.team == a.team then
                    target = dep
                elseif depD.team == a.team then
                    local targetD = getDeposit(target[ComponentType.new("DEPOSIT")])

                    local cur = depD.serving / glm.distance(getPosition(base), getPosition(dep))
                    local prev = targetD.serving / glm.distance(getPosition(base), getPosition(target))

                    if depD.team == a.team and cur < prev then
                        target = dep
                    end
                end
            end

            a.target = target

            if a.target ~= Entity.new() then
                local d = getDeposit(a.target[ComponentType.new("DEPOSIT")])
                d.serving = d.serving + 1
            end

            return true
        end,

        run = function(entities, entity)
            local w = getWorker(entity[ComponentType.new("WORKER")])
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local d = getDeposit(a.target[ComponentType.new("DEPOSIT")])
            
            if a.blackboard.threat ~= 0 then
                d.serving = d.serving - 1
                return true
            elseif a.team ~= d.team then
                d.serving = d.serving - 1
                return true
            elseif d.metal == 0 then
                d.serving = d.serving - 1
                return true
            elseif w.elapsed > 0.1 and glm.distance(getPosition(entity), getPosition(a.target)) < 8 then
                w.elapsed = 0
                
                d.metal = d.metal - 1
                w.metal = w.metal + 1

                if d.metal == 0 or w.metal == 5 then
                    d.serving = d.serving - 1
                    return true
                else 
                    return false
                end
            end

            return false
        end
    },

    craft = {
        start = function(entities, entity)
            local w = getWorker(entity[ComponentType.new("WORKER")])
            local a = getAgent(entity[ComponentType.new("AGENT")])

            local bases = EntityVector()
            for _, e in pairs(entities) do
                if e:has(ComponentType.new("BASE")) then
                    bases:add(e)
                end
            end

            local target = Entity.new()
            for _, base in pairs(bases) do
                local b = getBase(base[ComponentType.new("BASE")])

                if b.team == a.team then
                    target = base
                end
            end

            a.target = target

            return true
        end,

        run = function(entities, entity)
            local w = getWorker(entity[ComponentType.new("WORKER")])
            local a = getAgent(entity[ComponentType.new("AGENT")])

            if w.metal == 0 then
                return true
            elseif w.elapsed > 1.0 and glm.distance(getPosition(entity), getPosition(a.target)) < 8 then
                local b = getBase(a.target[ComponentType.new("BASE")])
                
                w.elapsed = 0.0
                w.metal = w.metal - 1
                b.metal = b.metal + 1

                return w.metal == 0
            end
        end
    },

    findTarget = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            local agents = EntityVector()
            for _, e in pairs(entities) do
                if e:has(ComponentType.new("AGENT")) then
                    agents:add(e)
                end
            end

            local target = Entity.new()
            for _, agent in pairs(agents) do
                local a2 = getAgent(agent[ComponentType.new("AGENT")])

                if target == Entity.new() and a.team ~= a2.team then
                    target = agent
                elseif a.team ~= a2.team and glm.distance(getPosition(agent), getPosition(entity)) < glm.distance(getPosition(target), getPosition(entity)) then
                    target = agent
                end
            end

            a.target = target

            if target == Entity.new() then
                return false
            else
                return true
            end
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])

            if entities:find(a.target:getID()) == nil then
                return true
            else
                return true
            end
        end
    },

    getInRange = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if entities:find(a.target:getID()) == nil then
                a.target = Entity.new()
                return true
            end

            local t = getAgent(a.target[ComponentType.new("AGENT")])
            if a.team ~= t.team then
                return true
            else
                return false
            end
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if entities:find(a.target:getID()) == nil then
                return true
            elseif glm.distance(getPosition(a.target), getPosition(entity)) < 64 then
                return true
            else
                return false
            end
        end
    },

    shoot = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if entities:find(a.target:getID()) == nil then
                a.target = Entity.new()
                return true
            end

            local t = getAgent(a.target[ComponentType.new("AGENT")])

            if a.team ~= t.team then
                return true
            else
                return false
            end
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if entities:find(a.target:getID()) == nil then
                return true
            end

            local pos1 = getPosition(a.target)
            local pos2 = getPosition(entity)
            local distance = glm.distance(pos1, pos2)

            if distance > 64 or s.ammo == 0 then
                a.target = Entity.new()
                return true
            elseif s.elapsed > 1.0 and distance < 32 then
                local start = pos1
                local dir = glm.normalize(pos2 - pos1)

                newBullet(start, glm.u8vec3.new(255, 255, 255), dir, entity:getID(), a.team)
                s.ammo = s.ammo - 1
                s.elapsed = 0

                return false
            else
                return false
            end
        end
    },

    melee = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if entities:find(a.target:getID()) == nil then
                return true
            end

            local t = getAgent(a.target[ComponentType.new("AGENT")])

            if a.team ~= t.team then
                return true
            else
                return false
            end
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if entities:find(a.target:getID()) == nil then
                return true
            end            

            local pos1 = getPosition(a.target)
            local pos2 = getPosition(entity)
            local distance = glm.distance(pos1, pos2)

            if distance > 64 then
                a.target = Entity.new()
                return true
            elseif s.elapsed > 1.0 and distance < 16 then
                killAgent(a.target)

                s.elapsed = 0
                a.target = Entity.new()

                return true
            else
                return false
            end
        end
    },

    scout = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            a.target = entity

            return true
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            if a.blackboard.threat ~= 0 or s.ammo < 3 then
                return true
            else
                return false
            end
        end
    },

    resupply = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            local bases = EntityVector()
            for _, e in pairs(entities) do
                if e:has(ComponentType.new("BASE")) then
                    bases:add(e)
                end
            end

            local target = Entity.new()
            for _, base in pairs(bases) do
                local b = getBase(base[ComponentType.new("BASE")])

                if b.team == a.team then
                    target = base
                end
            end

            a.target = target

            return true
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])
            local b = getBase(a.target[ComponentType.new("BASE")])

            if b.metal == 0 or a.blackboard.threat then
                return true
            elseif glm.distance(getPosition(a.target), getPosition(entity)) < 8 then
                return true
            else
                return false
            end
        end
    },

    capture = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            local deposits = EntityVector()
            for _, e in pairs(entities) do
                if e:has(ComponentType.new("DEPOSIT")) then
                    local d = getDeposit(e[ComponentType.new("DEPOSIT")])

                    if d.team ~= a.team then
                        deposits:add(e)
                    end
                end
            end

            if #deposits == 0 then
                return false
            end

            a.target = deposits[math.random(#deposits)]

            return true
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])
            local d = getDeposit(a.target[ComponentType.new("DEPOSIT")])

            if d.team == a.team or a.blackboard.threat ~= 0 then
                return true
            else
                return false
            end
        end
    },

    patrol = {
        start = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])

            local deposits = EntityVector()
            for _, e in pairs(entities) do
                if e:has(ComponentType.new("DEPOSIT")) then
                    local d = getDeposit(e[ComponentType.new("DEPOSIT")])

                    if d.team == a.team then
                        deposits:add(e)
                    end
                end
            end

            if #deposits == 0 then
                return false
            end

            a.target = deposits[math.random(#deposits)]

            return true
        end,

        run = function(entities, entity)
            local a = getAgent(entity[ComponentType.new("AGENT")])
            local s = getSoldier(entity[ComponentType.new("SOLDIER")])
            local d = getDeposit(a.target[ComponentType.new("DEPOSIT")])

            if a.blackboard.threat ~= 0 or s.ammo < 3 or glm.distance(getPosition(a.target), getPosition(entity)) < 2 then
                return true
            else
                return false
            end
        end
    }
}
