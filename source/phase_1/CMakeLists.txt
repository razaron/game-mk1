file(GLOB GAME_SOURCES "*.cpp")
file(GLOB GAME_HEADERS "*.hpp")
file(GLOB GAME_SCRIPTS "*.lua")

add_executable (game-phase-1 ${GAME_SOURCES} ${GAME_HEADERS} ${GAME_SCRIPTS})
set_target_properties(game-phase-1 PROPERTIES COMPILE_FLAGS ${MY_COMPILE_FLAGS} LINK_FLAGS ${MY_LINKER_FLAGS})
set_target_properties(game-phase-1 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)/${CONFIGURATION}")
target_link_libraries (game-phase-1 ${REQUIRED_LIBS})

# COPY LUA FILES
set(FROM_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TO_DIR $<TARGET_FILE_DIR:game-phase-1>)
file(GLOB LUA_FILES "${FROM_DIR}/*.lua")

FOREACH(LUA_FILE ${LUA_FILES})
    add_custom_command(TARGET game-phase-1 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LUA_FILE} ${TO_DIR})
ENDFOREACH()

# COPY RES DIRECTORY
add_custom_command(TARGET game-phase-1 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${FROM_DIR}/res ${TO_DIR}/res)

# COPY DLLS
IF(${CMAKE_GENERATOR} STREQUAL "Visual Studio 15 2017 Win64" OR ${CMAKE_GENERATOR} STREQUAL "Visual Studio 16 2019")
    set(FROM_DIR ${PROJECT_SOURCE_DIR}/3rd_party/libs/Win64/${CONFIGURATION})
    set(TO_DIR $<TARGET_FILE_DIR:game-phase-1>)
    file(GLOB DLL_FILES "${FROM_DIR}/*.dll")

    FOREACH(DLL_FILE ${DLL_FILES})
        add_custom_command(TARGET game-phase-1 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL_FILE} ${TO_DIR})
    ENDFOREACH()
ENDIF()
