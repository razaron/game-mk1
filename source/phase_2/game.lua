ProFi = require 'ProFi'
local agents = require "agents"
local buildings = require "buildings"

game = {}

function game.init()
    -- BASES
    newBase(TEAM.RED, glm.vec2.new(128, 128), glm.u8vec3.new(255, 0, 0))
    newBase(TEAM.GREEN, glm.vec2.new(896, 128), glm.u8vec3.new(0, 255, 0))
    newBase(TEAM.BLUE, glm.vec2.new(896, 896), glm.u8vec3.new(0, 0, 255))
    newBase(TEAM.YELLOW, glm.vec2.new(128, 896), glm.u8vec3.new(255, 255, 0))

  	-- DEPOSITS
    newDeposit(TEAM.RED, glm.vec2.new(192, 192), glm.u8vec3.new(255, 0, 0))
    newDeposit(TEAM.GREEN, glm.vec2.new(832, 192), glm.u8vec3.new(0, 255, 0))
    newDeposit(TEAM.BLUE, glm.vec2.new(832, 832), glm.u8vec3.new(0, 0, 255))
    newDeposit(TEAM.YELLOW, glm.vec2.new(192, 832), glm.u8vec3.new(255, 255, 0))

    local start = 256
    local span = 512
    local size = 2
    for i = 0, size, 1 do
        for j = 0, size, 1 do
            local x = start + i * span / size
            local y = start + j * span / size 
            newDeposit(TEAM.NONE, glm.vec2.new(x, y), glm.u8vec3.new(100, 100, 100))
        end
    end
end