input = {
    handlers = {},
    delta = 0
}

function input.init()
    --[[
        KEYBOARD
    ]]--
    input.handlers["w"] = function(isReleased)
        if isReleased then
            print("w")
        end
    end

    input.handlers["s"] = function(isReleased)
        if isReleased then
            print("s")
        end
    end

    input.handlers["a"] = function(isReleased)
        if isReleased then
            print("a")
        end
    end

    input.handlers["d"] = function(isReleased)
        if isReleased then
            print("d")
        end
    end

    input.handlers["1"] = function(isReleased)
        if isReleased then
            print("1")
            Agent.debug.savePlan = not Agent.debug.savePlan
        end
    end

    input.handlers["2"] = function(isReleased)
        if isReleased then
            print("2")
            game.running = not game.running
        end
    end

    input.handlers["3"] = function(isReleased)
        if isReleased then
            print("3")
        end
    end

    input.handlers["4"] = function(isReleased)
        if isReleased then
            renderer.overlay = not renderer.overlay
        end
    end

    --[[
        MOUSE
    ]]--
    input.handlers["m1"] = function(x, y)
        print(tostring(x)..", "..tostring(y))
    end

    input.handlers["m2"] = function(x, y)
        print(tostring(x)..", "..tostring(y))
    end

    input.handlers["move"] = function(x, y)
    end
end

function input.update(delta)
    input.delta = delta
end
