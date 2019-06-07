Deposit = {
	class = "Deposit",
	pos = glm.vec2.new(0,0),
	team = "DEFAULT",
	value = 0,
	elapsed = 0,
	control = 0.0, -- How close to being taken over
	serving = 0
}

function Deposit.new(pos, control)
	local self = {}
	setmetatable(self, {__index = Deposit})

	self.pos = pos or Deposit.pos
	self.team = Deposit.team
	self.value = Deposit.value
	self.control = control or Deposit.control
	self.serving = Deposit.serving

	return self
end

function Deposit:update(delta)
	self.elapsed = self.elapsed + delta

	if self.elapsed >= 1 then
		self.value = self.value + 1
		self.elapsed = self.elapsed - 1
	end

	local red = {}
	local blue = {}

	for _,agent in pairs(game.agents) do
		if glm.length(agent.pos - self.pos) > 64 then
			-- do nothing
		elseif agent.team == "RED" then
			table.insert(red, agent)
		elseif agent.team == "BLUE" then
			table.insert(blue, agent)
		end
	end

	if #red == 0 then
		self.control = self.control + #blue * delta / 10
	elseif #blue == 0 then
		self.control = self.control - #red * delta / 10
	else
		self.control = self.control + (#blue - #red) * delta / 10
	end

	if self.control <= -1.0 then
		self.control = -1.0
		self.team = "RED"
	elseif self.control >= 1.0 then
		self.control = 1.0
		self.team = "BLUE"
	else
		self.team = "DEFAULT"
	end
end
