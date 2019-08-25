ProFi = require 'ProFi'
local agents = require "agents"
local buildings = require "buildings"

game = {}

function game.init()
    -- BASES
    newBase("RED", glm.vec2.new(128, 128), glm.u8vec3.new(255, 100, 100))
    newBase("GREEN", glm.vec2.new(896, 128), glm.u8vec3.new(100, 255, 100))
    newBase("BLUE", glm.vec2.new(896, 896), glm.u8vec3.new(100, 100, 255))
    newBase("YELLOW", glm.vec2.new(128, 896), glm.u8vec3.new(255, 255, 100))
end