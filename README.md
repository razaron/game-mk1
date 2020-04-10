# Game
## Overview
There's 2 factions, Red and Blue, fighting for dominance. They mine resources to craft ammo to fight each other. There are also neutral animals (carnivores and herbivores) running around doing there thing.

## Development Phases
- Phase 1: Prototype the game in Lua, using the engine as boiler plate.
- Phase 2: Extract components and system logic from Lua into C++.
- Phase 3: Break performance critical code into tasks and pass them to the task scheduler to make them process in parallel.

## Misc
- `."/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -G "Visual Studio 16 2019" -A x64 -S . -B ../game_vs_solution -D CONFIGURATION=Debug`
