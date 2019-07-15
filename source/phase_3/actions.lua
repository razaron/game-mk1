Actions = {
    -------------------
    -- AGENT ACTIONS --
    -------------------
    findTarget = function(self)
        pre = ConditionSet()
        post = ConditionSet()

        procPre = ProceduralConditionSet()
        procPost = ProceduralConditionSet(
            function()
                local isAvailable = false
                for k, agent in pairs(game.agents) do
                    if agent.transform and agent.team ~= self.team then
                        isAvailable = true
                        break
                    end
                end

                return Condition.new("target", "isAvailable", isAvailable, OPERATION.ASSIGN)
            end
        )

        return Action.new(
            "find_target",
            1,
            pre,
            post,
            procPre,
            procPost
        )
    end,
    getInRange = function()
        pre = ConditionSet(
            Condition.new("target", "isAvailable", true, OPERATION.EQUAL)
        )
        post = ConditionSet(
            Condition.new("target", "isInRange", true, OPERATION.ASSIGN)
        )

        return Action.new(
            "get_in_range",
            1,
            pre,
            post
        )
    end,
    shoot = function()
        pre = ConditionSet(
            Condition.new("target", "isInRange", true, OPERATION.EQUAL),
            Condition.new("self", "ammo", 0, OPERATION.GREATER)
        )
        post = ConditionSet(
            Condition.new("target", "isDead", true, OPERATION.ASSIGN),
            Condition.new("self", "threat", 1, OPERATION.MINUS)
        )

        return Action.new(
            "shoot",
            1,
            pre,
            post
        )
    end,
    melee = function()
        pre = ConditionSet(
            Condition.new("target", "isInRange", true, OPERATION.EQUAL)
        )
        post = ConditionSet(
            Condition.new("target", "isDead", true, OPERATION.ASSIGN),
            Condition.new("self", "threat", 1, OPERATION.MINUS)
        )

        return Action.new(
            "melee",
            2,
            pre,
            post
        )
    end,
    scout = function()
        pre = ConditionSet()
        post = ConditionSet(Condition.new("self", "isScouting", true, OPERATION.ASSIGN))

        return Action.new(
            "scout",
            1,
            pre,
            post
        )
    end,
    patrol = function()
        pre = ConditionSet()
        post = ConditionSet(Condition.new("self", "isPatrolling", true, OPERATION.ASSIGN))

        return Action.new(
            "patrol",
            1,
            pre,
            post
        )
    end,

    mine = function(self)
        pre = ConditionSet(Condition.new("team", "hasDeposit", true, OPERATION.EQUAL))
        post = ConditionSet(Condition.new("self", "hasResource", true, OPERATION.ASSIGN))

        procPre = ProceduralConditionSet()
        procPost = ProceduralConditionSet(
            function()
                local hasDeposit = false
                for _, r in pairs(game.deposits) do
                    if r.team == self.team then
                        hasDeposit = true
                        break
                    end
                end

                return Condition.new("team", "hasDeposit", hasDeposit, OPERATION.ASSIGN)
            end
        )

        return Action.new(
            "mine",
            1,
            pre,
            post,
            procPre,
            procPost
        )
    end,
    craft = function()
        pre = ConditionSet(Condition.new("self", "hasResource", true, OPERATION.EQUAL))
        post = ConditionSet(Condition.new("self", "craft", true, OPERATION.ASSIGN))

        return Action.new(
            "craft",
            1,
            pre,
            post
        )
    end,
    resupply = function()
        pre = ConditionSet(Condition.new("base", "hasAmmo", true, OPERATION.EQUAL))
        post = ConditionSet(Condition.new("self", "ammo", 1, OPERATION.PLUS))

        return Action.new(
            "resupply",
            2,
            pre,
            post
        )
    end,
    capture = function()
        pre = ConditionSet(Condition.new("world", "hasDeposit", true, OPERATION.EQUAL))
        post = ConditionSet(Condition.new("team", "hasDeposit", true, OPERATION.ASSIGN))

        return Action.new(
            "capture",
            1,
            pre,
            post
        )
    end

    ----------------------
    -- DIRECTOR ACTIONS --
    ----------------------
}

