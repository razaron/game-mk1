Steering = {
    maxVelocity = 500,
    maxForce = 500,
    mass = 1.0,
    wanderAngle = 0.0
}

function Steering.new(maxVelocity, maxForce, mass)
    local self = {}
    setmetatable(self, {__index = Steering})

    self.maxVelocity = maxVelocity or Steering.maxVelocity
    self.maxForce = maxForce or Steering.maxForce
    self.mass = mass or Steering.mass
    self.wanderAngle = Steering.wanderAngle

    return self
end

function Steering:seek(position, target, velocity)
    desired = glm.normalize(target - position) * self.maxVelocity

    steering = glm.limit(desired - velocity, self.maxForce)

    return steering
end

function Steering:arrive(position, target, velocity, radius)
    desired = glm.normalize(target - position) * self.maxVelocity

    if glm.length(target - position) < radius then
        desired = desired * glm.length(target - position) / radius
    end

    steering = glm.limit(desired - velocity, self.maxForce)

    return steering
end

function Steering:maintain(position, target, velocity, radius)
    desired = glm.normalize(target - position) * self.maxVelocity

    if glm.length(target - position) < radius then
        desired = desired * - (1 - glm.length(target - position) / radius)
    end

    steering = glm.limit(desired - velocity, self.maxForce)

    return steering
end

function Steering:wander(velocity)
    local center = glm.normalize(velocity) * 64
    if velocity.x == 0 and velocity.y == 0 then
        center = velocity
    end

    local displacement = glm.vec2.new(32 * math.cos(self.wanderAngle), 32 * math.sin(self.wanderAngle))

    local theta = 3.14159*game.delta*16
    self.wanderAngle = self.wanderAngle + math.random() * theta - theta * 0.5
    self.wanderAngle = self.wanderAngle % (3.14159*2)

    steering = glm.normalize(center+displacement)*self.maxForce*10

    return steering
end
