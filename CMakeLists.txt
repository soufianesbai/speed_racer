cmake_minimum_required(VERSION 3.16)

project(speed_racer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib REQUIRED)

add_library(speed_racer_core
    game/game.cpp
    gameplay/car.cpp
)

target_include_directories(speed_racer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(speed_racer_core
    PUBLIC
        raylib
)

if(MSVC)
    target_compile_options(speed_racer_core PRIVATE /W4)
else()
    target_compile_options(speed_racer_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    add_executable(speed_racer main.cpp)
    target_link_libraries(speed_racer PRIVATE speed_racer_core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rl_bridge/main.cpp")
    add_executable(speed_racer_rl_bridge rl_bridge/main.cpp)
    target_link_libraries(speed_racer_rl_bridge PRIVATE speed_racer_core)
endif()