Effects = {
    -------------------
    -- AGENT EFFECTS --
    -------------------
    findTarget = function()
        return function(self)
            return {
                ["name"] = "find_target",
                ["target"] = self,
                ["behaviour"] = "STOP",

                ["effect"] = function()
                    local target = false
                    for k, agent in pairs(game.agents) do
                        if agent.pos and agent.team ~= self.team then
                            if target == false then
                                target = agent
                            elseif glm.length(self.pos - agent.pos) < glm.length(self.pos - target.pos) then
                                target = agent
                            end
                        end
                    end

                    self.blackboard.target = target

                    return true
                end
            }
        end
    end,
    getInRange = function()
        return function(self)
            return {
                ["name"] = "get_in_range",
                ["target"] = self.blackboard.target,
                ["behaviour"] = "SEEK",

                ["effect"] = function()
                    if self.blackboard.target and (glm.length(self.pos - self.blackboard.target.pos) < 500/4 or self.blackboard.target.isDead) then
                        return true
                    else 
                        return false
                    end
                end
            }
        end
    end,
    shoot = function()
        return function(self)
            return {
                ["name"] = "shoot",
                ["target"] = self.blackboard.target,
                ["behaviour"] = "MAINTAIN",

                ["effect"] = function()
                    if not self.blackboard.target.isDead and self.ammo > 0 then
                        if glm.length(self.pos - self.blackboard.target.pos) > 128 then
                            self.blackboard.target = false
                            return true
                        else
                            if glm.length(self.pos - self.blackboard.target.pos) < 64 then
                                self:shoot(self.blackboard.target.pos)
                            end
                            return false
                        end
                    else
                        return true
                    end
                end
            }
        end
    end,
    melee = function()
        return function(self)
            return {
                ["name"] = "melee",
                ["target"] = self.blackboard.target,
                ["behaviour"] = "SEEK",

                ["effect"] = function()
                    if not self.blackboard.target.isDead then
                        if glm.length(self.pos - self.blackboard.target.pos) > 128 then
                            self.blackboard.target = false

                            return true
                        elseif glm.length(self.pos - self.blackboard.target.pos) < 32 then
                            self.blackboard.target.isDead = true
                            self.blackboard.target = false

                            return true
                        end
                    else
                        return true
                    end

                    return false
                end
            }
        end
    end,
    scout = function()
        return function(self)
            return {
                ["name"] = "scout",
                ["target"] = self,
                ["behaviour"] = "WANDER",

                ["effect"] = function()
                    self:sense()
                    if self.blackboard.threat ~= 0 or (self.ammo < 3 and self.blackboard.ammoAvailable) then
                        return true
                    else
                        return false
                    end
                end
            }
        end
    end,
    patrol = function()
        return function(self)
            local deposits = {}
            for _, d in pairs(game.deposits) do
                if d.team == self.team then
                    table.insert(deposits, d)
                end
            end

            local deposit = deposits[math.random(#deposits)]

            return {
                ["name"] = "patrol",
                ["target"] = deposit,
                ["behaviour"] = "ARRIVE",

                ["effect"] = function()
                    self:sense()
                    if self.blackboard.threat ~= 0 or (self.ammo < 3 and self.blackboard.ammoAvailable) or glm.length(self.pos - deposit.pos) < 16/4 then
                        return true
                    else
                        return false
                    end
                end
            }
        end
    end,
    mine = function()
        return function(self)
            local base = false
            for _, b in pairs(game.bases) do
                if b.team == self.team then
                    base = b
                    break
                end
            end

            local deposit = false
            for _, d in pairs(game.deposits) do
                if d.team == base.team then deposit = d break end
            end

            for _, d in pairs(game.deposits) do
                local cur = d.serving / glm.length(base.pos - d.pos)
                local prev = deposit.serving / glm.length(base.pos - deposit.pos)
                if d.team == base.team and cur < prev then
                    deposit = d
                end
            end

            deposit.serving = deposit.serving + 1

            return {
                ["name"] = "mine",
                ["target"] = deposit,
                ["behaviour"] = "ARRIVE",

                ["effect"] = function()
                    self:sense()
                    if self.blackboard.threat ~= 0 then
                        deposit.serving = deposit.serving - 1
                        return true
                    elseif self.team ~= deposit.team then
                        deposit.serving = deposit.serving - 1
                        return true
                    elseif deposit.value == 0 then
                        deposit.serving = deposit.serving - 1
                        return true
                    elseif self.lastMine > 0.1 and glm.length(self.pos - deposit.pos) < 64/4 then
                        self.lastMine = 0

                        deposit.value = deposit.value - 1
                        self.resource = self.resource + 1

                        if deposit.value == 0 or self.resource == 5 then
                            deposit.serving = deposit.serving - 1
                            return true
                        else
                            return false
                        end
                    else
                        return false
                    end
                end
            }
        end
    end,
    craft = function()
        return function(self)
            local base = false
            for _, b in pairs(game.bases) do
                if b.team == self.team then
                    base = b
                    break
                end
            end

            return {
                ["name"] = "craft",
                ["target"] = base,
                ["behaviour"] = "ARRIVE",

                ["effect"] = function()
                    if self.resource == 0 then
                        return true
                    elseif self.lastCraft > 1 and glm.length(self.pos - base.pos) < 64/4 then
                        self.lastCraft = 0

                        self.resource = self.resource - 1
                        base.ammo = base.ammo + 1

                        if self.resource == 0 then return true
                        else return false end
                    end
                end
            }
        end
    end,
    resupply = function()
        return function(self)
            local base = false
            for _, b in pairs(game.bases) do
                if b.team == self.team then
                    base = b
                    break
                end
            end

            base.resupplying = base.resupplying + 1

            return {
                ["name"] = "resupply",
                ["target"] = base,
                ["behaviour"] = "ARRIVE",

                ["effect"] = function()
                    self:sense()

                    if base.ammo == 0 or self.blackboard.threat > 0 then
                        base.resupplying = base.resupplying - 1
                        return true
                    elseif glm.length(self.pos - base.pos) < 64/4 then
                        self.ammo = self.ammo + 1
                        base.ammo = base.ammo - 1
                        base.resupplying = base.resupplying - 1

                        return true
                    end

                    return false
                end
            }
        end
    end,
    capture = function()
        return function(self)
            local deposits = {}
            for _, d in pairs(game.deposits) do
                if d.team ~= self.team then
                    table.insert(deposits, d)
                end
            end

            local deposit = deposits[math.random(#deposits)]

            return {
                ["name"] = "capture",
                ["target"] = deposit,
                ["behaviour"] = "ARRIVE",

                ["effect"] = function()
                    self:sense()

                    if deposit.team == self.team or self.blackboard.threat ~= 0 then
                        return true
                    else
                        return false
                    end
                end
            }
        end
    end

    ----------------------
    -- DIRECTOR EFFECTS --
    ----------------------
}
