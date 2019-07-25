Deposit = {
	class = "Deposit",
	pos = glm.vec2.new(0,0),
	team = "DEFAULT",
	value = 0,
	elapsed = 0,
	control = {red = 0.0, green = 0.0, blue = 0.0, yellow = 0.0}, -- How close to being taken over
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
	local green = {}
	local blue = {}
	local yellow = {}

	for _,agent in pairs(game.agents) do
		if glm.length(agent.pos - self.pos) > 32 then
			-- do nothing
		elseif agent.team == "RED" then
			table.insert(red, agent)
		elseif agent.team == "GREEN" then
			table.insert(green, agent)
		elseif agent.team == "BLUE" then
			table.insert(blue, agent)
		elseif agent.team == "YELLOW" then
			table.insert(yellow, agent)
		end
	end

	local max = 0, leader;
	if #red > max then leader = "RED" end
	if #green > max then leader = "GREEN" end
	if #blue > max then leader = "BLUE" end
	if #yellow > max then leader = "YELLOW" end

	if leader == "RED" then
		local average = (#green + #blue + #yellow) / 3
		if self.control.green > 0.0 or self.control.blue > 0.0 or self.control.yellow > 0.0 then
			self.control.green = self.control.green - (#red - #green) * delta / 10
			self.control.blue = self.control.blue - (#red - #blue) * delta / 10
			self.control.yellow = self.control.yellow - (#red - #yellow) * delta / 10
		else
			self.control.red = self.control.red + (#red - average) * delta / 10
		end
	elseif leader == "GREEN" then
		local average = (#red + #blue + #yellow) / 3
		if self.control.red > 0.0 or self.control.blue > 0.0 or self.control.yellow > 0.0 then
			self.control.red = self.control.red - (#green - #red) * delta / 10
			self.control.blue = self.control.blue - (#green - #blue) * delta / 10
			self.control.yellow = self.control.yellow - (#green - #yellow) * delta / 10
		else
			self.control.green = self.control.green + (#green - average) * delta / 10
		end
	elseif leader == "BLUE" then
		local average = (#green + #red + #yellow) / 3
		if self.control.red > 0.0 or self.control.green > 0.0 or self.control.yellow > 0.0 then
			self.control.green = self.control.green - (#blue - #green) * delta / 10
			self.control.red = self.control.red - (#blue - #red) * delta / 10
			self.control.yellow = self.control.yellow - (#blue - #yellow) * delta / 10
		else
			self.control.blue = self.control.blue + (#blue - average) * delta / 10
		end
	elseif leader == "YELLOW" then
		local average = (#green + #blue + #red) / 3
		if self.control.red > 0.0 or self.control.green > 0.0 or self.control.blue > 0.0 then
			self.control.green = self.control.green - (#yellow - #green) * delta / 10
			self.control.blue = self.control.blue - (#yellow - #blue) * delta / 10
			self.control.red = self.control.red - (#yellow - #red) * delta / 10
		else
			self.control.yellow = self.control.yellow + (#yellow - average) * delta / 10
		end
	end

	if self.control.red <= 0.0 then self.control.red = 0.0 end
	if self.control.green <= 0.0 then self.control.green = 0.0 end
	if self.control.blue <= 0.0 then self.control.blue = 0.0 end
	if self.control.yellow <= 0.0 then self.control.yellow = 0.0 end

	if self.control.red >= 1.0 then 
		self.control.red = 1.0
		self.team = "RED"
	elseif self.control.green >= 1.0 then 
		self.control.green = 1.0 
		self.team = "GREEN"
	elseif self.control.blue >= 1.0 then 
		self.control.blue = 1.0 
		self.team = "BLUE"
	elseif self.control.yellow >= 1.0 then 
		self.control.yellow = 1.0 
		self.team = "YELLOW"
	else
		self.team = "DEFAULT"
	end
end
