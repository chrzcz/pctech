cmake_minimum_required(VERSION 3.22)
include(FindLua)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(C_STANDARD 17)
set(C_STANDARD_REQUIRED 17)

project(pcTech1 C)
set(SRCS    main.c
            src/game/game.c
            src/assets/assets.c
            src/structures/kdTree.c
            src/scripting.c
)
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SRCS})

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(cglm REQUIRED CONFIG REQUIRED)
find_package(Lua 5.4 REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE cglm::cglm)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LUA_LIBRARIES})
IF (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
ENDIF()

file(COPY robots DESTINATION .)